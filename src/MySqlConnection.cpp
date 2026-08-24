/* -*- C++ -*- */
//
// MySqlConnection.cpp
//

#include "MySqlConnection.h"
#include "DataError.h"
#include <sstream>

#include <mysqld_error.h> // ER_* server error codes (ER_DUP_ENTRY etc.)

namespace MyCommon {

MySqlConnection* MySqlConnection::_instance = 0;
std::mutex MySqlConnection::_instanceMutex;

MySqlConnection::MySqlConnection(const MySqlConnectionParams& params)
  : _mysql(0), _count(0), _destroyWhenIdle(false), _transactionActive(false)
{
  _mysql = mysql_init(0);
  if (_mysql == 0)
  {
    throw ServiceUnavailable("mysql_init() failed");
  }

  if (mysql_real_connect(_mysql,
                          params.host.c_str(),
                          params.user.c_str(),
                          params.password.c_str(),
                          params.database.c_str(),
                          params.port,
                          0,   // unix socket: use TCP per host/port above
                          0) == 0)
  {
    std::ostringstream ost;
    ost << "mysql_real_connect() to " << params.host << ":" << params.port
        << " failed: " << mysql_error(_mysql);
    mysql_close(_mysql);
    _mysql = 0;
    throw ServiceUnavailable(ost.str());
  }

  // Modification statements (INSERT/UPDATE/DELETE) commit immediately
  // unless a caller explicitly starts a transaction, matching the
  // original's ODBC autocommit default.
  mysql_autocommit(_mysql, 1);
}

MySqlConnection::~MySqlConnection()
{
  if (_mysql != 0)
  {
    mysql_close(_mysql);
  }
}

MySqlConnection*
MySqlConnection::getCurrentInstance(const MySqlConnectionParams& params)
{
  std::lock_guard<std::mutex> guard(_instanceMutex);

  if (_instance == 0)
  {
    _instance = new MySqlConnection(params);
  }
  ++_instance->_count;
  return _instance;
}

void
MySqlConnection::disconnect()
{
  std::lock_guard<std::mutex> guard(_instanceMutex);
  if (_instance != 0)
  {
    _instance->_destroyWhenIdle = true;
    if (_instance->_count == 0)
    {
      delete _instance;
      _instance = 0;
    }
  }
}

void
MySqlConnection::releaseInstance()
{
  std::lock_guard<std::mutex> guard(_instanceMutex);
  --_count;
  if (_count == 0 && _destroyWhenIdle && this == _instance)
  {
    _instance = 0;
    delete this;
  }
}

void
MySqlConnection::startTransaction()
{
  if (_mysql == 0) throw ServiceUnavailable();
  mysql_autocommit(_mysql, 0);
  _transactionActive = true;
}

void
MySqlConnection::commitTransaction()
{
  if (!_transactionActive) return;
  if (_mysql != 0) mysql_commit(_mysql);
  _transactionActive = false;
  if (_mysql != 0) mysql_autocommit(_mysql, 1);
}

void
MySqlConnection::rollbackTransaction()
{
  if (!_transactionActive) return;
  if (_mysql != 0) mysql_rollback(_mysql);
  _transactionActive = false;
  if (_mysql != 0) mysql_autocommit(_mysql, 1);
}

void
MySqlConnection::processError(const std::string& errorLoc,
                               bool throwExceptions,
                               const std::string& tableName)
{
  if (_mysql == 0)
  {
    if (throwExceptions) throw ServiceUnavailable("no connection");
    return;
  }

  unsigned int errNo = mysql_errno(_mysql);
  if (errNo == 0) return; // nothing to report

  std::ostringstream ost;
  ost << (tableName.empty() ? std::string("") : tableName + ": ")
      << errorLoc << ": " << mysql_error(_mysql) << " (errno " << errNo << ")";

  if (!throwExceptions) return;

  switch (errNo)
  {
    case ER_DUP_ENTRY:
      throw DuplicateEntry(ost.str());

    case ER_ROW_IS_REFERENCED:
    case ER_ROW_IS_REFERENCED_2:
    case ER_NO_REFERENCED_ROW:
    case ER_NO_REFERENCED_ROW_2:
      throw DependencyViolation(ost.str());

    case CR_CONNECTION_ERROR:
    case CR_CONN_HOST_ERROR:
    case CR_SERVER_GONE_ERROR:
    case CR_SERVER_LOST:
      throw ServiceUnavailable(ost.str());

    default:
      throw ServiceUnavailable(ost.str());
  }
}

} // namespace MyCommon
