/* -*- C++ -*- */
//
// DataFactory.h
//
// MySQL's C API doesn't have that shared-cursor problem: mysql_query() +
// mysql_store_result() hands back a self-contained MYSQL_RES* per call,
//
// _tableName was `const std::string&`, a reference bound straight to
// the constructor parameter -- safe only because every real caller
// passed a long-lived static string, but a dangling-reference trap in
// general. It's a plain owned std::string here instead.
//

#ifndef MYTEMPLATE_DATAFACTORY_H
#define MYTEMPLATE_DATAFACTORY_H

#include "DataError.h"
#include "MySqlConnection.h"
#include <cstddef>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

#include <mysql.h>

namespace MyCommon {

  class DataFactory
  {
  public:
    /// If set (see setDefaultConnectionParams() below), every DataFactory
    /// constructed afterwards connects automatically using these params, which
    /// connected implicitly using environment-variable configuration.
    /// Unset by default, so existing "construct without connecting" test
    /// code (test_DataFactory.cpp, test_WidgetTable.cpp) is unaffected;
    /// callers doing explicit connect() calls (test_WidgetTable_Integration.cpp)
    /// don't need it either. It exists for code that constructs a Table
    /// with no chance to call connect() itself -- notably CacheData.h's
    /// *SingleVersionSnapshotCacheData<>::reload(), which does exactly
    /// `Table table;` internally. See test_CacheData_Integration.cpp.
    static void setDefaultConnectionParams(const MySqlConnectionParams& params)
    {
      std::lock_guard<std::mutex> guard(defaultParamsMutex());
      defaultParams() = params;
      haveDefaultParams() = true;
    }

    static void clearDefaultConnectionParams()
    {
      std::lock_guard<std::mutex> guard(defaultParamsMutex());
      haveDefaultParams() = false;
    }

    explicit DataFactory(const std::string& tableName = "unknown")
      : _connection(0), _tableName(tableName)
    {
      std::lock_guard<std::mutex> guard(defaultParamsMutex());
      if (haveDefaultParams())
      {
        _connection = MySqlConnection::getCurrentInstance(defaultParams());
      }
    }

    /// Establishes (or joins) the shared connection. Only needed when no
    /// default connection params are set (see above) -- otherwise the
    /// constructor already did this.
    void connect(const MySqlConnectionParams& params)
    {
      _connection = MySqlConnection::getCurrentInstance(params);
    }

    /// The shared connection, or 0 if connect() hasn't been called.
    /// Exposed so callers can drive transactions
    /// (startTransaction()/commitTransaction()/rollbackTransaction()
    /// live on MySqlConnection, shared by every DataFactory using it).
    MySqlConnection* getConnection() const { return _connection; }

    virtual ~DataFactory()
    {
      if (_connection != 0) _connection->releaseInstance();
    }

    /// Move-only: a copy would share _connection's raw pointer without
    /// bumping its reference count, so both copies' destructors would
    /// each call releaseInstance() for what was only ever one increment
    /// -- a double-release. (A user-declared destructor, which this
    /// class has, suppresses the implicit move members but *not* the
    /// implicit, now-dangerous copy ones, so these have to be spelled
    /// out explicitly.) Moving instead transfers ownership: the moved-
    /// from object is left with a null _connection, safe to destroy.
    DataFactory(const DataFactory&) = delete;
    DataFactory& operator=(const DataFactory&) = delete;

    DataFactory(DataFactory&& other) noexcept
      : _connection(other._connection), _tableName(std::move(other._tableName))
    {
      other._connection = 0;
    }

    DataFactory& operator=(DataFactory&& other) noexcept
    {
      if (this != &other)
      {
        if (_connection != 0) _connection->releaseInstance();
        _connection = other._connection;
        _tableName = std::move(other._tableName);
        other._connection = 0;
      }
      return *this;
    }

    /// Runs an INSERT/UPDATE/DELETE statement. Throws EntryNotFound if
    /// it affected zero rows, or whatever processError() maps the
    /// underlying MySQL error to otherwise.
    void executeModificationSql(const std::string& query)
    {
      requireConnection();
      std::lock_guard<std::mutex> guard(_connection->getQueryMutex());

      if (mysql_real_query(_connection->getConnection(),
                            query.c_str(),
                            static_cast<unsigned long>(query.size())) != 0)
      {
        processError("executeModificationSql: " + query);
        return; // processError() always throws; unreachable in practice
      }

      my_ulonglong rowCount = mysql_affected_rows(_connection->getConnection());
      if (rowCount == 0)
      {
        throw EntryNotFound();
      }
    }

    /// Runs a SELECT and returns its result set. Caller owns the result
    /// (via mysql_free_result()) -- runQuery() below does that for you.
    MYSQL_RES* executeQuery(const std::string& query)
    {
      requireConnection();
      std::lock_guard<std::mutex> guard(_connection->getQueryMutex());

      if (mysql_real_query(_connection->getConnection(),
                            query.c_str(),
                            static_cast<unsigned long>(query.size())) != 0)
      {
        processError("executeQuery: " + query);
      }

      MYSQL_RES* result = mysql_store_result(_connection->getConnection());
      if (result == 0 && mysql_errno(_connection->getConnection()) != 0)
      {
        processError("mysql_store_result: " + query);
      }
      return result;
    }

    void processError(const std::string& errorLoc = "", bool throwExceptions = true)
    {
      if (_connection == 0)
      {
        if (throwExceptions) throw ServiceUnavailable("no connection");
        return;
      }
      _connection->processError(errorLoc, throwExceptions, _tableName);
    }

    const std::string& getTableName() const { return _tableName; }

    // -- Nullable-column convenience helpers
    //    MySQL's MYSQL_ROW represents SQL NULL as a null char*).

    /// Formats a string for use as a column value in an INSERT/UPDATE:
    /// quoted and SQL-escaped, or the literal `null` if empty.
    static std::string getNullableStringColumnValue(const std::string& stringValue)
    {
      if (stringValue.empty()) return "null";

      std::string escaped;
      escaped.reserve(stringValue.size() + 2);
      escaped += '\'';
      for (char c : stringValue)
      {
        if (c == '\'' || c == '\\') escaped += '\\';
        escaped += c;
      }
      escaped += '\'';
      return escaped;
    }

    /// Formats an int for use as a column value: the literal `null` if
    /// zero, otherwise the number.
    static std::string getNullableIntegerColumnValue(int value)
    {
      if (value == 0) return "null";
      std::ostringstream ost;
      ost << value;
      return ost.str();
    }

    /// Converts a fetched column cell to a string; a null cell (SQL
    /// NULL) becomes "".
    static std::string fetchNullableStringColumnValue(const char* cell)
    {
      return cell == 0 ? std::string() : std::string(cell);
    }

    /// Converts a fetched column cell to an int; a null cell (SQL NULL)
    /// becomes 0.
    static int fetchNullableIntegerColumnValue(const char* cell)
    {
      return cell == 0 ? 0 : std::atoi(cell);
    }

    static const std::size_t maxCharLength = 33;
    static const std::size_t maxLongCharLength = 256;

  private:
    void requireConnection()
    {
      if (_connection == 0) throw ServiceUnavailable("DataFactory not connected");
    }

    // Function-local statics instead of plain static data members, so
    // this class stays header-only (a plain static member would need an
    // out-of-line definition in some .cpp, since DataFactory isn't a
    // template -- unlike ObjectPool<>/CacheData<>'s per-instantiation
    // statics elsewhere in this project).
    static bool& haveDefaultParams() { static bool b = false; return b; }
    static MySqlConnectionParams& defaultParams() { static MySqlConnectionParams p; return p; }
    static std::mutex& defaultParamsMutex() { static std::mutex m; return m; }

    MySqlConnection* _connection;
    std::string _tableName;
  };

  /**
   * RowMapper
   *
   * Converts one fetched MYSQL_ROW into an Entry. This is what a
   * concrete table implements per-schema, 
   *
   * (runQuery(), which drives a SELECT through a RowMapper into a
   * ResultsProcessor, lives in Data.h alongside ResultsProcessor<>
   * rather than here, to avoid a header cycle between the two.)
   */
  template<class Entry>
  class RowMapper
  {
  public:
    virtual ~RowMapper() {}
    virtual Entry mapRow(MYSQL_ROW row, unsigned long* lengths) const = 0;
  };

} // namespace MyCommon

#endif // MYTEMPLATE_DATAFACTORY_H
