/* -*- C++ -*- */
//
// BasicTypes.h
//

#ifndef MYTEMPLATE_BASICTYPES_H
#define MYTEMPLATE_BASICTYPES_H

#include "Stringable.h"
#include <sstream>
#include <string>

namespace MyCommon {

  typedef unsigned long long u_int64;  ///< Unsigned 64 bit integer.
  typedef long long          int64;    ///< Signed 64 bit integer.

  typedef unsigned int       u_int32;  ///< Unsigned 32 bit integer.
  typedef int                int32;    ///< Signed 32 bit integer.

  typedef unsigned short     u_int16;  ///< Unsigned 16 bit integer.
  typedef short              int16;    ///< Signed 16 bit integer.

  typedef unsigned char      u_int8;   ///< Unsigned 8 bit integer.
  typedef char               int8;     ///< Signed 8 bit integer.

  /**
   * BasicType
   *
   * @brief Wrapper template for simple identifier/value types.
   *
   * Provides the generic operations a value wrapper needs:
   *  1. Comparison operators: ==, !=, > and <
   *  2. Accessors: getValue(), setValue()
   *  3. getString(), implementing the Stringable interface as a plain
   *     textual representation of the wrapped value.
   *
   * A type deriving from BasicType<> is expected to supply:
   *  1. A constructor with a default initialization value.
   *  2. A copy constructor (a template can't match a derived class's own
   *     copy constructor).
   *  3. A virtual destructor.
   *  4. An override of getString() giving a readable, type-named string
   *     typically via the protected print() helper.
   */
  template<class Type>
  class BasicType : public Stringable
  {
  public:

    /// Default constructor; wraps a default-constructed Type.
    BasicType()
      : _t(Type())
    {}

    /// Construct from a Type value.
    explicit BasicType(const Type& t)
      : _t(t)
    {}

    /// Copy constructor.
    BasicType(const BasicType& bt)
      : _t(bt._t)
    {}

    /// Destructor.
    virtual ~BasicType() {}

    /// Assignment operator.
    const BasicType& operator=(const BasicType& t)
    {
      if (this != &t)
      {
        _t = t._t;
      }
      return *this;
    }

    /// Equality operator.
    bool operator==(const BasicType& t) const
    { return _t == t._t; }

    /// Non-equality operator.
    bool operator!=(const BasicType& t) const
    { return _t != t._t; }

    /// Less-than operator.
    bool operator<(const BasicType& t) const
    { return _t < t._t; }

    /// Greater-than operator.
    bool operator>(const BasicType& t) const
    { return _t > t._t; }

    /// Set the wrapped value.
    void setValue(const Type& t)
    { _t = t; }

    /// Get the wrapped value.
    const Type& getValue() const
    { return _t; }

    /// Plain string form of the wrapped value.
    virtual std::string getString() const;

  protected:
    Type _t;

    /// Helper for derived classes' getString(): renders as
    /// " [ <typeName>: <value> ] ".
    std::string print(const std::string& typeName) const;
  };

  template<class Type>
  std::string BasicType<Type>::getString() const
  {
    std::ostringstream ost;
    ost << _t;
    return ost.str();
  }

  template<class Type>
  std::string BasicType<Type>::print(const std::string& typeName) const
  {
    std::ostringstream ost;
    ost << " [ "
        << typeName
        << ": "
        << _t
        << " ] ";
    return ost.str();
  }

} // namespace MyCommon

#endif // MYTEMPLATE_BASICTYPES_H
