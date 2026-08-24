/* -*- C++ -*- */
//
// test_ObjectPool.cpp
//
// Unit tests for MyCommon::ObjectPool<>, the generalized/generified form
// of an original message-pool class. Widget below plays the role the
// original's pooled message type played: a default-constructible,
// RCObject<TSRefCount>-derived, poolable type with a reset() the pool
// calls when a handle is released.

#include "MiniTest.h"
#include "ObjectPool.h"
#include "RefCount.h"

using namespace MyCommon;

namespace {

struct Widget : public RCObject<TSRefCount>
{
  Widget() : value(-1) {}
  void reset() { value = -1; }
  int value;
};

void test_InitialPoolSize(MiniTest& t)
{
  ObjectPool<Widget> pool(3, 2);
  MT_CHECK(t, pool.getPoolSize() == 3);
}

/// The constructor's initialSize/incrementStep both default to 5.
void test_DefaultConstructorUsesDefaultSizes(MiniTest& t)
{
  ObjectPool<Widget> pool;
  MT_CHECK(t, pool.getPoolSize() == 10);

  // Drain it and confirm growth also uses the default increment (3).
  for (int i = 0; i < 10; ++i) pool.acquire();
  MT_CHECK(t, pool.getPoolSize() == 0);
  pool.acquire();
  MT_CHECK(t, pool.getPoolSize() == 2); // grew by 3, one immediately taken
}

/// getPoolSize() in isolation: reflects acquisitions and releases, and
/// nothing else.
void test_GetPoolSize(MiniTest& t)
{
  ObjectPool<Widget> pool(4, 4);
  MT_CHECK(t, pool.getPoolSize() == 4);

  ObjectPool<Widget>::Handle w = pool.acquire();
  MT_CHECK(t, pool.getPoolSize() == 3);

  pool.release(w);
  MT_CHECK(t, pool.getPoolSize() == 4);
}

/// release() on a default-constructed (null) handle must be a safe no-op,
/// not a crash -- mirrors the isNull() guard at the top of release().
void test_ReleaseNullHandleIsNoop(MiniTest& t)
{
  ObjectPool<Widget> pool(2, 2);
  ObjectPool<Widget>::Handle empty;
  MT_CHECK(t, empty.isNull());

  MT_NO_THROW(t, pool.release(empty));
  MT_CHECK(t, pool.getPoolSize() == 2); // unaffected
}

/// Destroying the pool while a handle is still checked out must not
/// affect that handle: the object stays alive until the handle itself
/// goes away, per ~ObjectPool()'s documented contract.
void test_PoolDestructionDoesNotAffectCheckedOutHandles(MiniTest& t)
{
  ObjectPool<Widget>::Handle w;
  {
    ObjectPool<Widget> pool(1, 1);
    w = pool.acquire();
    w->value = 5;
  } // pool destroyed here; w is still checked out
  MT_CHECK(t, w->value == 5); // object is still alive and unchanged
}

void test_AcquireShrinksPool(MiniTest& t)
{
  ObjectPool<Widget> pool(2, 2);

  ObjectPool<Widget>::Handle w1 = pool.acquire();
  MT_CHECK(t, !w1.isNull());
  MT_CHECK(t, pool.getPoolSize() == 1);

  ObjectPool<Widget>::Handle w2 = pool.acquire();
  MT_CHECK(t, !w2.isNull());
  MT_CHECK(t, pool.getPoolSize() == 0);

  pool.release(w1);
  pool.release(w2);
  MT_CHECK(t, pool.getPoolSize() == 2);
}

void test_AutoGrowWhenExhausted(MiniTest& t)
{
  ObjectPool<Widget> pool(1, 3);

  ObjectPool<Widget>::Handle w1 = pool.acquire();
  MT_CHECK(t, pool.getPoolSize() == 0);

  // Pool is empty: acquire() must grow it by incrementStep (3) first.
  ObjectPool<Widget>::Handle w2 = pool.acquire();
  MT_CHECK(t, pool.getPoolSize() == 2);

  pool.release(w1);
  pool.release(w2);
  MT_CHECK(t, pool.getPoolSize() == 4);
}

void test_ReleaseResetsObject(MiniTest& t)
{
  ObjectPool<Widget> pool(1, 1);

  ObjectPool<Widget>::Handle w = pool.acquire();
  w->value = 42;
  pool.release(w);

  ObjectPool<Widget>::Handle recycled = pool.acquire();
  MT_CHECK(t, recycled.getPtr() == w.getPtr()); // same object recycled
  MT_CHECK(t, recycled->value == -1);            // reset() applied on release

  pool.release(recycled);
}

void test_GetString(MiniTest& t)
{
  ObjectPool<Widget> pool(2, 2);
  MT_CHECK(t, pool.getString() == " [ ObjectPool: size = 2 ] ");
}

/// Widget doesn't implement getString() itself; it inherits the default
/// from RCObject<TSRefCount> (see RefCount.h), and an acquired handle
/// gets to it through RCPtr<>::getString()'s delegation.
void test_PooledObjectGetStringComesFromRCObject(MiniTest& t)
{
  ObjectPool<Widget> pool(1, 1);
  ObjectPool<Widget>::Handle w = pool.acquire();

  MT_CHECK(t, w->getString().find("RCObject") != std::string::npos);
  MT_CHECK(t, w->getString().find("refCount=1") != std::string::npos);
  MT_CHECK(t, w.getString() == w->getString());

  pool.release(w);
}

void test_SingletonPerType(MiniTest& t)
{
  ObjectPool<Widget>& a = ObjectPool<Widget>::getInstance();
  ObjectPool<Widget>& b = ObjectPool<Widget>::getInstance();
  MT_CHECK(t, &a == &b);
}

/// The point of pooling RCPtr<T> handles instead of raw T*: an object
/// checked out and never explicitly released is still not leaked -- it
/// is destroyed as soon as the last handle referencing it disappears.
void test_UnreleasedHandleSelfDestructsInsteadOfLeaking(MiniTest& t)
{
  bool destructed = false;

  struct TrackedWidget : public RCObject<TSRefCount>
  {
    explicit TrackedWidget(bool* flag) : _flag(flag) { *_flag = false; }
    ~TrackedWidget() { *_flag = true; }
    void reset() {}
    bool* _flag;
  };

  // Not pooled via ObjectPool<> here (TrackedWidget isn't default-
  // constructible, which ObjectPool<> requires) -- this test exercises
  // the same RCPtr lifetime mechanism ObjectPool<>::acquire() relies on.
  {
    RCPtr<TrackedWidget> handle(new TrackedWidget(&destructed));
    MT_CHECK(t, !destructed);
  }
  MT_CHECK(t, destructed); // deleted once the last handle went out of scope
}

/// Multiple handles to the same acquired object keep it alive until all
/// of them are gone, even if none of them is ever release()d.
void test_SharedHandleKeepsObjectAlive(MiniTest& t)
{
  ObjectPool<Widget> pool(1, 1);

  ObjectPool<Widget>::Handle first = pool.acquire();
  Widget* raw = first.getPtr();
  MT_CHECK(t, first->getRefCount() == 1);

  {
    ObjectPool<Widget>::Handle second = first; // copy: refcount -> 2
    MT_CHECK(t, first->getRefCount() == 2);
    MT_CHECK(t, second.getPtr() == raw);
  } // second destroyed: refcount -> 1

  MT_CHECK(t, first->getRefCount() == 1);
  pool.release(first);
}

} // namespace

int main()
{
  MiniTest t("test_ObjectPool");

  MT_RUN(t, test_InitialPoolSize);
  MT_RUN(t, test_DefaultConstructorUsesDefaultSizes);
  MT_RUN(t, test_GetPoolSize);
  MT_RUN(t, test_AcquireShrinksPool);
  MT_RUN(t, test_AutoGrowWhenExhausted);
  MT_RUN(t, test_ReleaseResetsObject);
  MT_RUN(t, test_ReleaseNullHandleIsNoop);
  MT_RUN(t, test_PoolDestructionDoesNotAffectCheckedOutHandles);
  MT_RUN(t, test_GetString);
  MT_RUN(t, test_PooledObjectGetStringComesFromRCObject);
  MT_RUN(t, test_SingletonPerType);
  MT_RUN(t, test_UnreleasedHandleSelfDestructsInsteadOfLeaking);
  MT_RUN(t, test_SharedHandleKeepsObjectAlive);

  return t.result();
}
