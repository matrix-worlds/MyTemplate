/* -*- C++ -*- */
//
// CommonMap.h
//

#ifndef MYTEMPLATE_COMMONMAP_H
#define MYTEMPLATE_COMMONMAP_H

#include "Dump.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

namespace MyCommon {

  /**
   * CommonMap
   *
   * Template wrapper around std::map<Key, Data> providing the generic
   * operations most callers need: insert-if-absent, lookup, remove,
   * bulk reload, and a parallel vector of keys. Registers itself with
   * DiagnosticDumpRegistry so its dump() participates in a
   * process-wide diagnostic dump.
   *
   * Use DumpableCommonMap<> instead when Key and Data both support
   * operator<< and you want dump() to print entry contents, not just
   * keys.
   */
  template<class Key, class Data, class Mutex = std::recursive_mutex>
  class CommonMap : public Dumpable
  {
  public:

    typedef std::map<Key, Data> DataMap;
    typedef std::vector<Key> KeyVector;

    CommonMap()
    {
      DiagnosticDumpRegistry::registerDumpable("CommonMap<>", this);
    }

    CommonMap(const CommonMap& cp)
      : _dataMap(cp._dataMap),
        _keyVector(cp._keyVector)
    {}

    virtual ~CommonMap()
    {
      DiagnosticDumpRegistry::deregisterDumpable(this);
    }

    CommonMap& operator=(const CommonMap& cp)
    {
      if (this != &cp)
      {
        std::lock_guard<Mutex> guard(_mutex);
        _dataMap = cp._dataMap;
        _keyVector = cp._keyVector;
      }
      return *this;
    }

    /// Inserts `data` under `key` if `key` is not already present.
    /// Returns true if inserted, false if `key` was already in the map.
    virtual bool addDataEntry(const Key& key, Data data)
    {
      std::lock_guard<Mutex> guard(_mutex);
      if (_dataMap.insert(std::make_pair(key, data)).second)
      {
        _keyVector.push_back(key);
        return true;
      }
      return false;
    }

    /// If `key` is present, copies its Data into `data` and returns
    /// true; otherwise leaves `data` unmodified and returns false.
    virtual bool lookupDataEntry(const Key& key, Data& data) const
    {
      std::lock_guard<Mutex> guard(_mutex);
      typename DataMap::const_iterator k = _dataMap.find(key);
      if (k != _dataMap.end())
      {
        data = k->second;
        return true;
      }
      return false;
    }

    /// Removes the entry for `key`, if any.
    virtual void removeDataEntry(const Key& key)
    {
      std::lock_guard<Mutex> guard(_mutex);
      _dataMap.erase(key);
      _keyVector.erase(std::remove(_keyVector.begin(), _keyVector.end(), key),
                        _keyVector.end());
    }

    const DataMap& getDataMap() const { return _dataMap; }
    DataMap& getDataMap() { return _dataMap; }

    unsigned int size() const { return _dataMap.size(); }

    /// Removes every entry, `delete`-ing each Data first. Only call
    /// this when Data is a pointer type that was allocated with `new`
    void clearDataMap()
    {
      std::lock_guard<Mutex> guard(_mutex);
      typename DataMap::iterator k = _dataMap.begin();
      while (k != _dataMap.end())
      {
        typename DataMap::iterator temp = k;
        ++k;
        delete temp->second;
        _dataMap.erase(temp);
      }
      _keyVector.clear();
    }

    /// Replaces this map's contents with `commonMap`'s and rebuilds
    /// the key vector to match.
    virtual void reload(CommonMap<Key, Data, Mutex>& commonMap)
    {
      std::lock_guard<Mutex> guard(_mutex);
      _dataMap = commonMap._dataMap;

      _keyVector.clear();
      for (typename DataMap::const_iterator m = _dataMap.begin();
           m != _dataMap.end(); ++m)
      {
        _keyVector.push_back(m->first);
      }
    }

    /// The vector of keys currently in the map.
    virtual KeyVector& getKeyVector() { return _keyVector; }

    /// Prints the number of entries and each key. Data contents are
    /// not printed here -- see DumpableCommonMap<> for that.
    virtual void dump() const
    {
      std::lock_guard<Mutex> guard(_mutex);
      std::cout << "CommonMap::dump(): size = " << _dataMap.size() << "\n";
      for (typename DataMap::const_iterator m = _dataMap.begin();
           m != _dataMap.end(); ++m)
      {
        std::ostringstream key;
        key << m->first;
        std::cout << "[ Key: " << key.str()
                   << ", Data: caller's responsibility to print Data ]\n";
      }
    }

    virtual Mutex& getMutex() { return _mutex; }

  private:
    DataMap _dataMap;
    KeyVector _keyVector;

  protected:
    mutable Mutex _mutex;
  };

  /**
   * DumpableCommonMap
   *
   * A CommonMap<> whose dump() prints both keys and values, via
   * operator<<. Requires Key and Data to each support operator<<
   * (built-in types and classes do so natively; a user-defined class
   * should derive from Stringable and rely on its operator<<).
   */
  template<class Key, class Data, class Mutex = std::recursive_mutex>
  class DumpableCommonMap : public CommonMap<Key, Data, Mutex>
  {
  public:

    DumpableCommonMap() {}
    virtual ~DumpableCommonMap() {}

    virtual void dump() const
    {
      typedef typename CommonMap<Key, Data, Mutex>::DataMap DataMap;

      std::lock_guard<Mutex> guard(this->_mutex);
      const DataMap& dataMap = this->getDataMap();

      std::cout << "DumpableCommonMap::dump(): size = " << dataMap.size() << "\n";
      for (typename DataMap::const_iterator m = dataMap.begin();
           m != dataMap.end(); ++m)
      {
        std::cout << "[ Key: " << m->first << ", Data: " << m->second << " ]\n";
      }
    }

  protected:
    DumpableCommonMap(const DumpableCommonMap&) {}
    DumpableCommonMap& operator=(const DumpableCommonMap&) { return *this; }
  };

} // namespace MyCommon

#endif // MYTEMPLATE_COMMONMAP_H
