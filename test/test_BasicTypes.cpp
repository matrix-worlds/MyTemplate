/* -*- C++ -*- */
//
// test_BasicTypes.cpp
//

#include "BasicTypes.h"
#include "MiniTest.h"
#include <sstream>
#include <string>

using namespace MyCommon;

namespace {

void test_DefaultAndValueConstruction(MiniTest& t)
{
  BasicType<int32> defaulted;
  MT_CHECK(t, defaulted.getValue() == 0);

  BasicType<int32> five(5);
  MT_CHECK(t, five.getValue() == 5);

  BasicType<std::string> emptyStr;
  MT_CHECK(t, emptyStr.getValue() == "");

  BasicType<std::string> hello(std::string("hello"));
  MT_CHECK(t, hello.getValue() == "hello");
}

void test_GetValue(MiniTest& t)
{
  BasicType<int32> a(7);
  MT_CHECK(t, a.getValue() == 7);

  a.setValue(8);
  MT_CHECK(t, a.getValue() == 8); // reflects the most recent setValue()
}

void test_CopyConstructionAndAssignment(MiniTest& t)
{
  BasicType<u_int32> a(42);
  BasicType<u_int32> b(a);
  MT_CHECK(t, b.getValue() == 42);
  MT_CHECK(t, a == b);

  BasicType<u_int32> c(7);
  MT_NO_THROW(t, c = a);
  MT_CHECK(t, c.getValue() == 42);
  MT_CHECK(t, c == a);

  // Self-assignment must be a no-op, not a corruption.
  MT_NO_THROW(t, c = c);
  MT_CHECK(t, c.getValue() == 42);
}

void test_ComparisonOperators(MiniTest& t)
{
  BasicType<int32> a(1);
  BasicType<int32> b(2);
  BasicType<int32> c(2);

  MT_CHECK(t, a == a);
  MT_CHECK(t, b == c);
  MT_CHECK(t, a != b);
  MT_CHECK(t, a < b);
  MT_CHECK(t, b > a);
  MT_CHECK(t, !(b < c));
  MT_CHECK(t, !(b > c));
}

void test_SetValue(MiniTest& t)
{
  BasicType<int32> a(1);
  MT_NO_THROW(t, a.setValue(99));
  MT_CHECK(t, a.getValue() == 99);
}

void test_GetStringOnIntegerType(MiniTest& t)
{
  BasicType<int32> a(123);
  MT_CHECK(t, a.getString() == "123");

  std::ostringstream ost;
  ost << a; // exercises Stringable::operator<<
  MT_CHECK(t, ost.str() == "123");
}

void test_GetStringOnStringType(MiniTest& t)
{
  BasicType<std::string> a(std::string("abc"));
  MT_CHECK(t, a.getString() == "abc");
}

void test_GetStringOnCharType(MiniTest& t)
{
  BasicType<char> a('x');
  MT_CHECK(t, a.getString() == "x");
}

class SampleId : public BasicType<u_int32>
{
public:
  SampleId(u_int32 a = 0)
    : BasicType<u_int32>(a)
  {}

  SampleId(const SampleId& id)
    : BasicType<u_int32>(id._t)
  {}

  virtual ~SampleId() {}

  virtual std::string getString() const
  {
    return print("SampleId");
  }
};

void test_DerivedClass(MiniTest& t)
{
  SampleId id(7);
  MT_CHECK(t, id.getValue() == 7);
  MT_CHECK(t, id.getString() == " [ SampleId: 7 ] ");

  SampleId same(7);
  SampleId other(8);
  MT_CHECK(t, id == same);
  MT_CHECK(t, id != other);
  MT_CHECK(t, id < other);

  // BasicType<>& reference can hold a SampleId polymorphically.
  const BasicType<u_int32>& asBase = id;
  MT_CHECK(t, asBase.getValue() == 7);
}

} // namespace

int main()
{
  MiniTest t("test_BasicTypes");

  MT_RUN(t, test_DefaultAndValueConstruction);
  MT_RUN(t, test_GetValue);
  MT_RUN(t, test_CopyConstructionAndAssignment);
  MT_RUN(t, test_ComparisonOperators);
  MT_RUN(t, test_SetValue);
  MT_RUN(t, test_GetStringOnIntegerType);
  MT_RUN(t, test_GetStringOnStringType);
  MT_RUN(t, test_GetStringOnCharType);
  MT_RUN(t, test_DerivedClass);

  return t.result();
}
