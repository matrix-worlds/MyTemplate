/* -*- C++ -*- */
//
// test_DataFactory.cpp
//
// Unit tests for MyCommon::DataFactory that don't require a live MySQL
// server: the static nullable-column helpers (pure functions), and the
// behavior of a DataFactory that was constructed but never connect()ed
// (every DB-touching method must fail predictably with
// ServiceUnavailable rather than crash on a null connection).
//
// What is NOT covered here, on purpose (see the project's MySQL section
// of the writeup): connect() actually reaching a server, and
// executeModificationSql()/executeQuery() actually running SQL against
// one. Those paths are real (see MySqlConnection.cpp/DataFactory.h) but
// need a running mysqld this test suite doesn't require.

#include "DataError.h"
#include "DataFactory.h"
#include "MiniTest.h"

using namespace MyCommon;

namespace {

// ------------------------------------------------------------------
// Static nullable-column helpers
// ------------------------------------------------------------------

void test_GetNullableStringColumnValue(MiniTest& t)
{
  MT_CHECK(t, DataFactory::getNullableStringColumnValue("") == "null");
  MT_CHECK(t, DataFactory::getNullableStringColumnValue("abc") == "'abc'");
}

void test_GetNullableStringColumnValueEscapesQuotesAndBackslashes(MiniTest& t)
{
  MT_CHECK(t, DataFactory::getNullableStringColumnValue("O'Brien") == "'O\\'Brien'");
  MT_CHECK(t, DataFactory::getNullableStringColumnValue("a\\b") == "'a\\\\b'");
}

void test_GetNullableIntegerColumnValue(MiniTest& t)
{
  MT_CHECK(t, DataFactory::getNullableIntegerColumnValue(0) == "null");
  MT_CHECK(t, DataFactory::getNullableIntegerColumnValue(42) == "42");
  MT_CHECK(t, DataFactory::getNullableIntegerColumnValue(-7) == "-7");
}

void test_FetchNullableStringColumnValue(MiniTest& t)
{
  MT_CHECK(t, DataFactory::fetchNullableStringColumnValue(0) == "");
  MT_CHECK(t, DataFactory::fetchNullableStringColumnValue("hello") == "hello");
}

void test_FetchNullableIntegerColumnValue(MiniTest& t)
{
  MT_CHECK(t, DataFactory::fetchNullableIntegerColumnValue(0) == 0);
  MT_CHECK(t, DataFactory::fetchNullableIntegerColumnValue("123") == 123);
  MT_CHECK(t, DataFactory::fetchNullableIntegerColumnValue("-5") == -5);
}

void test_MaxLengthConstants(MiniTest& t)
{
  MT_CHECK(t, DataFactory::maxCharLength == 33);
  MT_CHECK(t, DataFactory::maxLongCharLength == 256);
}

// ------------------------------------------------------------------
// Behavior without a live connection
// ------------------------------------------------------------------

void test_ConstructionDoesNotConnect(MiniTest& t)
{
  MT_NO_THROW(t, DataFactory df("widget"));
}

void test_GetTableName(MiniTest& t)
{
  DataFactory df("widget");
  MT_CHECK(t, df.getTableName() == "widget");

  DataFactory defaulted;
  MT_CHECK(t, defaulted.getTableName() == "unknown");
}

void test_ExecuteModificationSqlWithoutConnectionThrows(MiniTest& t)
{
  DataFactory df("widget");
  bool threw = false;
  try { df.executeModificationSql("delete from widget"); }
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_ExecuteQueryWithoutConnectionThrows(MiniTest& t)
{
  DataFactory df("widget");
  bool threw = false;
  try { df.executeQuery("select * from widget"); }
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_ProcessErrorWithoutConnectionThrows(MiniTest& t)
{
  DataFactory df("widget");
  bool threw = false;
  try { df.processError("some location"); }
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);
}

void test_ProcessErrorWithoutConnectionCanBeSuppressed(MiniTest& t)
{
  DataFactory df("widget");
  MT_NO_THROW(t, df.processError("some location", /*throwExceptions=*/false));
}

// ------------------------------------------------------------------
// setDefaultConnectionParams(): the mechanism CacheData.h's reload()
// relies on to get a connected `Table table;` with no chance to call
// connect() itself (see test_CacheData_Integration.cpp). Run last and
// clean up after itself -- this is process-wide state.
// ------------------------------------------------------------------

/// With no default params set (the state every other test above ran
/// in), construction still doesn't connect.
void test_NoDefaultParamsMeansConstructionDoesNotConnect(MiniTest& t)
{
  DataFactory::clearDefaultConnectionParams();
  DataFactory df("widget");
  MT_CHECK(t, df.getConnection() == 0);
}

/// Once default params are set, the constructor itself tries to
/// connect -- observable here because pointing at an unreachable host
/// makes construction throw ServiceUnavailable immediately, rather than
/// leaving a null connection to fail later.
void test_DefaultParamsMakeConstructorAutoConnectAndFailPredictably(MiniTest& t)
{
  MySqlConnectionParams badParams;
  badParams.host = "127.0.0.1";
  badParams.port = 1; // nothing listens here
  badParams.user = "root";
  badParams.database = "mytemplate_test";
  DataFactory::setDefaultConnectionParams(badParams);

  bool threw = false;
  try { DataFactory df("widget"); }
  catch (const ServiceUnavailable&) { threw = true; }
  MT_CHECK(t, threw);

  DataFactory::clearDefaultConnectionParams();
}

/// clearDefaultConnectionParams() restores the original behavior.
void test_ClearDefaultConnectionParamsRestoresManualConnectBehavior(MiniTest& t)
{
  MySqlConnectionParams params;
  params.port = 1;
  DataFactory::setDefaultConnectionParams(params);
  DataFactory::clearDefaultConnectionParams();

  MT_NO_THROW(t, DataFactory df("widget")); // no longer auto-connects
}

} // namespace

int main()
{
  MiniTest t("test_DataFactory");

  MT_RUN(t, test_GetNullableStringColumnValue);
  MT_RUN(t, test_GetNullableStringColumnValueEscapesQuotesAndBackslashes);
  MT_RUN(t, test_GetNullableIntegerColumnValue);
  MT_RUN(t, test_FetchNullableStringColumnValue);
  MT_RUN(t, test_FetchNullableIntegerColumnValue);
  MT_RUN(t, test_MaxLengthConstants);
  MT_RUN(t, test_ConstructionDoesNotConnect);
  MT_RUN(t, test_GetTableName);
  MT_RUN(t, test_ExecuteModificationSqlWithoutConnectionThrows);
  MT_RUN(t, test_ExecuteQueryWithoutConnectionThrows);
  MT_RUN(t, test_ProcessErrorWithoutConnectionThrows);
  MT_RUN(t, test_ProcessErrorWithoutConnectionCanBeSuppressed);
  MT_RUN(t, test_NoDefaultParamsMeansConstructionDoesNotConnect);
  MT_RUN(t, test_DefaultParamsMakeConstructorAutoConnectAndFailPredictably);
  MT_RUN(t, test_ClearDefaultConnectionParamsRestoresManualConnectBehavior);

  return t.result();
}
