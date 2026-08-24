/* -*- C++ -*- */
//
// test_CacheData.cpp
//
// Unit tests for CacheData.h -- the last link in the chain this project
// has built: BasicType<> -> CommonKey1..6<> -> CommonEntry<>/CommonTable<>
// -> *SingleVersionSnapshotCacheData<>. None of this needs a live MySQL
// server: FakeWidgetTable (below) stands in for WidgetTable, backed by
// a static in-memory "database" every fresh `Table table;` CacheData.h's
// reload() constructs can see -- the same way every fresh real
// WidgetTable sees the same live MySQL server.
//
// See test/test_WidgetTable_Integration.cpp's sibling,
// test_CacheData_Integration.cpp, for the same coverage against a real
// MySQL server, completing the deep-verification pass end to end.

#include "CacheData.h"
#include "MiniTest.h"
#include "WidgetTable.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <vector>

using namespace MyCommon;

namespace {

/// Stand-in for DataFactory (same role as in test_Data.cpp).
class FakeDataFactory
{
public:
  explicit FakeDataFactory(const std::string& tableName) : _tableName(tableName) {}
  virtual ~FakeDataFactory() {}
  const std::string& getTableName() const { return _tableName; }

private:
  std::string _tableName;
};

/// A static, process-wide "database" every fresh FakeWidgetTable
/// instance reads from -- mirroring how every fresh real WidgetTable
/// instance talks to the same live MySQL server. Each test resets it.
class FakeWidgetDatabase
{
public:
  static std::vector<WidgetEntry>& rows()
  {
    static std::vector<WidgetEntry> data;
    return data;
  }

  static void reset(const std::vector<WidgetEntry>& seed) { rows() = seed; }
};

class FakeWidgetTable : public CommonTable<WidgetKey, WidgetEntry, FakeDataFactory>
{
public:
  FakeWidgetTable() : CommonTable<WidgetKey, WidgetEntry, FakeDataFactory>("widget") {}
  virtual ~FakeWidgetTable() {}

  // Same reasoning as WidgetTable's (WidgetTable.h): this class's own
  // destructor above suppresses implicit move generation.
  FakeWidgetTable(const FakeWidgetTable&) = delete;
  FakeWidgetTable& operator=(const FakeWidgetTable&) = delete;
  FakeWidgetTable(FakeWidgetTable&&) = default;
  FakeWidgetTable& operator=(FakeWidgetTable&&) = default;

  virtual void query(const WidgetKey& key, ResultsProcessor<WidgetEntry>& rp)
  {
    std::vector<WidgetEntry> snapshot = FakeWidgetDatabase::rows();
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
    std::vector<WidgetEntry> snapshot = FakeWidgetDatabase::rows();
    for (std::size_t i = 0; i < snapshot.size(); ++i)
    {
      if (!rp.process(snapshot[i])) break;
    }
  }

  /// The constrained query ConstrainedSingleVersionSnapshotCacheData<>
  /// calls: only rows in `category`.
  void query(const std::string& category, ResultsProcessor<WidgetEntry>& rp)
  {
    std::vector<WidgetEntry> snapshot = FakeWidgetDatabase::rows();
    for (std::size_t i = 0; i < snapshot.size(); ++i)
    {
      if (snapshot[i].getCategory() == category)
      {
        if (!rp.process(snapshot[i])) break;
      }
    }
  }

  void remove(const WidgetKey& key)
  {
    std::vector<WidgetEntry>& data = FakeWidgetDatabase::rows();
    for (std::vector<WidgetEntry>::iterator it = data.begin(); it != data.end(); )
    {
      if (it->getKey() == key) it = data.erase(it);
      else ++it;
    }
  }
};

typedef UnconstrainedSingleVersionSnapshotCacheData<WidgetKey, WidgetEntry, FakeWidgetTable>
  WidgetCacheData;
typedef ConstrainedSingleVersionSnapshotCacheData<WidgetKey, WidgetEntry, FakeWidgetTable,
          DumpableCommonMap<WidgetKey, RCPtr<CacheEntry<WidgetEntry> > >,
          CommonCacheProcessor<WidgetKey, WidgetEntry,
            DumpableCommonMap<WidgetKey, RCPtr<CacheEntry<WidgetEntry> > > >,
          std::string>
  WidgetCategoryCacheData;

void seedThreeWidgetsTwoCategories()
{
  std::vector<WidgetEntry> data;
  data.push_back(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  data.push_back(WidgetEntry(WidgetKey(2), "nut", "hardware", 20));
  data.push_back(WidgetEntry(WidgetKey(3), "screwdriver", "tools", 5));
  FakeWidgetDatabase::reset(data);
}

// ------------------------------------------------------------------
// CacheEntry<>
// ------------------------------------------------------------------

void test_CacheEntryDefaultConstruction(MiniTest& t)
{
  CacheEntry<WidgetEntry> ce;
  MT_CHECK(t, ce.getKey() == WidgetKey(0));
}

void test_CacheEntryWrapsEntryAndInheritsAccessors(MiniTest& t)
{
  WidgetEntry entry(WidgetKey(1), "bolt", "hardware", 10);
  CacheEntry<WidgetEntry> ce(entry);

  MT_CHECK(t, ce.getKey() == WidgetKey(1));
  MT_CHECK(t, ce.getName() == "bolt");   // WidgetEntry's own accessor
  MT_CHECK(t, ce.getString() == entry.getString()); // inherited override
}

void test_CacheEntryIsRefCounted(MiniTest& t)
{
  RCPtr<CacheEntry<WidgetEntry> > p(new CacheEntry<WidgetEntry>(
    WidgetEntry(WidgetKey(1), "bolt", "hardware", 10)));
  MT_CHECK(t, p->getRefCount() == 1);

  RCPtr<CacheEntry<WidgetEntry> > p2(p);
  MT_CHECK(t, p->getRefCount() == 2);
}

// ------------------------------------------------------------------
// CommonCacheProcessor<>
// ------------------------------------------------------------------

void test_CommonCacheProcessorInsertsByGetCacheKey(MiniTest& t)
{
  typedef DumpableCommonMap<WidgetKey, RCPtr<CacheEntry<WidgetEntry> > > Container;
  Container container;
  CommonCacheProcessor<WidgetKey, WidgetEntry, Container> processor(container);

  WidgetEntry entry(WidgetKey(5), "washer", "hardware", 1);
  MT_CHECK(t, processor.process(entry));

  RCPtr<CacheEntry<WidgetEntry> > found;
  MT_CHECK(t, container.lookupDataEntry(entry.getCacheKey(), found));
  MT_CHECK(t, found->getName() == "washer");
}

// ------------------------------------------------------------------
// UnconstrainedSingleVersionSnapshotCacheData<>
// ------------------------------------------------------------------

void test_UnconstrainedCacheDataConstruction(MiniTest& t)
{
  WidgetCacheData cache("widget");
  MT_CHECK(t, cache.getTableName() == "widget");
  MT_CHECK(t, cache.size() == 0); // empty until reload()
}

void test_UnconstrainedCacheDataReloadPopulatesFromTable(MiniTest& t)
{
  seedThreeWidgetsTwoCategories();

  WidgetCacheData cache("widget");
  cache.reload();

  MT_CHECK(t, cache.size() == 3);
}

void test_UnconstrainedCacheDataLookupEntry(MiniTest& t)
{
  seedThreeWidgetsTwoCategories();

  WidgetCacheData cache("widget");
  cache.reload();

  RCPtr<CacheEntry<WidgetEntry> > found;
  MT_CHECK(t, cache.lookupEntry(WidgetKey(2), found));
  MT_CHECK(t, found->getName() == "nut");

  RCPtr<CacheEntry<WidgetEntry> > notFound;
  MT_CHECK(t, !cache.lookupEntry(WidgetKey(999), notFound));
}

/// reload() rebuilds into a private local container and swaps it in
/// atomically (via DumpableCommonMap::reload(), from CommonMap.h) --
/// a second reload() after the underlying data changed must reflect the
/// new data, not an accumulation of both.
void test_UnconstrainedCacheDataReloadReflectsLatestData(MiniTest& t)
{
  seedThreeWidgetsTwoCategories();

  WidgetCacheData cache("widget");
  cache.reload();
  MT_CHECK(t, cache.size() == 3);

  std::vector<WidgetEntry> updated;
  updated.push_back(WidgetEntry(WidgetKey(4), "hammer", "tools", 2));
  FakeWidgetDatabase::reset(updated);

  cache.reload();
  MT_CHECK(t, cache.size() == 1);

  RCPtr<CacheEntry<WidgetEntry> > found;
  MT_CHECK(t, cache.lookupEntry(WidgetKey(4), found));

  RCPtr<CacheEntry<WidgetEntry> > stale;
  MT_CHECK(t, !cache.lookupEntry(WidgetKey(1), stale)); // gone after reload
}

void test_UnconstrainedCacheDataDump(MiniTest& t)
{
  seedThreeWidgetsTwoCategories();

  WidgetCacheData cache("widget");
  cache.reload();

  std::ostringstream captured;
  std::streambuf* old = std::cout.rdbuf(captured.rdbuf());
  cache.dump();
  std::cout.rdbuf(old);

  std::string output = captured.str();
  MT_CHECK(t, output.find("widget CacheData Dump:") != std::string::npos);
  MT_CHECK(t, output.find("bolt") != std::string::npos);
}

void test_UnconstrainedCacheDataSingleton(MiniTest& t)
{
  WidgetCacheData::resetInstanceForTesting();

  WidgetCacheData cache("widget");
  MT_NO_THROW(t, WidgetCacheData::setInstance(&cache));
  MT_CHECK(t, WidgetCacheData::getInstance() == &cache);

  bool threwOnDoubleSet = false;
  try { WidgetCacheData::setInstance(&cache); }
  catch (const std::logic_error&) { threwOnDoubleSet = true; }
  MT_CHECK(t, threwOnDoubleSet);

  WidgetCacheData::resetInstanceForTesting();
}

void test_UnconstrainedCacheDataGetInstanceBeforeSetThrows(MiniTest& t)
{
  WidgetCacheData::resetInstanceForTesting();

  bool threw = false;
  try { WidgetCacheData::getInstance(); }
  catch (const std::logic_error&) { threw = true; }
  MT_CHECK(t, threw);
}

// ------------------------------------------------------------------
// ConstrainedSingleVersionSnapshotCacheData<>
// ------------------------------------------------------------------

void test_ConstrainedCacheDataConstruction(MiniTest& t)
{
  WidgetCategoryCacheData cache("hardware", "widget");
  MT_CHECK(t, cache.getCriteria() == "hardware");
  MT_CHECK(t, cache.getTableName() == "widget");
}

void test_ConstrainedCacheDataReloadOnlyLoadsMatchingCriteria(MiniTest& t)
{
  seedThreeWidgetsTwoCategories(); // 2 hardware, 1 tools

  WidgetCategoryCacheData hardwareCache("hardware", "widget");
  hardwareCache.reload();
  MT_CHECK(t, hardwareCache.size() == 2);

  RCPtr<CacheEntry<WidgetEntry> > bolt;
  MT_CHECK(t, hardwareCache.lookupEntry(WidgetKey(1), bolt));

  RCPtr<CacheEntry<WidgetEntry> > screwdriver;
  MT_CHECK(t, !hardwareCache.lookupEntry(WidgetKey(3), screwdriver)); // wrong category
}

void test_ConstrainedCacheDataDifferentCriteriaSeeDifferentData(MiniTest& t)
{
  seedThreeWidgetsTwoCategories();

  WidgetCategoryCacheData hardwareCache("hardware", "widget");
  WidgetCategoryCacheData toolsCache("tools", "widget");
  hardwareCache.reload();
  toolsCache.reload();

  MT_CHECK(t, hardwareCache.size() == 2);
  MT_CHECK(t, toolsCache.size() == 1);

  RCPtr<CacheEntry<WidgetEntry> > found;
  MT_CHECK(t, toolsCache.lookupEntry(WidgetKey(3), found));
  MT_CHECK(t, found->getName() == "screwdriver");
}

} // namespace

int main()
{
  MiniTest t("test_CacheData");

  MT_RUN(t, test_CacheEntryDefaultConstruction);
  MT_RUN(t, test_CacheEntryWrapsEntryAndInheritsAccessors);
  MT_RUN(t, test_CacheEntryIsRefCounted);
  MT_RUN(t, test_CommonCacheProcessorInsertsByGetCacheKey);
  MT_RUN(t, test_UnconstrainedCacheDataConstruction);
  MT_RUN(t, test_UnconstrainedCacheDataReloadPopulatesFromTable);
  MT_RUN(t, test_UnconstrainedCacheDataLookupEntry);
  MT_RUN(t, test_UnconstrainedCacheDataReloadReflectsLatestData);
  MT_RUN(t, test_UnconstrainedCacheDataDump);
  MT_RUN(t, test_UnconstrainedCacheDataSingleton);
  MT_RUN(t, test_UnconstrainedCacheDataGetInstanceBeforeSetThrows);
  MT_RUN(t, test_ConstrainedCacheDataConstruction);
  MT_RUN(t, test_ConstrainedCacheDataReloadOnlyLoadsMatchingCriteria);
  MT_RUN(t, test_ConstrainedCacheDataDifferentCriteriaSeeDifferentData);

  return t.result();
}
