/* -*- C++ -*- */
//
// test_DataFactory_Integration.cpp
//
// End-to-end tests of DataFactory/MySqlConnection against a *real* MySQL
// server -- driving DataFactory directly with raw SQL, rather than
// through WidgetTable (see test_WidgetTable_Integration.cpp) or CacheData
// (see test_CacheData_Integration.cpp). This is the layer those two
// build on, so it covers what's specific to *this* layer and not
// exercised elsewhere:
//   - processError()'s mapping of real MySQL error codes (duplicate key,
//     syntax error) to this project's exceptions.
//   - Multiple DataFactory instances sharing one underlying MySqlConnection.
//   - MySqlConnection::disconnect() and reconnecting afterwards.
//   - setDefaultConnectionParams() actually producing a connected
//     DataFactory with no explicit connect() call.
//
// Requires a MySQL server reachable with the connection parameters below
// and a `widget` table already created (see WidgetTable.h's file comment
// for the CREATE TABLE statement). If no server is reachable, main()
// prints a SKIPPED notice and exits 0, same as the other two integration
// test files.

#include "DataError.h"
#include "DataFactory.h"
#include "MiniTest.h"
#include <cstdlib>
#include <iostream>
#include <string>

using namespace MyCommon;

namespace {

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

/// A connected DataFactory ("widget" is just a convenient existing table
/// to run raw SQL against -- this file never goes through WidgetTable)
/// with an empty `widget` table.
DataFactory freshDataFactory()
{
  DataFactory df("widget");
  df.connect(testConnectionParams());
  df.executeQuery("DELETE FROM widget"); // doesn't throw on 0 rows affected
  return df;
}

void test_ConnectSucceedsAgainstRealServer(MiniTest& t)
{
  DataFactory df("widget");
  MT_CHECK(t, df.getConnection() == 0);

  MT_NO_THROW(t, df.connect(testConnectionParams()));
  MT_CHECK(t, df.getConnection() != 0);
}

void test_ExecuteModificationSqlInsertsRealRow(MiniTest& t)
{
  DataFactory df = freshDataFactory();
  MT_NO_THROW(t, df.executeModificationSql(
    "INSERT INTO widget (id, name, category, quantity) VALUES "
    "(1, 'bolt', 'hardware', 10)"));

  MYSQL_RES* result = df.executeQuery("SELECT name, quantity FROM widget WHERE id = 1");
  MT_CHECK(t, result != 0);
  if (result != 0)
  {
    MYSQL_ROW row = mysql_fetch_row(result);
    MT_CHECK(t, row != 0);
    if (row != 0)
    {
      MT_CHECK(t, std::string(row[0]) == "bolt");
      MT_CHECK(t, std::string(row[1]) == "10");
    }
    mysql_free_result(result);
  }
}

void test_ExecuteModificationSqlOnZeroRowsThrowsEntryNotFound(MiniTest& t)
{
  DataFactory df = freshDataFactory();

  bool threw = false;
  try { df.executeModificationSql("UPDATE widget SET quantity = 5 WHERE id = 999"); }
  catch (const EntryNotFound&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_ProcessErrorMapsDuplicateKeyToDuplicateEntry(MiniTest& t)
{
  DataFactory df = freshDataFactory();
  df.executeModificationSql(
    "INSERT INTO widget (id, name, category, quantity) VALUES "
    "(1, 'bolt', 'hardware', 10)");

  bool threw = false;
  try
  {
    df.executeModificationSql(
      "INSERT INTO widget (id, name, category, quantity) VALUES "
      "(1, 'bolt-again', 'hardware', 5)");
  }
  catch (const DuplicateEntry&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_ProcessErrorMapsSyntaxErrorToServiceUnavailable(MiniTest& t)
{
  DataFactory df = freshDataFactory();

  bool threw = false;
  try { df.executeQuery("SELEKT * FROM widget"); } // deliberate typo
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);
}

/// Two DataFactory objects that connect() with the same parameters join
/// the same underlying MySqlConnection rather than opening a second one.
void test_MultipleDataFactoriesShareSameConnection(MiniTest& t)
{
  DataFactory df1("widget");
  df1.connect(testConnectionParams());

  DataFactory df2("widget");
  df2.connect(testConnectionParams());

  MT_CHECK(t, df1.getConnection() != 0);
  MT_CHECK(t, df1.getConnection() == df2.getConnection());
}

void test_TransactionRollbackViaRawSql(MiniTest& t)
{
  DataFactory df = freshDataFactory();
  MySqlConnection* conn = df.getConnection();

  conn->startTransaction();
  df.executeModificationSql(
    "INSERT INTO widget (id, name, category, quantity) VALUES "
    "(1, 'bolt', 'hardware', 10)");
  conn->rollbackTransaction();

  MYSQL_RES* result = df.executeQuery("SELECT COUNT(*) FROM widget WHERE id = 1");
  MT_CHECK(t, result != 0);
  if (result != 0)
  {
    MYSQL_ROW row = mysql_fetch_row(result);
    MT_CHECK(t, row != 0 && std::string(row[0]) == "0"); // rolled back
    mysql_free_result(result);
  }
}

void test_TransactionCommitViaRawSql(MiniTest& t)
{
  DataFactory df = freshDataFactory();
  MySqlConnection* conn = df.getConnection();

  conn->startTransaction();
  df.executeModificationSql(
    "INSERT INTO widget (id, name, category, quantity) VALUES "
    "(1, 'bolt', 'hardware', 10)");
  conn->commitTransaction();

  MYSQL_RES* result = df.executeQuery("SELECT COUNT(*) FROM widget WHERE id = 1");
  MT_CHECK(t, result != 0);
  if (result != 0)
  {
    MYSQL_ROW row = mysql_fetch_row(result);
    MT_CHECK(t, row != 0 && std::string(row[0]) == "1"); // persisted
    mysql_free_result(result);
  }
}

void test_SetDefaultConnectionParamsReallyConnects(MiniTest& t)
{
  DataFactory::setDefaultConnectionParams(testConnectionParams());

  DataFactory df("widget"); // no explicit connect() call
  MT_CHECK(t, df.getConnection() != 0);
  MT_NO_THROW(t, df.executeQuery("DELETE FROM widget"));

  DataFactory::clearDefaultConnectionParams();
}

/// disconnect() marks the shared connection for teardown once every
/// DataFactory using it has released it; a later connect() must then
/// reconnect cleanly rather than reuse the torn-down connection. Run
/// last: it tears down and rebuilds the process-wide shared connection,
/// which every test above (and, transitively, MySqlConnection's
/// reference count) relies on.
void test_DisconnectThenReconnect(MiniTest& t)
{
  {
    DataFactory df("widget");
    df.connect(testConnectionParams());
    MySqlConnection::disconnect(); // no-op on the *connection*, yet: df still holds a reference
  } // df destroyed here: last reference gone -> connection actually torn down

  DataFactory df2("widget");
  MT_NO_THROW(t, df2.connect(testConnectionParams()));
  MT_CHECK(t, df2.getConnection() != 0);
  MT_NO_THROW(t, df2.executeQuery("SELECT 1"));
}

} // namespace

int main()
{
  try
  {
    DataFactory probe("widget");
    probe.connect(testConnectionParams());
  }
  catch (const ServiceUnavailable& e)
  {
    std::cout << "[test_DataFactory_Integration] SKIPPED: no MySQL server "
                 "reachable (" << e.what() << ")" << std::endl;
    return 0;
  }

  MiniTest t("test_DataFactory_Integration");

  MT_RUN(t, test_ConnectSucceedsAgainstRealServer);
  MT_RUN(t, test_ExecuteModificationSqlInsertsRealRow);
  MT_RUN(t, test_ExecuteModificationSqlOnZeroRowsThrowsEntryNotFound);
  MT_RUN(t, test_ProcessErrorMapsDuplicateKeyToDuplicateEntry);
  MT_RUN(t, test_ProcessErrorMapsSyntaxErrorToServiceUnavailable);
  MT_RUN(t, test_MultipleDataFactoriesShareSameConnection);
  MT_RUN(t, test_TransactionRollbackViaRawSql);
  MT_RUN(t, test_TransactionCommitViaRawSql);
  MT_RUN(t, test_SetDefaultConnectionParamsReallyConnects);
  MT_RUN(t, test_DisconnectThenReconnect);

  return t.result();
}
