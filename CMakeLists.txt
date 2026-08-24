cmake_minimum_required(VERSION 3.10)
project(MyTemplate CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

# --- MySQL client library (headers + libmysqlclient) -----------------
# Installed via `brew install mysql-client` (keg-only; not symlinked
# into the default Homebrew prefix). Falls back to /usr for Linux
# distros that package it there.
find_path(MYSQL_INCLUDE_DIR
  NAMES mysql.h
  PATH_SUFFIXES mysql
  HINTS /opt/homebrew/opt/mysql-client/include /opt/homebrew/opt/mysql/include
        /usr/local/opt/mysql-client/include /usr/local/opt/mysql/include
  PATHS /usr/include /usr/local/include
)
find_library(MYSQL_LIBRARY
  NAMES mysqlclient
  HINTS /opt/homebrew/opt/mysql-client/lib /opt/homebrew/opt/mysql/lib
        /usr/local/opt/mysql-client/lib /usr/local/opt/mysql/lib
  PATHS /usr/lib /usr/local/lib
)
if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_LIBRARY)
  message(FATAL_ERROR
    "MySQL client library not found. Install it with "
    "'brew install mysql-client' (macOS) or your distro's libmysqlclient-dev "
    "package, or point MYSQL_INCLUDE_DIR/MYSQL_LIBRARY at it manually.")
endif()
message(STATUS "MySQL include dir: ${MYSQL_INCLUDE_DIR}")
message(STATUS "MySQL library: ${MYSQL_LIBRARY}")

add_library(basictypes
  src/Stringable.cpp
  src/Dump.cpp
  src/MySqlConnection.cpp
  src/WidgetTable.cpp
)
target_include_directories(basictypes PUBLIC ${MYSQL_INCLUDE_DIR})
target_link_libraries(basictypes Threads::Threads ${MYSQL_LIBRARY})

enable_testing()

add_executable(test_Stringable test/test_Stringable.cpp)
target_link_libraries(test_Stringable basictypes)
add_test(NAME StringableTest COMMAND test_Stringable)

add_executable(test_BasicTypes test/test_BasicTypes.cpp)
target_link_libraries(test_BasicTypes basictypes)
add_test(NAME BasicTypesTest COMMAND test_BasicTypes)

add_executable(test_ObjectPool test/test_ObjectPool.cpp)
target_link_libraries(test_ObjectPool basictypes)
add_test(NAME ObjectPoolTest COMMAND test_ObjectPool)

add_executable(test_RefCount test/test_RefCount.cpp)
target_link_libraries(test_RefCount basictypes)
add_test(NAME RefCountTest COMMAND test_RefCount)

add_executable(test_Dump test/test_Dump.cpp)
target_link_libraries(test_Dump basictypes)
add_test(NAME DumpTest COMMAND test_Dump)

add_executable(test_CommonMap test/test_CommonMap.cpp)
target_link_libraries(test_CommonMap basictypes)
add_test(NAME CommonMapTest COMMAND test_CommonMap)

add_executable(test_CommonKey test/test_CommonKey.cpp)
target_link_libraries(test_CommonKey basictypes)
add_test(NAME CommonKeyTest COMMAND test_CommonKey)

add_executable(test_Data test/test_Data.cpp)
target_link_libraries(test_Data basictypes)
add_test(NAME DataTest COMMAND test_Data)

add_executable(test_DataFactory test/test_DataFactory.cpp)
target_link_libraries(test_DataFactory basictypes)
add_test(NAME DataFactoryTest COMMAND test_DataFactory)

# Requires a real, reachable MySQL server; skips itself cleanly (exit 0)
# if there isn't one, same as the other *_Integration tests.
add_executable(test_DataFactory_Integration test/test_DataFactory_Integration.cpp)
target_link_libraries(test_DataFactory_Integration basictypes)
add_test(NAME DataFactoryIntegrationTest COMMAND test_DataFactory_Integration)

add_executable(test_WidgetTable test/test_WidgetTable.cpp)
target_link_libraries(test_WidgetTable basictypes)
add_test(NAME WidgetTableTest COMMAND test_WidgetTable)

add_executable(test_CacheData test/test_CacheData.cpp)
target_link_libraries(test_CacheData basictypes)
add_test(NAME CacheDataTest COMMAND test_CacheData)

# Requires a real, reachable MySQL server (see the file's header comment
# for how to point it at one); skips itself cleanly (exit 0) if there
# isn't one, so it's safe to always build and always run under ctest.
add_executable(test_WidgetTable_Integration test/test_WidgetTable_Integration.cpp)
target_link_libraries(test_WidgetTable_Integration basictypes)
add_test(NAME WidgetTableIntegrationTest COMMAND test_WidgetTable_Integration)

# Same reachable-server requirement/skip behavior as
# test_WidgetTable_Integration above.
add_executable(test_CacheData_Integration test/test_CacheData_Integration.cpp)
target_link_libraries(test_CacheData_Integration basictypes)
add_test(NAME CacheDataIntegrationTest COMMAND test_CacheData_Integration)
