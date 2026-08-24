/* -*- C++ -*- */
//
// test_Stringable.cpp
//
// Unit tests for MyCommon::Stringable, the base interface every other
// class in this project (BasicType<>, TSRefCount, RCObject<>, CommonKey*)
// implements. It only has two things to test: a concrete getString()
// override, and the free operator<< that delegates to it -- but neither
// had a dedicated test anywhere, so this file covers both directly with
// a minimal concrete subclass.

#include "MiniTest.h"
#include "Stringable.h"
#include <sstream>

using namespace MyCommon;

namespace {

class Labeled : public Stringable
{
public:
  explicit Labeled(std::string label) : _label(std::move(label)) {}

  virtual std::string getString() const { return "[ Labeled: " + _label + " ]"; }

private:
  std::string _label;
};

void test_ConcreteGetString(MiniTest& t)
{
  Labeled obj("x");
  MT_CHECK(t, obj.getString() == "[ Labeled: x ]");
}

void test_OperatorInsertionDelegatesToGetString(MiniTest& t)
{
  Labeled obj("y");

  std::ostringstream ost;
  ost << obj;
  MT_CHECK(t, ost.str() == obj.getString());
}

void test_PolymorphicUsageThroughBaseReference(MiniTest& t)
{
  Labeled obj("z");
  const Stringable& asBase = obj;

  MT_CHECK(t, asBase.getString() == obj.getString());

  std::ostringstream ost;
  ost << asBase; // operator<< takes a Stringable&, so this is the
                  // polymorphic call path every other class relies on.
  MT_CHECK(t, ost.str() == "[ Labeled: z ]");
}

} // namespace

int main()
{
  MiniTest t("test_Stringable");

  MT_RUN(t, test_ConcreteGetString);
  MT_RUN(t, test_OperatorInsertionDelegatesToGetString);
  MT_RUN(t, test_PolymorphicUsageThroughBaseReference);

  return t.result();
}
