/* -*- C++ -*- */
//
// test_CacheData_Integration.cpp
//
// End-to-end test of the whole chain this project set out to validate,
// against a *real* MySQL server:
//
//   BasicType<>      (int32, u_int32, ...)
//   -> CommonKey1<>   (WidgetKey)
//   -> CommonEntry<>/CommonTable<>   (WidgetEntry / WidgetTable)
//   -> *SingleVersionSnapshotCacheData<>   (CacheData.h)
//
// i.e. rows really inserted through WidgetTable into MySQL, really
// fetched back out via WidgetTable::query(), really wrapped in
// CacheEntry<>/RCPtr<> and stored in a CacheData<>'s DumpableCommonMap<>,
// really looked back up by key.
//
// Requires a MySQL server reachable with the connection parameters
// below and a `widget` table already created (see WidgetTable.h's file
// comment for the CREATE TABLE statement). If no server is reachable,
// main() prints a SKIPPED notice and exits 0, same as
// test_WidgetTable_Integration.cpp.
//
// Uses DataFactory::setDefaultConnectionParams() (see DataFactory.h) so
// that the bare `Table table;` CacheData<>::reload() constructs
// internally -- with no chance for this test to call connect() on it --
// still ends up connected, the way the original's BaseData connected
// implicitly from environment configuration.

#include "CacheData.h"
#include "DataError.h"
#include "MiniTest.h"
#include "WidgetTable.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <streambuf>

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

std::string envOr(const char* name, const std::string& fallback)
{
  const char* v = std::getenv(name);
  return (v != 0 && v[0] != '\0') ? std::string(v) : fallback;
}

MySqlConnectionParams testConnectionParams()
{
  MySqlConnectionParams params;
  params.host = envOr("MYTEMPLATE_MYSQL_HOST", "127.0.0.1");
  params.user = envOr("MYTEMPLATE_MYSQL_USER", "root");
  params.password = envOr("MYTEMPLATE_MYSQL_PASSWORD", "");
  params.database = envOr("MYTEMPLATE_MYSQL_DATABASE", "mytemplate_test");
  params.port = static_cast<unsigned int>(
    std::atoi(envOr("MYTEMPLATE_MYSQL_PORT", "3306").c_str()));
  return params;
}

/// A connected WidgetTable with an empty `widget` table -- connects via
/// the ordinary explicit connect() (not the default-params mechanism
/// under test), so table setup here doesn't depend on it.
WidgetTable freshTable()
{
  WidgetTable table;
  table.connect(testConnectionParams());
  table.executeQuery("DELETE FROM widget");
  return table;
}

typedef UnconstrainedSingleVersionSnapshotCacheData<WidgetKey, WidgetEntry, WidgetTable>
  WidgetCacheData;
typedef ConstrainedSingleVersionSnapshotCacheData<WidgetKey, WidgetEntry, WidgetTable,
          DumpableCommonMap<WidgetKey, RCPtr<CacheEntry<WidgetEntry> > >,
          CommonCacheProcessor<WidgetKey, WidgetEntry,
            DumpableCommonMap<WidgetKey, RCPtr<CacheEntry<WidgetEntry> > > >,
          std::string>
  WidgetCategoryCacheData;

void test_ReloadPopulatesFromRealTable(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  table.insert(WidgetEntry(WidgetKey(2), "nut", "hardware", 20));

  WidgetCacheData cache("widget");
  cache.reload();

  MT_CHECK(t, cache.size() == 2);

  RCPtr<CacheEntry<WidgetEntry> > found;
  MT_CHECK(t, cache.lookupEntry(WidgetKey(1), found));
  MT_CHECK(t, found->getName() == "bolt");
  MT_CHECK(t, found->getQuantity() == 10);
}

void test_LookupEntryMissReturnsFalse(MiniTest& t)
{
  freshTable(); // empty table

  WidgetCacheData cache("widget");
  cache.reload();

  RCPtr<CacheEntry<WidgetEntry> > found;
  MT_CHECK(t, !cache.lookupEntry(WidgetKey(999), found));
}

/// Same guarantee as the no-DB test in test_CacheData.cpp, now against
/// a real table: a second reload() after the underlying data actually
/// changed in MySQL reflects the new state, not an accumulation.
void test_ReloadReflectsLatestRealData(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));

  WidgetCacheData cache("widget");
  cache.reload();
  MT_CHECK(t, cache.size() == 1);

  table.remove(WidgetKey(1));
  table.insert(WidgetEntry(WidgetKey(2), "nut", "hardware", 20));
  table.insert(WidgetEntry(WidgetKey(3), "washer", "hardware", 30));

  cache.reload();
  MT_CHECK(t, cache.size() == 2);

  RCPtr<CacheEntry<WidgetEntry> > stale;
  MT_CHECK(t, !cache.lookupEntry(WidgetKey(1), stale));

  RCPtr<CacheEntry<WidgetEntry> > fresh;
  MT_CHECK(t, cache.lookupEntry(WidgetKey(3), fresh));
}

/// Two lookups for the same key return handles to the *same* CacheEntry
/// (same pointer, shared reference count) -- proving the cache really
/// stores one reference-counted object per row, not a copy per lookup.
void test_LookupEntryReturnsSharedHandle(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));

  WidgetCacheData cache("widget");
  cache.reload();

  RCPtr<CacheEntry<WidgetEntry> > first;
  RCPtr<CacheEntry<WidgetEntry> > second;
  MT_CHECK(t, cache.lookupEntry(WidgetKey(1), first));
  MT_CHECK(t, cache.lookupEntry(WidgetKey(1), second));

  MT_CHECK(t, first.getPtr() == second.getPtr());
  MT_CHECK(t, first->getRefCount() >= 3); // cache's own + first + second
}

void test_DumpAgainstRealData(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));

  WidgetCacheData cache("widget");
  cache.reload();

  std::string output;
  {
    CoutCapture capture;
    cache.dump();
    output = capture.str();
  }
  MT_CHECK(t, output.find("widget CacheData Dump:") != std::string::npos);
  MT_CHECK(t, output.find("bolt") != std::string::npos);
}

void test_ConstrainedCacheDataFiltersByCategoryAgainstRealTable(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  table.insert(WidgetEntry(WidgetKey(2), "nut", "hardware", 20));
  table.insert(WidgetEntry(WidgetKey(3), "screwdriver", "tools", 5));

  WidgetCategoryCacheData hardwareCache("hardware", "widget");
  hardwareCache.reload();

  MT_CHECK(t, hardwareCache.size() == 2);

  RCPtr<CacheEntry<WidgetEntry> > bolt;
  MT_CHECK(t, hardwareCache.lookupEntry(WidgetKey(1), bolt));

  RCPtr<CacheEntry<WidgetEntry> > screwdriver;
  MT_CHECK(t, !hardwareCache.lookupEntry(WidgetKey(3), screwdriver)); // wrong category

  WidgetCategoryCacheData toolsCache("tools", "widget");
  toolsCache.reload();
  MT_CHECK(t, toolsCache.size() == 1);
}

} // namespace

int main()
{
  MySqlConnectionParams params = testConnectionParams();

  try
  {
    WidgetTable probe;
    probe.connect(params);
  }
  catch (const ServiceUnavailable& e)
  {
    std::cout << "[test_CacheData_Integration] SKIPPED: no MySQL server "
                 "reachable (" << e.what() << ")" << std::endl;
    return 0;
  }

  // So CacheData<>::reload()'s internal `Table table;` connects too.
  DataFactory::setDefaultConnectionParams(params);

  MiniTest t("test_CacheData_Integration");

  MT_RUN(t, test_ReloadPopulatesFromRealTable);
  MT_RUN(t, test_LookupEntryMissReturnsFalse);
  MT_RUN(t, test_ReloadReflectsLatestRealData);
  MT_RUN(t, test_LookupEntryReturnsSharedHandle);
  MT_RUN(t, test_DumpAgainstRealData);
  MT_RUN(t, test_ConstrainedCacheDataFiltersByCategoryAgainstRealTable);

  DataFactory::clearDefaultConnectionParams();

  return t.result();
}
