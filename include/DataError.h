/* -*- C++ -*- */
//
// DataError.h
//

#ifndef MYTEMPLATE_DATAERROR_H
#define MYTEMPLATE_DATAERROR_H

#include <stdexcept>
#include <string>

namespace MyCommon {

  /// Raised when a request is made of the database and it is unavailable
  /// (connection failure, query execution failure, etc).
  class ServiceUnavailable : public std::runtime_error
  {
  public:
    ServiceUnavailable() : runtime_error("service unavailable") {}
    explicit ServiceUnavailable(const std::string& msg) : runtime_error(msg) {}
  };

  /// Raised by insert() when a row with the same key already exists.
  class DuplicateEntry : public std::runtime_error
  {
  public:
    DuplicateEntry() : runtime_error("duplicate entry") {}
    explicit DuplicateEntry(const std::string& msg) : runtime_error(msg) {}
  };

  /// Raised by update()/remove()/executeModificationSql() when the
  /// targeted row does not exist.
  class EntryNotFound : public std::runtime_error
  {
  public:
    EntryNotFound() : runtime_error("entry not found") {}
    explicit EntryNotFound(const std::string& msg) : runtime_error(msg) {}
  };

  /// Raised when a modification would violate a foreign-key relationship.
  class DependencyViolation : public std::runtime_error
  {
  public:
    DependencyViolation() : runtime_error("dependency violation") {}
    explicit DependencyViolation(const std::string& msg) : runtime_error(msg) {}
  };

} // namespace MyCommon

#endif // MYTEMPLATE_DATAERROR_H
