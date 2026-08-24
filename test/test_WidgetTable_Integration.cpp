/* -*- C++ -*- */
//
// test_WidgetTable_Integration.cpp
//
// End-to-end tests of WidgetTable/DataFactory/MySqlConnection against a
// *real* MySQL server -- the "real database" half deliberately left out
// of test_Data.cpp/test_DataFactory.cpp/test_WidgetTable.cpp.
//
// Requires a MySQL server reachable with the connection parameters
// below and a `widget` table already created (see WidgetTable.h's file
// comment for the CREATE TABLE statement) in the target database. If no
// server is reachable, main() prints a SKIPPED notice and exits 0
// rather than failing -- this test suite is not meant to force every
// environment to run a MySQL server just to build this project.
//
// Connection parameters are read from environment variables so this
// isn't hardwired to one developer's local setup:
//   MYTEMPLATE_MYSQL_HOST     (default "127.0.0.1")
//   MYTEMPLATE_MYSQL_PORT     (default 3306)
//   MYTEMPLATE_MYSQL_USER     (default "root")
//   MYTEMPLATE_MYSQL_PASSWORD (default "")
//   MYTEMPLATE_MYSQL_DATABASE (default "mytemplate_test")

#include "DataError.h"
#include "MiniTest.h"
#include "WidgetTable.h"
#include <cstdlib>
#include <iostream>
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

/// A freshly connect()ed WidgetTable with an empty `widget` table.
WidgetTable freshTable()
{
  WidgetTable table;
  table.connect(testConnectionParams());
  table.executeQuery("DELETE FROM widget"); // doesn't throw on 0 rows affected
  return table;
}

void test_InsertAndQueryByKey(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));

  WidgetEntry found;
  bool wasFound = false;
  FindProcessor<WidgetEntry> rp(found, wasFound);
  table.query(WidgetKey(1), rp);

  MT_CHECK(t, wasFound);
  MT_CHECK(t, found.getName() == "bolt");
  MT_CHECK(t, found.getQuantity() == 10);
}

void test_QueryByKeyNotFound(MiniTest& t)
{
  WidgetTable table = freshTable();

  WidgetEntry found;
  bool wasFound = true;
  FindProcessor<WidgetEntry> rp(found, wasFound);
  table.query(WidgetKey(999), rp);

  MT_CHECK(t, !wasFound);
}

void test_DuplicateInsertThrows(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));

  bool threw = false;
  try { table.insert(WidgetEntry(WidgetKey(1), "bolt-again", "hardware", 5)); }
  catch (const DuplicateEntry&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_UpdateExistingRow(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  table.update(WidgetEntry(WidgetKey(1), "bolt", "hardware", 99));

  WidgetEntry found;
  bool wasFound = false;
  FindProcessor<WidgetEntry> rp(found, wasFound);
  table.query(WidgetKey(1), rp);

  MT_CHECK(t, wasFound);
  MT_CHECK(t, found.getQuantity() == 99);
}

void test_UpdateNonexistentRowThrows(MiniTest& t)
{
  WidgetTable table = freshTable();

  bool threw = false;
  try { table.update(WidgetEntry(WidgetKey(404), "nope", "hardware", 1)); }
  catch (const EntryNotFound&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_RemoveRow(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  table.remove(WidgetKey(1));

  WidgetEntry found;
  bool wasFound = true;
  FindProcessor<WidgetEntry> rp(found, wasFound);
  table.query(WidgetKey(1), rp);
  MT_CHECK(t, !wasFound);
}

void test_RemoveNonexistentRowThrows(MiniTest& t)
{
  WidgetTable table = freshTable();

  bool threw = false;
  try { table.remove(WidgetKey(404)); }
  catch (const EntryNotFound&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_QueryAllViaCopyProcessor(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  table.insert(WidgetEntry(WidgetKey(2), "nut", "hardware", 20));
  table.insert(WidgetEntry(WidgetKey(3), "washer", "hardware", 30));

  std::vector<WidgetEntry> all;
  CopyProcessor<WidgetEntry, std::back_insert_iterator<std::vector<WidgetEntry> > >
    rp(std::back_inserter(all));
  table.query(rp);

  MT_CHECK(t, all.size() == 3);
}

void test_CountProcessorAgainstRealQuery(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  table.insert(WidgetEntry(WidgetKey(2), "nut", "hardware", 20));

  int count = -1;
  CountProcessorTemplate<WidgetEntry> rp(count);
  table.query(rp);
  MT_CHECK(t, count == 2);
}

void test_DeleteProcessorAgainstRealQuery(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  table.insert(WidgetEntry(WidgetKey(2), "nut", "hardware", 20));

  // Snapshot first: DeleteProcessorTemplate's remove() calls would
  // otherwise mutate the very MYSQL_RES rows are being fetched from.
  std::vector<WidgetEntry> all;
  CopyProcessor<WidgetEntry, std::back_insert_iterator<std::vector<WidgetEntry> > >
    copy(std::back_inserter(all));
  table.query(copy);

  DeleteProcessorTemplate<WidgetTable, WidgetEntry> del(table);
  for (std::size_t i = 0; i < all.size(); ++i) del.process(all[i]);

  int count = -1;
  CountProcessorTemplate<WidgetEntry> countRp(count);
  table.query(countRp);
  MT_CHECK(t, count == 0);
}

void test_CommonTableDumpAgainstRealData(MiniTest& t)
{
  WidgetTable table = freshTable();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));

  std::string output;
  {
    CoutCapture capture;
    table.dump();
    output = capture.str();
  }
  MT_CHECK(t, output.find("widget Dump:") != std::string::npos);
  MT_CHECK(t, output.find("bolt") != std::string::npos);
}

void test_TransactionRollbackReverts(MiniTest& t)
{
  WidgetTable table = freshTable();

  MySqlConnection* conn = table.getConnection();
  conn->startTransaction();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  conn->rollbackTransaction();

  int count = -1;
  CountProcessorTemplate<WidgetEntry> rp(count);
  table.query(rp);
  MT_CHECK(t, count == 0); // rolled back: never persisted
}

void test_TransactionCommitPersists(MiniTest& t)
{
  WidgetTable table = freshTable();

  MySqlConnection* conn = table.getConnection();
  conn->startTransaction();
  table.insert(WidgetEntry(WidgetKey(1), "bolt", "hardware", 10));
  conn->commitTransaction();

  int count = -1;
  CountProcessorTemplate<WidgetEntry> rp(count);
  table.query(rp);
  MT_CHECK(t, count == 1);
}

} // namespace

int main()
{
  // Probe connectivity once up front; skip the whole suite (exit 0,
  // not a failure) if no server is reachable, rather than forcing every
  // build of this project to run a MySQL server.
  try
  {
    WidgetTable probe;
    probe.connect(testConnectionParams());
  }
  catch (const ServiceUnavailable& e)
  {
    std::cout << "[test_WidgetTable_Integration] SKIPPED: no MySQL server "
                 "reachable (" << e.what() << ")" << std::endl;
    return 0;
  }

  MiniTest t("test_WidgetTable_Integration");

  MT_RUN(t, test_InsertAndQueryByKey);
  MT_RUN(t, test_QueryByKeyNotFound);
  MT_RUN(t, test_DuplicateInsertThrows);
  MT_RUN(t, test_UpdateExistingRow);
  MT_RUN(t, test_UpdateNonexistentRowThrows);
  MT_RUN(t, test_RemoveRow);
  MT_RUN(t, test_RemoveNonexistentRowThrows);
  MT_RUN(t, test_QueryAllViaCopyProcessor);
  MT_RUN(t, test_CountProcessorAgainstRealQuery);
  MT_RUN(t, test_DeleteProcessorAgainstRealQuery);
  MT_RUN(t, test_CommonTableDumpAgainstRealData);
  MT_RUN(t, test_TransactionRollbackReverts);
  MT_RUN(t, test_TransactionCommitPersists);

  return t.result();
}
