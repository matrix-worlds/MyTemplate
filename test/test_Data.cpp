/* -*- C++ -*- */
//
// test_Data.cpp
//
// Unit tests for the backend-agnostic layer in Data.h: CommonEntry<>,
// the ResultsProcessor<> family, CommonDumpProcessor<>, and
// CommonTable<>::dump(). None of this needs a real database -- a fake,
// in-memory table (FakeWidgetTable) stands in for a MySQL-backed one so
// every piece here is exercised without libmysqlclient making a single
// network call.

#include "Data.h"
#include "MiniTest.h"
#include "WidgetTable.h"
#include <iterator>
#include <sstream>
#include <streambuf>
#include <vector>

using namespace MyCommon;

namespace {

/// Redirects std::cout into a string for the duration of its scope.
class CoutCapture
{
public:
  CoutCapture() : _old(std::cout.rdbuf(_buf.rdbuf())) {}
  ~CoutCapture() { std::cout.rdbuf(_old); }
  std::string str() const { return _buf.str(); }

private:
  std::ostringstream _buf;
  std::streambuf* _old;
};

/// Stand-in for DataFactory: satisfies CommonTable<>'s
/// `CommonDataFactory(tableName)` constructor requirement and
/// getTableName() call, with no MySQL involved at all.
class FakeDataFactory
{
public:
  explicit FakeDataFactory(const std::string& tableName) : _tableName(tableName) {}
  virtual ~FakeDataFactory() {}
  const std::string& getTableName() const { return _tableName; }

private:
  std::string _tableName;
};

/// An in-memory CommonTable<>: query() iterates a std::vector instead
/// of running SQL, remove() erases from it. This drives every piece in
/// Data.h (ResultsProcessor<> family, CommonDumpProcessor<>,
/// CommonTable<>::dump()) without a database.
class FakeWidgetTable : public CommonTable<WidgetKey, WidgetEntry, FakeDataFactory>
{
public:
  FakeWidgetTable() : CommonTable<WidgetKey, WidgetEntry, FakeDataFactory>("widget") {}
  virtual ~FakeWidgetTable() {}

  // Same reasoning as WidgetTable's (WidgetTable.h): this class's own
  // destructor above suppresses implicit move generation, and
  // makeThreeWidgetTable() below returns one by value.
  FakeWidgetTable(const FakeWidgetTable&) = delete;
  FakeWidgetTable& operator=(const FakeWidgetTable&) = delete;
  FakeWidgetTable(FakeWidgetTable&&) = default;
  FakeWidgetTable& operator=(FakeWidgetTable&&) = default;

  std::vector<WidgetEntry> entries;

  // Both overloads iterate a snapshot copy, not `entries` itself, so a
  // ResultsProcessor that mutates the table mid-query (DeleteProcessorTemplate,
  // notably) doesn't invalidate the iteration -- the same guarantee a
  // real SELECT's result set gives you against concurrent DELETEs.

  virtual void query(const WidgetKey& key, ResultsProcessor<WidgetEntry>& rp)
  {
    std::vector<WidgetEntry> snapshot = entries;
    for (std::size_t i = 0; i < snapshot.size(); ++i)
    {
      if (snapshot[i].getKey() == key)
      {
        if (!rp.process(snapshot[i])) break;
      }
    }
  }

  virtual void query(ResultsProcessor<WidgetEntry>& rp)
  {
    std::vector<WidgetEntry> snapshot = entries;
    for (std::size_t i = 0; i < snapshot.size(); ++i)
    {
      if (!rp.process(snapshot[i])) break;
    }
  }

  void remove(const WidgetKey& key)
  {
    for (std::vector<WidgetEntry>::iterator it = entries.begin();
         it != entries.end(); )
    {
      if (it->getKey() == key) it = entries.erase(it);
      else ++it;
    }
  }
};

FakeWidgetTable makeThreeWidgetTable()
{
  FakeWidgetTable table;
  table.entries.push_back(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  table.entries.push_back(WidgetEntry(WidgetKey(2), "nut", "hardware", 20));
  table.entries.push_back(WidgetEntry(WidgetKey(3), "washer", "hardware", 30));
  return table;
}

// ------------------------------------------------------------------
// CommonEntry<>
// ------------------------------------------------------------------

void test_CommonEntryDefaultAndKeyConstruction(MiniTest& t)
{
  CommonEntry<WidgetKey> defaulted;
  MT_CHECK(t, defaulted.getKey() == WidgetKey(0));

  CommonEntry<WidgetKey> withKey(WidgetKey(7));
  MT_CHECK(t, withKey.getKey() == WidgetKey(7));
}

void test_CommonEntrySetKey(MiniTest& t)
{
  CommonEntry<WidgetKey> entry;
  entry.setKey(WidgetKey(9));
  MT_CHECK(t, entry.getKey() == WidgetKey(9));
}

void test_CommonEntryGetString(MiniTest& t)
{
  // The base getString() (not WidgetEntry's override) -- format is
  // " [ Key: <key> ] " using the key's own operator<<.
  CommonEntry<WidgetKey> entry(WidgetKey(5));
  MT_CHECK(t, entry.getString() == " [ Key: " + WidgetKey(5).getString() + " ] ");
}

// ------------------------------------------------------------------
// ResultsProcessor<> family
// ------------------------------------------------------------------

void test_CopyProcessor(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();

  std::vector<WidgetEntry> copy;
  CopyProcessor<WidgetEntry, std::back_insert_iterator<std::vector<WidgetEntry> > >
    processor(std::back_inserter(copy));

  table.query(processor);

  MT_CHECK(t, copy.size() == 3);
  MT_CHECK(t, copy[0].getKey() == WidgetKey(1));
  MT_CHECK(t, copy[2].getName() == "washer");
}

void test_FindProcessorMatchFound(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();

  WidgetEntry found;
  bool wasFound = false;
  FindProcessor<WidgetEntry> processor(found, wasFound);

  table.query(WidgetKey(2), processor);

  MT_CHECK(t, wasFound);
  MT_CHECK(t, found.getName() == "nut");
}

void test_FindProcessorNoMatch(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();

  WidgetEntry found;
  bool wasFound = true; // constructor must reset this to false
  FindProcessor<WidgetEntry> processor(found, wasFound);
  MT_CHECK(t, !wasFound); // reset on construction, before any process() call

  table.query(WidgetKey(999), processor);
  MT_CHECK(t, !wasFound);
}

void test_CountProcessorTemplate(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();

  int count = -1;
  CountProcessorTemplate<WidgetEntry> processor(count);
  MT_CHECK(t, count == 0); // reset on construction

  table.query(processor);
  MT_CHECK(t, count == 3);
}

void test_DeleteProcessorTemplate(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();

  DeleteProcessorTemplate<FakeWidgetTable, WidgetEntry> processor(table);
  table.query(processor); // deletes every row it iterates

  MT_CHECK(t, table.entries.empty());
}

void test_CommonDumpProcessorPrintsGetString(MiniTest& t)
{
  WidgetEntry entry(WidgetKey(1), "bolt", "hardware", 10);
  CommonDumpProcessor<WidgetEntry> processor;

  std::string output;
  {
    CoutCapture capture;
    processor.process(entry);
    output = capture.str();
  }
  MT_CHECK(t, output.find(entry.getString()) != std::string::npos);
}

// ------------------------------------------------------------------
// CommonTable<>::dump()
// ------------------------------------------------------------------

void test_CommonTableDump(MiniTest& t)
{
  FakeWidgetTable table = makeThreeWidgetTable();

  std::string output;
  {
    CoutCapture capture;
    table.dump();
    output = capture.str();
  }
  MT_CHECK(t, output.find("widget Dump:") != std::string::npos);
  MT_CHECK(t, output.find("bolt") != std::string::npos);
  MT_CHECK(t, output.find("nut") != std::string::npos);
  MT_CHECK(t, output.find("washer") != std::string::npos);
}

} // namespace

int main()
{
  MiniTest t("test_Data");

  MT_RUN(t, test_CommonEntryDefaultAndKeyConstruction);
  MT_RUN(t, test_CommonEntrySetKey);
  MT_RUN(t, test_CommonEntryGetString);
  MT_RUN(t, test_CopyProcessor);
  MT_RUN(t, test_FindProcessorMatchFound);
  MT_RUN(t, test_FindProcessorNoMatch);
  MT_RUN(t, test_CountProcessorTemplate);
  MT_RUN(t, test_DeleteProcessorTemplate);
  MT_RUN(t, test_CommonDumpProcessorPrintsGetString);
  MT_RUN(t, test_CommonTableDump);

  return t.result();
}
