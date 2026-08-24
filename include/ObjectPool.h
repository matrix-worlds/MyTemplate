/* -*- C++ -*- */
//
// ObjectPool.h
//
// This header pre-allocates a batch of objects, hand them out and take 
// them back, auto-grow by a fixed increment when exhausted, one shared 
// instance per pooled type, report pool size via Stringable::getString() 
// -- as a standalone template, using only <list> and <mutex>.
//

#ifndef MYTEMPLATE_OBJECTPOOL_H
#define MYTEMPLATE_OBJECTPOOL_H

#include "RefCount.h"
#include "Stringable.h"
#include <cstddef>
#include <list>
#include <mutex>
#include <sstream>
#include <string>

namespace MyCommon {

  template<class T>
  class ObjectPool : public Stringable
  {
  public:

    /// A pooled reference to a T. Copyable and safe to hold onto after
    /// release()
    typedef RCPtr<T> Handle;

    /// Construct a pool pre-populated with `initialSize` objects; when
    /// exhausted, acquire() grows the pool by `incrementStep` at a time.
    explicit ObjectPool(std::size_t initialSize = 10,
                         std::size_t incrementStep = 3)
      : _incrementStep(incrementStep)
    {
      std::lock_guard<std::mutex> guard(_mutex);
      for (std::size_t i = 0; i < initialSize; ++i)
      {
        _pool.push_back(Handle(new T()));
      }
    }

    /// Drops the pool's references to whatever is still in the free
    /// list. Objects currently checked out (acquired but not released)
    /// are unaffected: their own Handle keeps them alive, and each is
    /// destroyed once its last Handle goes away, wherever that happens.
    virtual ~ObjectPool()
    {
      std::lock_guard<std::mutex> guard(_mutex);
      _pool.clear();
    }

    /// Take an object out of the pool, growing the pool first if it is
    /// currently empty.
    Handle acquire()
    {
      std::lock_guard<std::mutex> guard(_mutex);
      if (_pool.empty())
      {
        growLocked();
      }
      Handle obj = _pool.front();
      _pool.pop_front();
      return obj;
    }

    /// Return a handle previously obtained from acquire(). The object
    /// is reset() before being made available again.
    void release(const Handle& obj)
    {
      if (obj.isNull()) return;
      obj->reset();
      std::lock_guard<std::mutex> guard(_mutex);
      _pool.push_back(obj);
    }

    /// Number of objects currently available (not checked out).
    std::size_t getPoolSize() const
    {
      std::lock_guard<std::mutex> guard(_mutex);
      return _pool.size();
    }

    /// One shared pool per pooled type T, created on first use.
    static ObjectPool<T>& getInstance()
    {
      std::lock_guard<std::mutex> guard(_instanceMutex);
      if (_instance == 0)
      {
        _instance = new ObjectPool<T>();
      }
      return *_instance;
    }

    virtual std::string getString() const
    {
      std::ostringstream ost;
      ost << " [ ObjectPool: size = " << getPoolSize() << " ] ";
      return ost.str();
    }

  protected:
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

  private:
    /// Precondition: _mutex already held by the caller.
    void growLocked()
    {
      for (std::size_t i = 0; i < _incrementStep; ++i)
      {
        _pool.push_back(Handle(new T()));
      }
    }

    std::list<Handle> _pool;
    mutable std::mutex _mutex;
    std::size_t _incrementStep;

    static ObjectPool<T>* _instance;
    static std::mutex _instanceMutex;
  };

  template<class T>
  ObjectPool<T>* ObjectPool<T>::_instance = 0;

  template<class T>
  std::mutex ObjectPool<T>::_instanceMutex;

} // namespace MyCommon

#endif // MYTEMPLATE_OBJECTPOOL_H
