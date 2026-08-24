/* -*- C++ -*- */
//
// RefCount.h
//
// Reference-counting utility
//
// Changes made to get this to compile standalone / on a modern compiler:
//  - TSRefCount used a single *static* pthread_mutex_t shared by every
//    TSRefCount instance in the process to guard a plain int -- a global
//    contention point that isn't actually needed. Replaced with a
//    std::atomic<int> per instance: each counter now synchronizes only
//    with itself.
// One addition: getString(), following the Stringable convention used 
// throughout this project.
//  - TSRefCount isStringable; getString() is just its
//    numeric value, matching BasicType<>::getString()'s style.
//  - RCObject<> is Stringable too, with a concrete (non-pure)
//    default getString() reporting the current ref count and
//    shareable flag. Since it's concrete, no existing subclass is
//    forced to implement anything to stay instantiable; a subclass
//    that wants a more specific string can still override it.
//  - RCPtr<> gets a plain getString() that delegates to the pointee's
//    (now always available) getString(), or reports "null" for an
//    empty handle.

#ifndef MYTEMPLATE_REFCOUNT_H
#define MYTEMPLATE_REFCOUNT_H

#include "Stringable.h"
#include <atomic>
#include <sstream>

namespace MyCommon {

  /// Thread-safe reference counter for use with RCObject<>.
  class TSRefCount : public Stringable
  {
  public:
    TSRefCount() : _value(0) {}
    explicit TSRefCount(int value) : _value(value) {}
    TSRefCount(const TSRefCount& rc) : _value(rc._value.load()) {}
    ~TSRefCount() {}

    TSRefCount& operator=(const TSRefCount& rc)
    { _value.store(rc._value.load()); return *this; }

    bool operator==(const TSRefCount& rc) const
    { return _value.load() == rc._value.load(); }

    bool operator==(int value) const
    { return _value.load() == value; }

    /// Atomically increments the count and returns the new value.
    int operator++() { return ++_value; }

    /// Atomically decrements the count and returns the new value.
    int operator--() { return --_value; }

    /// Current value of the count.
    operator int() const { return _value.load(); }

    virtual std::string getString() const
    {
      std::ostringstream ost;
      ost << _value.load();
      return ost.str();
    }

  private:
    std::atomic<int> _value;
  };

  /**
   * RCObject
   *
   * Base class for reference-counted objects, implementing the interface
   * RCPtr<> expects. Parameterized by the counter type: use `int` for a
   * plain (non-thread-safe) count or TSRefCount for one safe to touch
   * from multiple threads. removeReference() deletes the object once
   * its count reaches zero, so instances must always be heap-allocated
   * and accessed only through RCPtr<>.
   */
  template<class T>
  class RCObject : public Stringable
  {
  public:
    T getRefCount() const { return _refCount; }
    bool isShareable() const { return _shareable; }

    void addReference() { ++_refCount; }
    void removeReference() { if (--_refCount == 0) delete this; }

    void markUnshareable() { _shareable = false; }

    /// Default rendering: current ref count and shareable flag. Concrete
    /// (not pure), so a subclass isn't required to override this to stay
    /// instantiable -- but may still do so for a more specific string.
    virtual std::string getString() const
    {
      std::ostringstream ost;
      ost << " [ RCObject: refCount=" << static_cast<int>(_refCount)
          << ", shareable=" << (_shareable ? "true" : "false") << " ] ";
      return ost.str();
    }

  protected:
    RCObject() : _refCount(0), _shareable(true) {}
    RCObject(const RCObject&) : _refCount(0), _shareable(true) {}
    virtual ~RCObject() {}
    RCObject& operator=(const RCObject&) { return *this; }

  private:
    T _refCount;
    bool _shareable;
  };

  /**
   * RCPtr
   *
   * Intrusive smart pointer for objects derived from RCObject<>: copying
   * or assigning an RCPtr increments the referenced object's count,
   * destroying or reassigning it decrements the count (deleting the
   * object once nothing references it anymore). T must derive from
   * RCObject<Counter> for some counter type.
   */
  template<class T>
  class RCPtr
  {
  public:
    RCPtr(T* theRCObject = 0) : _theRCObject(theRCObject) { init(); }
    RCPtr(const RCPtr& rhs) : _theRCObject(rhs._theRCObject) { init(); }

    ~RCPtr()
    {
      if (_theRCObject) _theRCObject->removeReference();
    }

    RCPtr& operator=(const RCPtr& rhs)
    {
      if (_theRCObject != rhs._theRCObject)
      {
        if (_theRCObject) _theRCObject->removeReference();
        _theRCObject = rhs._theRCObject;
        init();
      }
      return *this;
    }

    T* operator->() const { return _theRCObject; }
    T& operator*() const { return *_theRCObject; }

    bool operator==(const RCPtr& p) const { return _theRCObject == p._theRCObject; }
    bool operator!=(const RCPtr& p) const { return _theRCObject != p._theRCObject; }

    T* getPtr() const { return _theRCObject; }
    bool isNull() const { return (_theRCObject == 0); }

    /// Delegates to the pointee's getString() (always available, since
    /// T derives from RCObject<>), or reports "null" for an empty handle.
    std::string getString() const
    {
      if (_theRCObject == 0) return " [ RCPtr: null ] ";
      return _theRCObject->getString();
    }

  private:
    void init() { if (_theRCObject) _theRCObject->addReference(); }

    T* _theRCObject;
  };

  /// Streams via getString(), same as Stringable's own operator<< --
  /// lets an RCPtr<T> be used directly as a DumpableCommonMap<> (see
  /// CommonMap.h) Data value, e.g. RCPtr<CacheEntry<Entry>> in CacheData.h.
  template<class T>
  std::ostream& operator<<(std::ostream& ost, const RCPtr<T>& p)
  {
    ost << p.getString();
    return ost;
  }

} // namespace MyCommon

#endif // MYTEMPLATE_REFCOUNT_H
