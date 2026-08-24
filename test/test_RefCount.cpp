/* -*- C++ -*- */
//
// test_RefCount.cpp
//

#include "MiniTest.h"
#include "RefCount.h"
#include "Stringable.h"
#include <thread>
#include <vector>

using namespace MyCommon;

namespace {

void test_TSRefCountBasicOps(MiniTest& t)
{
  TSRefCount rc;
  MT_CHECK(t, rc == 0);

  MT_CHECK(t, ++rc == 1);
  MT_CHECK(t, ++rc == 2);
  MT_CHECK(t, --rc == 1);

  TSRefCount other(1);
  MT_CHECK(t, rc == other);

  TSRefCount copy(rc);
  MT_CHECK(t, copy == rc);
}

void test_TSRefCountAssignment(MiniTest& t)
{
  TSRefCount a(3);
  TSRefCount b(9);

  MT_NO_THROW(t, a = b);
  MT_CHECK(t, a == b);
  MT_CHECK(t, a == 9);

  // Mutating one afterwards must not affect the other -- confirms
  // operator= copied the value, not some shared state.
  ++a;
  MT_CHECK(t, a == 10);
  MT_CHECK(t, b == 9);
}

void test_TSRefCountGetString(MiniTest& t)
{
  TSRefCount rc(5);
  MT_CHECK(t, rc.getString() == "5");

  ++rc;
  MT_CHECK(t, rc.getString() == "6");

  // Polymorphic call through the Stringable base is the same string.
  const Stringable& asStringable = rc;
  MT_CHECK(t, asStringable.getString() == "6");
}

void test_RCObjectLifecycle_PlainCounter(MiniTest& t)
{
  bool destructed = false;

  struct CountedObj : public RCObject<int>
  {
    explicit CountedObj(bool* flag) : _flag(flag) { *_flag = false; }
    ~CountedObj() { *_flag = true; }
    bool* _flag;
  };

  CountedObj* raw = new CountedObj(&destructed);
  MT_CHECK(t, raw->getRefCount() == 0);
  MT_CHECK(t, raw->isShareable());

  RCPtr<CountedObj> p1(raw);
  MT_CHECK(t, p1->getRefCount() == 1);

  RCPtr<CountedObj> p2(p1);
  MT_CHECK(t, p1->getRefCount() == 2);
  MT_CHECK(t, &(*p1) == &(*p2));

  {
    RCPtr<CountedObj> p3(p2);
    MT_CHECK(t, p1->getRefCount() == 3);
  } // p3 destroyed
  MT_CHECK(t, p1->getRefCount() == 2);

  p1->markUnshareable();
  MT_CHECK(t, !p1->isShareable());

  p1 = 0; // drop one reference
  MT_CHECK(t, !destructed);
  p2 = 0; // drop the last reference
  MT_CHECK(t, destructed);
}

void test_RCObjectLifecycle_ThreadSafeCounter(MiniTest& t)
{
  bool destructed = false;

  struct TSCountedObj : public RCObject<TSRefCount>
  {
    explicit TSCountedObj(bool* flag) : _flag(flag) { *_flag = false; }
    ~TSCountedObj() { *_flag = true; }
    bool* _flag;
  };

  RCPtr<TSCountedObj> p1(new TSCountedObj(&destructed));
  MT_CHECK(t, p1->getRefCount() == 1);

  RCPtr<TSCountedObj> p2(p1);
  MT_CHECK(t, p1->getRefCount() == 2);

  p1 = 0;
  MT_CHECK(t, !destructed);
  p2 = 0;
  MT_CHECK(t, destructed);
}

/// addReference()/removeReference() called directly, without going
/// through RCPtr<> -- RCPtr<> is just one caller of these public
/// methods, so they deserve their own test independent of it.
void test_RCObjectAddRemoveReferenceDirectly(MiniTest& t)
{
  bool destructed = false;

  struct Obj : public RCObject<int>
  {
    explicit Obj(bool* flag) : _flag(flag) { *_flag = false; }
    ~Obj() { *_flag = true; }
    bool* _flag;
  };

  Obj* raw = new Obj(&destructed);
  MT_CHECK(t, raw->getRefCount() == 0);

  raw->addReference();
  MT_CHECK(t, raw->getRefCount() == 1);

  raw->addReference();
  MT_CHECK(t, raw->getRefCount() == 2);

  raw->removeReference();
  MT_CHECK(t, raw->getRefCount() == 1);
  MT_CHECK(t, !destructed);

  raw->removeReference(); // count hits zero: deletes itself
  MT_CHECK(t, destructed);
}

void test_RCPtrEqualityAndNull(MiniTest& t)
{
  struct Obj : public RCObject<int> {};

  RCPtr<Obj> empty;
  MT_CHECK(t, empty.isNull());

  RCPtr<Obj> a(new Obj());
  MT_CHECK(t, !a.isNull());

  RCPtr<Obj> b(a);
  MT_CHECK(t, a == b);
  MT_CHECK(t, a != empty);
}

void test_RCPtrGetPtr(MiniTest& t)
{
  struct Obj : public RCObject<int> {};

  RCPtr<Obj> empty;
  MT_CHECK(t, empty.getPtr() == 0);

  Obj* raw = new Obj();
  RCPtr<Obj> p(raw);
  MT_CHECK(t, p.getPtr() == raw);
}

/// operator=() assigning between two distinct, live objects: the old
/// referent loses a reference (and is deleted if that was its last),
/// the new one gains one.
void test_RCPtrAssignmentSwitchesReferent(MiniTest& t)
{
  bool firstDestructed = false;
  bool secondDestructed = false;

  struct Obj : public RCObject<int>
  {
    explicit Obj(bool* flag) : _flag(flag) { *_flag = false; }
    ~Obj() { *_flag = true; }
    bool* _flag;
  };

  RCPtr<Obj> p(new Obj(&firstDestructed));
  RCPtr<Obj> q(new Obj(&secondDestructed));
  MT_CHECK(t, p->getRefCount() == 1);
  MT_CHECK(t, q->getRefCount() == 1);

  p = q; // p's original object was only referenced by p: gets deleted
  MT_CHECK(t, firstDestructed);
  MT_CHECK(t, !secondDestructed);
  MT_CHECK(t, p == q);
  MT_CHECK(t, q->getRefCount() == 2);

  // Self-assignment must be a safe no-op (operator= checks pointer
  // identity before touching reference counts).
  MT_NO_THROW(t, p = p);
  MT_CHECK(t, q->getRefCount() == 2);
}

/// RCObject<>::getString() default rendering, and RCPtr<>::getString()
/// delegating to it.
void test_RCObjectGetStringDefault(MiniTest& t)
{
  struct PlainObj : public RCObject<int> {};

  RCPtr<PlainObj> empty;
  MT_CHECK(t, empty.getString() == " [ RCPtr: null ] ");

  RCPtr<PlainObj> p(new PlainObj());
  MT_CHECK(t, p->getString().find("refCount=1") != std::string::npos);
  MT_CHECK(t, p->getString().find("shareable=true") != std::string::npos);
  MT_CHECK(t, p.getString() == p->getString()); // RCPtr delegates to pointee

  p->markUnshareable();
  MT_CHECK(t, p->getString().find("shareable=false") != std::string::npos);

  RCPtr<PlainObj> q(p);
  MT_CHECK(t, p->getString().find("refCount=2") != std::string::npos);
}

/// A subclass may still override getString() for a more specific string;
/// RCPtr<>::getString() must call it polymorphically, not RCObject<>'s.
void test_RCObjectGetStringSubclassOverride(MiniTest& t)
{
  struct CustomObj : public RCObject<int>
  {
    virtual std::string getString() const { return "[ CustomObj ]"; }
  };

  RCPtr<CustomObj> p(new CustomObj());
  MT_CHECK(t, p->getString() == "[ CustomObj ]");
  MT_CHECK(t, p.getString() == "[ CustomObj ]");
}

/// Concurrent copies/destructions of an RCPtr<TSCountedObj> across
/// several threads must leave the reference count exactly where it
/// started -- the point of TSRefCount's atomic increments/decrements.
void test_ConcurrentAccess(MiniTest& t)
{
  bool destructed = false;

  struct TSCountedObj : public RCObject<TSRefCount>
  {
    explicit TSCountedObj(bool* flag) : _flag(flag) { *_flag = false; }
    ~TSCountedObj() { *_flag = true; }
    bool* _flag;
  };

  RCPtr<TSCountedObj> shared(new TSCountedObj(&destructed));
  int before = shared->getRefCount();

  auto worker = [&shared]() {
    for (int i = 0; i < 20000; ++i)
    {
      RCPtr<TSCountedObj> local = shared;
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < 8; ++i)
  {
    threads.push_back(std::thread(worker));
  }
  for (auto& th : threads)
  {
    th.join();
  }

  int after = shared->getRefCount();
  MT_CHECK(t, before == after);
}

} // namespace

int main()
{
  MiniTest t("test_RefCount");

  MT_RUN(t, test_TSRefCountBasicOps);
  MT_RUN(t, test_TSRefCountAssignment);
  MT_RUN(t, test_TSRefCountGetString);
  MT_RUN(t, test_RCObjectLifecycle_PlainCounter);
  MT_RUN(t, test_RCObjectLifecycle_ThreadSafeCounter);
  MT_RUN(t, test_RCObjectAddRemoveReferenceDirectly);
  MT_RUN(t, test_RCPtrEqualityAndNull);
  MT_RUN(t, test_RCPtrGetPtr);
  MT_RUN(t, test_RCPtrAssignmentSwitchesReferent);
  MT_RUN(t, test_RCObjectGetStringDefault);
  MT_RUN(t, test_RCObjectGetStringSubclassOverride);
  MT_RUN(t, test_ConcurrentAccess);

  return t.result();
}
