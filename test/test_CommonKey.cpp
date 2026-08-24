/* -*- C++ -*- */
//
// test_CommonKey.cpp
//
// Unit tests for MyCommon::CommonKey1..CommonKey6. Each class gets its
// own Construction / Comparison / Accessors / GetString cases so every
// constructor, operator, and get/set pair is exercised on its own, not
// just incidentally as a side effect of some other check.

#include "CommonKey.h"
#include "MiniTest.h"
#include <string>

using namespace MyCommon;

namespace {

// ------------------------------------------------------------------
// CommonKey1<int>
// ------------------------------------------------------------------

void test_CommonKey1_Construction(MiniTest& t)
{
  typedef CommonKey1<int> Key1;

  Key1 defaulted;
  MT_CHECK(t, defaulted.getA() == 0);

  Key1 a(1);
  MT_CHECK(t, a.getA() == 1);

  Key1 copy(a);
  MT_CHECK(t, copy.getA() == 1);
  MT_CHECK(t, copy == a);

  Key1 assigned;
  MT_NO_THROW(t, assigned = a);
  MT_CHECK(t, assigned == a);

  // Self-assignment must be a safe no-op.
  MT_NO_THROW(t, assigned = assigned);
  MT_CHECK(t, assigned.getA() == 1);
}

void test_CommonKey1_Comparison(MiniTest& t)
{
  typedef CommonKey1<int> Key1;

  Key1 a(1);
  Key1 b(2);
  Key1 c(2);

  MT_CHECK(t, a != b);
  MT_CHECK(t, b == c);
  MT_CHECK(t, a < b);
  MT_CHECK(t, b > a);
  MT_CHECK(t, !(b < c));
  MT_CHECK(t, !(b > c));
}

void test_CommonKey1_Accessors(MiniTest& t)
{
  typedef CommonKey1<int> Key1;

  Key1 a(1);
  MT_CHECK(t, a.getA() == 1);

  a.setA(9);
  MT_CHECK(t, a.getA() == 9);
}

void test_CommonKey1_GetString(MiniTest& t)
{
  CommonKey1<int> a(1);
  MT_CHECK(t, a.getString() == "[ CommonKey1: [ A: 1 ] ] ");
}

// ------------------------------------------------------------------
// CommonKey2<int, std::string>
// ------------------------------------------------------------------

void test_CommonKey2_Construction(MiniTest& t)
{
  typedef CommonKey2<int, std::string> Key2;

  Key2 defaulted;
  MT_CHECK(t, defaulted.getA() == 0);
  MT_CHECK(t, defaulted.getB() == "");

  Key2 a(1, "x");
  MT_CHECK(t, a.getA() == 1);
  MT_CHECK(t, a.getB() == "x");

  Key2 copy(a);
  MT_CHECK(t, copy == a);

  Key2 assigned;
  MT_NO_THROW(t, assigned = a);
  MT_CHECK(t, assigned == a);
}

void test_CommonKey2_Comparison(MiniTest& t)
{
  typedef CommonKey2<int, std::string> Key2;

  Key2 a(1, "x");
  Key2 b(1, "y"); // same A, differ in B
  Key2 c(2, "a"); // A dominates comparison over B

  MT_CHECK(t, a == Key2(1, "x"));
  MT_CHECK(t, a != b);
  MT_CHECK(t, a < b);   // same A, B "x" < "y"
  MT_CHECK(t, b < c);   // A 1 < 2, regardless of B
  MT_CHECK(t, c > a);
}

void test_CommonKey2_Accessors(MiniTest& t)
{
  typedef CommonKey2<int, std::string> Key2;

  Key2 a(1, "x");
  MT_CHECK(t, a.getA() == 1);
  MT_CHECK(t, a.getB() == "x");

  a.setA(2);
  MT_CHECK(t, a.getA() == 2);

  a.setB("z");
  MT_CHECK(t, a.getB() == "z");
}

void test_CommonKey2_GetString(MiniTest& t)
{
  CommonKey2<int, std::string> a(1, "z");
  MT_CHECK(t, a.getString() == "[ CommonKey2: [ A: 1 , B: z ] ] ");
}

// ------------------------------------------------------------------
// CommonKey3<int, int, int>
// ------------------------------------------------------------------

void test_CommonKey3_Construction(MiniTest& t)
{
  typedef CommonKey3<int, int, int> Key3;

  Key3 defaulted;
  MT_CHECK(t, defaulted.getA() == 0 && defaulted.getB() == 0 && defaulted.getC() == 0);

  Key3 a(1, 2, 3);
  MT_CHECK(t, a.getA() == 1 && a.getB() == 2 && a.getC() == 3);

  Key3 copy(a);
  MT_CHECK(t, copy == a);

  Key3 assigned;
  MT_NO_THROW(t, assigned = a);
  MT_CHECK(t, assigned == a);
}

void test_CommonKey3_Comparison(MiniTest& t)
{
  typedef CommonKey3<int, int, int> Key3;

  Key3 a(1, 2, 3);
  Key3 b(1, 2, 4); // differ in C
  Key3 c(1, 3, 0); // differ in B

  MT_CHECK(t, a == Key3(1, 2, 3));
  MT_CHECK(t, a != b);
  MT_CHECK(t, a < b);
  MT_CHECK(t, b < c);
  MT_CHECK(t, c > b);
}

void test_CommonKey3_Accessors(MiniTest& t)
{
  typedef CommonKey3<int, int, int> Key3;

  Key3 a(1, 2, 3);
  MT_CHECK(t, a.getA() == 1);
  MT_CHECK(t, a.getB() == 2);
  MT_CHECK(t, a.getC() == 3);

  a.setA(10); MT_CHECK(t, a.getA() == 10);
  a.setB(20); MT_CHECK(t, a.getB() == 20);
  a.setC(30); MT_CHECK(t, a.getC() == 30);
}

void test_CommonKey3_GetString(MiniTest& t)
{
  CommonKey3<int, int, int> a(1, 2, 3);
  MT_CHECK(t, a.getString() == "[ CommonKey3: [ A: 1 , B: 2 , C: 3 ] ] ");
}

// ------------------------------------------------------------------
// CommonKey4<int, int, int, int>
// ------------------------------------------------------------------

void test_CommonKey4_Construction(MiniTest& t)
{
  typedef CommonKey4<int, int, int, int> Key4;

  Key4 defaulted;
  MT_CHECK(t, defaulted.getA() == 0 && defaulted.getD() == 0);

  Key4 a(1, 2, 3, 4);
  MT_CHECK(t, a.getA() == 1 && a.getB() == 2 && a.getC() == 3 && a.getD() == 4);

  Key4 copy(a);
  MT_CHECK(t, copy == a);

  Key4 assigned;
  MT_NO_THROW(t, assigned = a);
  MT_CHECK(t, assigned == a);
}

void test_CommonKey4_Comparison(MiniTest& t)
{
  typedef CommonKey4<int, int, int, int> Key4;

  Key4 a(1, 1, 1, 1);
  Key4 b(1, 1, 1, 2); // differ in D

  MT_CHECK(t, a == Key4(1, 1, 1, 1));
  MT_CHECK(t, a != b);
  MT_CHECK(t, a < b);
  MT_CHECK(t, b > a);
}

void test_CommonKey4_Accessors(MiniTest& t)
{
  typedef CommonKey4<int, int, int, int> Key4;

  Key4 a(1, 2, 3, 4);
  MT_CHECK(t, a.getA() == 1);
  MT_CHECK(t, a.getB() == 2);
  MT_CHECK(t, a.getC() == 3);
  MT_CHECK(t, a.getD() == 4);

  a.setA(10); MT_CHECK(t, a.getA() == 10);
  a.setB(20); MT_CHECK(t, a.getB() == 20);
  a.setC(30); MT_CHECK(t, a.getC() == 30);
  a.setD(40); MT_CHECK(t, a.getD() == 40);
}

void test_CommonKey4_GetString(MiniTest& t)
{
  CommonKey4<int, int, int, int> a(1, 2, 3, 4);
  MT_CHECK(t, a.getString() == "[ CommonKey4: [ A: 1 , B: 2 , C: 3 , D: 4 ] ] ");
}

// ------------------------------------------------------------------
// CommonKey5<int, int, int, int, int>
// ------------------------------------------------------------------

void test_CommonKey5_Construction(MiniTest& t)
{
  typedef CommonKey5<int, int, int, int, int> Key5;

  Key5 defaulted;
  MT_CHECK(t, defaulted.getA() == 0 && defaulted.getE() == 0);

  Key5 a(1, 2, 3, 4, 5);
  MT_CHECK(t, a.getA() == 1 && a.getB() == 2 && a.getC() == 3 &&
              a.getD() == 4 && a.getE() == 5);

  Key5 copy(a);
  MT_CHECK(t, copy == a);

  Key5 assigned;
  MT_NO_THROW(t, assigned = a);
  MT_CHECK(t, assigned == a);
}

void test_CommonKey5_Comparison(MiniTest& t)
{
  typedef CommonKey5<int, int, int, int, int> Key5;

  Key5 a(1, 1, 1, 1, 1);
  Key5 b(1, 1, 1, 1, 2); // differ in E

  MT_CHECK(t, a == Key5(1, 1, 1, 1, 1));
  MT_CHECK(t, a != b);
  MT_CHECK(t, a < b);
  MT_CHECK(t, b > a);
}

void test_CommonKey5_Accessors(MiniTest& t)
{
  typedef CommonKey5<int, int, int, int, int> Key5;

  Key5 a(1, 2, 3, 4, 5);
  MT_CHECK(t, a.getA() == 1);
  MT_CHECK(t, a.getB() == 2);
  MT_CHECK(t, a.getC() == 3);
  MT_CHECK(t, a.getD() == 4);
  MT_CHECK(t, a.getE() == 5);

  a.setA(10); MT_CHECK(t, a.getA() == 10);
  a.setB(20); MT_CHECK(t, a.getB() == 20);
  a.setC(30); MT_CHECK(t, a.getC() == 30);
  a.setD(40); MT_CHECK(t, a.getD() == 40);
  a.setE(50); MT_CHECK(t, a.getE() == 50);
}

void test_CommonKey5_GetString(MiniTest& t)
{
  CommonKey5<int, int, int, int, int> a(1, 2, 3, 4, 5);
  MT_CHECK(t, a.getString() ==
              "[ CommonKey5: [ A: 1 , B: 2 , C: 3 , D: 4 , E: 5 ] ] ");
}

// ------------------------------------------------------------------
// CommonKey6<int, int, int, int, int, int>
// ------------------------------------------------------------------

void test_CommonKey6_Construction(MiniTest& t)
{
  typedef CommonKey6<int, int, int, int, int, int> Key6;

  Key6 defaulted;
  MT_CHECK(t, defaulted.getA() == 0 && defaulted.getF() == 0);

  Key6 a(1, 2, 3, 4, 5, 6);
  MT_CHECK(t, a.getA() == 1 && a.getB() == 2 && a.getC() == 3 &&
              a.getD() == 4 && a.getE() == 5 && a.getF() == 6);

  Key6 copy(a);
  MT_CHECK(t, copy == a);

  Key6 assigned;
  MT_NO_THROW(t, assigned = a);
  MT_CHECK(t, assigned == a);
}

void test_CommonKey6_Comparison(MiniTest& t)
{
  typedef CommonKey6<int, int, int, int, int, int> Key6;

  Key6 a(1, 1, 1, 1, 1, 1);
  Key6 b(1, 1, 1, 1, 1, 2); // differ only in F

  MT_CHECK(t, a == Key6(1, 1, 1, 1, 1, 1));
  MT_CHECK(t, a != b);
  MT_CHECK(t, a < b);
  MT_CHECK(t, b > a);
}

void test_CommonKey6_Accessors(MiniTest& t)
{
  typedef CommonKey6<int, int, int, int, int, int> Key6;

  Key6 a(1, 2, 3, 4, 5, 6);
  MT_CHECK(t, a.getA() == 1);
  MT_CHECK(t, a.getB() == 2);
  MT_CHECK(t, a.getC() == 3);
  MT_CHECK(t, a.getD() == 4);
  MT_CHECK(t, a.getE() == 5);
  MT_CHECK(t, a.getF() == 6);

  a.setA(10); MT_CHECK(t, a.getA() == 10);
  a.setB(20); MT_CHECK(t, a.getB() == 20);
  a.setC(30); MT_CHECK(t, a.getC() == 30);
  a.setD(40); MT_CHECK(t, a.getD() == 40);
  a.setE(50); MT_CHECK(t, a.getE() == 50);

  a.setF(7); // the fixed accessor (was misnamed setE() in the original)
  MT_CHECK(t, a.getF() == 7);
  MT_CHECK(t, a.getE() == 50); // setF() must not disturb E
}

void test_CommonKey6_GetString(MiniTest& t)
{
  CommonKey6<int, int, int, int, int, int> a(1, 1, 1, 1, 1, 7);
  MT_CHECK(t, a.getString() ==
              "[ CommonKey6: [ A: 1 , B: 1 , C: 1 , D: 1 , E: 1 , F: 7 ] ] ");
}

/// A CommonKey1<> is-a Stringable: operator<< should work polymorphically.
void test_PolymorphicStringable(MiniTest& t)
{
  CommonKey1<int> key(5);
  const CommonKey& asBase = key;
  MT_CHECK(t, asBase.getString() == key.getString());
}

} // namespace

int main()
{
  MiniTest t("test_CommonKey");

  MT_RUN(t, test_CommonKey1_Construction);
  MT_RUN(t, test_CommonKey1_Comparison);
  MT_RUN(t, test_CommonKey1_Accessors);
  MT_RUN(t, test_CommonKey1_GetString);

  MT_RUN(t, test_CommonKey2_Construction);
  MT_RUN(t, test_CommonKey2_Comparison);
  MT_RUN(t, test_CommonKey2_Accessors);
  MT_RUN(t, test_CommonKey2_GetString);

  MT_RUN(t, test_CommonKey3_Construction);
  MT_RUN(t, test_CommonKey3_Comparison);
  MT_RUN(t, test_CommonKey3_Accessors);
  MT_RUN(t, test_CommonKey3_GetString);

  MT_RUN(t, test_CommonKey4_Construction);
  MT_RUN(t, test_CommonKey4_Comparison);
  MT_RUN(t, test_CommonKey4_Accessors);
  MT_RUN(t, test_CommonKey4_GetString);

  MT_RUN(t, test_CommonKey5_Construction);
  MT_RUN(t, test_CommonKey5_Comparison);
  MT_RUN(t, test_CommonKey5_Accessors);
  MT_RUN(t, test_CommonKey5_GetString);

  MT_RUN(t, test_CommonKey6_Construction);
  MT_RUN(t, test_CommonKey6_Comparison);
  MT_RUN(t, test_CommonKey6_Accessors);
  MT_RUN(t, test_CommonKey6_GetString);

  MT_RUN(t, test_PolymorphicStringable);

  return t.result();
}
