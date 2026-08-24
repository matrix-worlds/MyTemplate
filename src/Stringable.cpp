/* -*- C++ -*- */
//
// Stringable.cpp
//

#include "Stringable.h"

namespace MyCommon {

  std::ostream& operator<<(std::ostream& ost, const Stringable& obj)
  {
    ost << obj.getString();
    return ost;
  }

} // namespace MyCommon
