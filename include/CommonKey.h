/* -*- C++ -*- */
//
// CommonKey.h
//

#ifndef MYTEMPLATE_COMMONKEY_H
#define MYTEMPLATE_COMMONKEY_H

#include "Stringable.h"
#include <sstream>
#include <string>

namespace MyCommon {

  typedef Stringable CommonKey;

  /**
   * CommonKey1
   *
   * Key type with one component. A subclass should add: a default
   * constructor, a constructor initializing the component, get/set
   * accessors as needed, and a getString() override.
   */
  template<class A>
  class CommonKey1 : public CommonKey
  {
  public:

    CommonKey1()
      : _a(A())
    {}

    CommonKey1(const A& a)
      : _a(a)
    {}

    CommonKey1(const CommonKey1& k)
      : _a(k._a)
    {}

    virtual ~CommonKey1() {}

    const CommonKey1& operator=(const CommonKey1& k)
    {
      if (this != &k)
      {
        _a = k._a;
      }
      return *this;
    }

    bool operator==(const CommonKey1& k) const
    { return (_a == k._a); }

    bool operator!=(const CommonKey1& k) const
    { return (_a != k._a); }

    bool operator<(const CommonKey1& k) const
    { return (_a < k._a); }

    bool operator>(const CommonKey1& k) const
    { return (_a > k._a); }

    const A& getA() const { return _a; }
    void setA(const A& a) { _a = a; }

    /// Subclasses should override this to give a readable, type-named
    /// string, typically via a protected print() helper (see BasicType).
    virtual std::string getString() const;

  protected:
    A _a;
  };

  template<class A>
  std::string CommonKey1<A>::getString() const
  {
    std::ostringstream ost;
    ost << "[ CommonKey1: [ A: " << _a << " ] ] ";
    return ost.str();
  }

  /**
   * CommonKey2
   *
   * Key type with two components, compared lexicographically (A first,
   * then B). See CommonKey1 for the subclassing contract.
   */
  template<class A, class B>
  class CommonKey2 : public CommonKey
  {
  public:

    CommonKey2()
      : _a(A()), _b(B())
    {}

    CommonKey2(const A& a, const B& b)
      : _a(a), _b(b)
    {}

    CommonKey2(const CommonKey2& k)
      : _a(k._a), _b(k._b)
    {}

    virtual ~CommonKey2() {}

    const CommonKey2& operator=(const CommonKey2& k)
    {
      if (this != &k)
      {
        _a = k._a;
        _b = k._b;
      }
      return *this;
    }

    bool operator==(const CommonKey2& k) const
    { return ((_a == k._a) && (_b == k._b)); }

    bool operator!=(const CommonKey2& k) const
    { return ((_a != k._a) || (_b != k._b)); }

    bool operator<(const CommonKey2& k) const
    {
      return ((_a < k._a) ||
              ((_a == k._a) && (_b < k._b)));
    }

    bool operator>(const CommonKey2& k) const
    {
      return ((_a > k._a) ||
              ((_a == k._a) && (_b > k._b)));
    }

    const A& getA() const { return _a; }
    void setA(const A& a) { _a = a; }

    const B& getB() const { return _b; }
    void setB(const B& b) { _b = b; }

    virtual std::string getString() const;

  protected:
    A _a;
    B _b;
  };

  template<class A, class B>
  std::string CommonKey2<A, B>::getString() const
  {
    std::ostringstream ost;
    ost << "[ CommonKey2: [ A: " << _a << " , B: " << _b << " ] ] ";
    return ost.str();
  }

  /**
   * CommonKey3
   *
   * Key type with three components, compared lexicographically
   * (A, then B, then C). See CommonKey1 for the subclassing contract.
   */
  template<class A, class B, class C>
  class CommonKey3 : public CommonKey
  {
  public:

    CommonKey3()
      : _a(A()), _b(B()), _c(C())
    {}

    CommonKey3(const A& a, const B& b, const C& c)
      : _a(a), _b(b), _c(c)
    {}

    CommonKey3(const CommonKey3& k)
      : _a(k._a), _b(k._b), _c(k._c)
    {}

    virtual ~CommonKey3() {}

    const CommonKey3& operator=(const CommonKey3& k)
    {
      if (this != &k)
      {
        _a = k._a;
        _b = k._b;
        _c = k._c;
      }
      return *this;
    }

    bool operator==(const CommonKey3& k) const
    { return ((_a == k._a) && (_b == k._b) && (_c == k._c)); }

    bool operator!=(const CommonKey3& k) const
    { return ((_a != k._a) || (_b != k._b) || (_c != k._c)); }

    bool operator<(const CommonKey3& k) const
    {
      return ((_a < k._a) ||
              ((_a == k._a) &&
               ((_b < k._b) ||
                ((_b == k._b) && (_c < k._c)))));
    }

    bool operator>(const CommonKey3& k) const
    {
      return ((_a > k._a) ||
              ((_a == k._a) &&
               ((_b > k._b) ||
                ((_b == k._b) && (_c > k._c)))));
    }

    const A& getA() const { return _a; }
    void setA(const A& a) { _a = a; }

    const B& getB() const { return _b; }
    void setB(const B& b) { _b = b; }

    const C& getC() const { return _c; }
    void setC(const C& c) { _c = c; }

    virtual std::string getString() const;

  protected:
    A _a;
    B _b;
    C _c;
  };

  template<class A, class B, class C>
  std::string CommonKey3<A, B, C>::getString() const
  {
    std::ostringstream ost;
    ost << "[ CommonKey3: [ A: " << _a
        << " , B: " << _b
        << " , C: " << _c << " ] ] ";
    return ost.str();
  }

  /**
   * CommonKey4
   *
   * Key type with four components, compared lexicographically
   * (A, B, C, then D). See CommonKey1 for the subclassing contract.
   */
  template<class A, class B, class C, class D>
  class CommonKey4 : public CommonKey
  {
  public:

    CommonKey4()
      : _a(A()), _b(B()), _c(C()), _d(D())
    {}

    CommonKey4(const A& a, const B& b, const C& c, const D& d)
      : _a(a), _b(b), _c(c), _d(d)
    {}

    CommonKey4(const CommonKey4& k)
      : _a(k._a), _b(k._b), _c(k._c), _d(k._d)
    {}

    virtual ~CommonKey4() {}

    const CommonKey4& operator=(const CommonKey4& k)
    {
      if (this != &k)
      {
        _a = k._a;
        _b = k._b;
        _c = k._c;
        _d = k._d;
      }
      return *this;
    }

    bool operator==(const CommonKey4& k) const
    {
      return ((_a == k._a) && (_b == k._b) &&
              (_c == k._c) && (_d == k._d));
    }

    bool operator!=(const CommonKey4& k) const
    {
      return ((_a != k._a) || (_b != k._b) ||
              (_c != k._c) || (_d != k._d));
    }

    bool operator<(const CommonKey4& k) const
    {
      return ((_a < k._a) ||
              ((_a == k._a) &&
               ((_b < k._b) ||
                ((_b == k._b) &&
                 ((_c < k._c) ||
                  ((_c == k._c) && (_d < k._d)))))));
    }

    bool operator>(const CommonKey4& k) const
    {
      return ((_a > k._a) ||
              ((_a == k._a) &&
               ((_b > k._b) ||
                ((_b == k._b) &&
                 ((_c > k._c) ||
                  ((_c == k._c) && (_d > k._d)))))));
    }

    const A& getA() const { return _a; }
    void setA(const A& a) { _a = a; }

    const B& getB() const { return _b; }
    void setB(const B& b) { _b = b; }

    const C& getC() const { return _c; }
    void setC(const C& c) { _c = c; }

    const D& getD() const { return _d; }
    void setD(const D& d) { _d = d; }

    virtual std::string getString() const;

  protected:
    A _a;
    B _b;
    C _c;
    D _d;
  };

  template<class A, class B, class C, class D>
  std::string CommonKey4<A, B, C, D>::getString() const
  {
    std::ostringstream ost;
    ost << "[ CommonKey4: [ A: " << _a
        << " , B: " << _b
        << " , C: " << _c
        << " , D: " << _d << " ] ] ";
    return ost.str();
  }

  /**
   * CommonKey5
   *
   * Key type with five components, compared lexicographically
   * (A, B, C, D, then E). See CommonKey1 for the subclassing contract.
   */
  template<class A, class B, class C, class D, class E>
  class CommonKey5 : public CommonKey
  {
  public:

    CommonKey5()
      : _a(A()), _b(B()), _c(C()), _d(D()), _e(E())
    {}

    CommonKey5(const A& a, const B& b, const C& c, const D& d, const E& e)
      : _a(a), _b(b), _c(c), _d(d), _e(e)
    {}

    CommonKey5(const CommonKey5& k)
      : _a(k._a), _b(k._b), _c(k._c), _d(k._d), _e(k._e)
    {}

    virtual ~CommonKey5() {}

    const CommonKey5& operator=(const CommonKey5& k)
    {
      if (this != &k)
      {
        _a = k._a;
        _b = k._b;
        _c = k._c;
        _d = k._d;
        _e = k._e;
      }
      return *this;
    }

    bool operator==(const CommonKey5& k) const
    {
      return ((_a == k._a) && (_b == k._b) &&
              (_c == k._c) && (_d == k._d) && (_e == k._e));
    }

    bool operator!=(const CommonKey5& k) const
    {
      return ((_a != k._a) || (_b != k._b) ||
              (_c != k._c) || (_d != k._d) || (_e != k._e));
    }

    bool operator<(const CommonKey5& k) const
    {
      return ((_a < k._a) ||
              ((_a == k._a) &&
               ((_b < k._b) ||
                ((_b == k._b) &&
                 ((_c < k._c) ||
                  ((_c == k._c) &&
                   ((_d < k._d) ||
                    ((_d == k._d) && (_e < k._e)))))))));
    }

    bool operator>(const CommonKey5& k) const
    {
      return ((_a > k._a) ||
              ((_a == k._a) &&
               ((_b > k._b) ||
                ((_b == k._b) &&
                 ((_c > k._c) ||
                  ((_c == k._c) &&
                   ((_d > k._d) ||
                    ((_d == k._d) && (_e > k._e)))))))));
    }

    const A& getA() const { return _a; }
    void setA(const A& a) { _a = a; }

    const B& getB() const { return _b; }
    void setB(const B& b) { _b = b; }

    const C& getC() const { return _c; }
    void setC(const C& c) { _c = c; }

    const D& getD() const { return _d; }
    void setD(const D& d) { _d = d; }

    const E& getE() const { return _e; }
    void setE(const E& e) { _e = e; }

    virtual std::string getString() const;

  protected:
    A _a;
    B _b;
    C _c;
    D _d;
    E _e;
  };

  template<class A, class B, class C, class D, class E>
  std::string CommonKey5<A, B, C, D, E>::getString() const
  {
    std::ostringstream ost;
    ost << "[ CommonKey5: [ A: " << _a
        << " , B: " << _b
        << " , C: " << _c
        << " , D: " << _d
        << " , E: " << _e << " ] ] ";
    return ost.str();
  }

  /**
   * CommonKey6
   *
   * Key type with six components, compared lexicographically
   * (A, B, C, D, E, then F). See CommonKey1 for the subclassing contract.
   */
  template<class A, class B, class C, class D, class E, class F>
  class CommonKey6 : public CommonKey
  {
  public:

    CommonKey6()
      : _a(A()), _b(B()), _c(C()), _d(D()), _e(E()), _f(F())
    {}

    CommonKey6(const A& a, const B& b, const C& c,
               const D& d, const E& e, const F& f)
      : _a(a), _b(b), _c(c), _d(d), _e(e), _f(f)
    {}

    CommonKey6(const CommonKey6& k)
      : _a(k._a), _b(k._b), _c(k._c), _d(k._d), _e(k._e), _f(k._f)
    {}

    virtual ~CommonKey6() {}

    const CommonKey6& operator=(const CommonKey6& k)
    {
      if (this != &k)
      {
        _a = k._a;
        _b = k._b;
        _c = k._c;
        _d = k._d;
        _e = k._e;
        _f = k._f;
      }
      return *this;
    }

    bool operator==(const CommonKey6& k) const
    {
      return ((_a == k._a) && (_b == k._b) &&
              (_c == k._c) && (_d == k._d) &&
              (_e == k._e) && (_f == k._f));
    }

    bool operator!=(const CommonKey6& k) const
    {
      return ((_a != k._a) || (_b != k._b) ||
              (_c != k._c) || (_d != k._d) ||
              (_e != k._e) || (_f != k._f));
    }

    bool operator<(const CommonKey6& k) const
    {
      return ((_a < k._a) ||
              ((_a == k._a) &&
               ((_b < k._b) ||
                ((_b == k._b) &&
                 ((_c < k._c) ||
                  ((_c == k._c) &&
                   ((_d < k._d) ||
                    ((_d == k._d) &&
                     ((_e < k._e) ||
                      ((_e == k._e) && (_f < k._f)))))))))));
    }

    bool operator>(const CommonKey6& k) const
    {
      return ((_a > k._a) ||
              ((_a == k._a) &&
               ((_b > k._b) ||
                ((_b == k._b) &&
                 ((_c > k._c) ||
                  ((_c == k._c) &&
                   ((_d > k._d) ||
                    ((_d == k._d) &&
                     ((_e > k._e) ||
                      ((_e == k._e) && (_f > k._f)))))))))));
    }

    const A& getA() const { return _a; }
    void setA(const A& a) { _a = a; }

    const B& getB() const { return _b; }
    void setB(const B& b) { _b = b; }

    const C& getC() const { return _c; }
    void setC(const C& c) { _c = c; }

    const D& getD() const { return _d; }
    void setD(const D& d) { _d = d; }

    const E& getE() const { return _e; }
    void setE(const E& e) { _e = e; }

    const F& getF() const { return _f; }
    void setF(const F& f) { _f = f; } 

    virtual std::string getString() const;

  protected:
    A _a;
    B _b;
    C _c;
    D _d;
    E _e;
    F _f;
  };

  template<class A, class B, class C, class D, class E, class F>
  std::string CommonKey6<A, B, C, D, E, F>::getString() const
  {
    std::ostringstream ost;
    ost << "[ CommonKey6: [ A: " << _a
        << " , B: " << _b
        << " , C: " << _c
        << " , D: " << _d
        << " , E: " << _e
        << " , F: " << _f << " ] ] ";
    return ost.str();
  }

} // namespace MyCommon

#endif // MYTEMPLATE_COMMONKEY_H
