/* -*- C++ -*- */
//
// MySqlConnection.h
//
// A reference-counted singleton connection every DataFactory instance shares. 
//
// This header/source compiles and links against libmysqlclient; nothing
// in this project's test suite calls connect() against a real server
// (see test/test_MySqlConnection.cpp for exactly what is and isn't
// covered without one).

#ifndef MYTEMPLATE_MYSQLCONNECTION_H
#define MYTEMPLATE_MYSQLCONNECTION_H

#include "BasicTypes.h"
#include <mutex>
#include <string>

#include <mysql.h>

namespace MyCommon {

  /// Connection parameters for a MySQL server. Mirrors the handful of
  /// things the original read from environment variables (data source
  /// name, effectively), but as explicit fields instead.
  struct MySqlConnectionParams
  {
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    unsigned int port = 3306;
  };

  class MySqlConnection
  {
  public:
    /// Returns the current connection, connecting with `params` if none
    /// is open yet, and increments the reference count either way.
    /// Throws ServiceUnavailable if a new connection attempt fails.
    static MySqlConnection* getCurrentInstance(const MySqlConnectionParams& params);

    /// Marks the current connection (if any) for disconnection once its
    /// reference count reaches zero. A no-op if there is no connection.
    static void disconnect();

    /// Called by a DataFactory that is done with this connection.
    void releaseInstance();

    void startTransaction();
    void commitTransaction();
    void rollbackTransaction();
    bool isTransactionActive() const { return _transactionActive; }

    MYSQL* getConnection() const { return _mysql; }

    /// Throws the exception matching the connection's last MySQL error
    /// (see .cpp for the mapping), unless throwExceptions is false, in
    /// which case this is a no-op.
    void processError(const std::string& errorLoc = "",
                       bool throwExceptions = true,
                       const std::string& tableName = "");

    /// Serializes access to the connection: two DataFactory objects
    /// sharing one MYSQL* must not run queries on it concurrently.
    std::mutex& getQueryMutex() { return _queryMutex; }

  protected:
    explicit MySqlConnection(const MySqlConnectionParams& params);
    ~MySqlConnection();

    MySqlConnection(const MySqlConnection&) = delete;
    MySqlConnection& operator=(const MySqlConnection&) = delete;

  private:
    MYSQL* _mysql;
    u_int32 _count;
    bool _destroyWhenIdle;
    bool _transactionActive;
    std::mutex _queryMutex;

    static MySqlConnection* _instance;
    static std::mutex _instanceMutex;
  };

} // namespace MyCommon

#endif // MYTEMPLATE_MYSQLCONNECTION_H
