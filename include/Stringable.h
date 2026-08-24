/* -*- C++ -*- */
//
// Stringable.h
//

#ifndef MYTEMPLATE_STRINGABLE_H
#define MYTEMPLATE_STRINGABLE_H

#include <iostream>
#include <string>

namespace MyCommon {

  /// Abstract interface for objects that can render themselves as a string.
  /// Implementers must provide getString(); operator<< is then available
  /// for free via the free function below.
  class Stringable {
  public:
    virtual std::string getString() const = 0;
  };

  std::ostream& operator<<(std::ostream&, const Stringable&);

} // namespace MyCommon

#endif // MYTEMPLATE_STRINGABLE_H
