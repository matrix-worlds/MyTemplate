/* -*- C++ -*- */
//
// CacheData.h
//
//   BasicType<>      (BasicTypes.h)   -- the data types columns hold
//   CommonKey1..6<>  (CommonKey.h)    -- a table's key, built from those types
//   CommonEntry<>    (Data.h)         -- a table row: a key plus columns
//   CommonTable<>    (Data.h)         -- the table itself (query/insert/...)
//   *CacheData<>     (this file)      -- an in-memory, reference-counted
//                                        cache of a table's contents
//
//  - CacheEntry<Entry>: wraps an Entry as an RCObject<TSRefCount>, so it
//    can be shared via RCPtr<> between the cache and every caller
//    holding a lookup result.
//  - CommonCacheProcessor<>: a ResultsProcessor<Entry> (see Data.h) that
//    wraps each row from a table query in a CacheEntry<> and inserts it
//    into an AssociativeContainer (CommonMap<>/DumpableCommonMap<> --
//    see CommonMap.h) keyed by entry.getCacheKey().
//  - Unconstrained/ConstrainedSingleVersionSnapshotCacheData<>: load (or
//    reload) a table's contents into a fresh, private AssociativeContainer,
//    then atomically swap it in via AssociativeContainer::reload()
//    (already in CommonMap.h) -- so a reload() never leaves lookupEntry()
//    seeing a half-populated cache. "Constrained" adds a Criteria
//    (e.g. filter by category) the query is run with.
//

#ifndef MYTEMPLATE_CACHEDATA_H
#define MYTEMPLATE_CACHEDATA_H

#include "CommonMap.h"
#include "Data.h"
#include "RefCount.h"
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>

namespace MyCommon {

  /**
   * CacheEntry
   *
   * A reference-counted wrapper around a table's Entry type, so cached
   * rows can be shared (via RCPtr<CacheEntry<Entry>>) between the cache
   * and every caller holding a lookup result, without copying the row
   * or worrying about who deletes it.
   */
  template<class Entry>
  class CacheEntry : public Entry, public RCObject<TSRefCount>
  {
  public:
    CacheEntry() : Entry() {}
    explicit CacheEntry(const Entry& entry) : Entry(entry) {}
    virtual ~CacheEntry() {}

    /// Entry and RCObject<TSRefCount> both ultimately derive from
    /// Stringable (Entry via CommonEntry<>/Data; RCObject<> got its own
    /// default getString() when this project extracted RefCount.h --
    /// see RefCount.h's file comment), so without this override
    /// getString() would be ambiguous: two unrelated Stringable base
    /// subobjects, neither more derived than the other. Route it to
    /// Entry's -- a cached entry should print exactly like the
    /// uncached row it wraps.
    virtual std::string getString() const { return Entry::getString(); }
  };

  /**
   * CommonCacheProcessor
   *
   * A ResultsProcessor<Entry> (see Data.h) that, for each row a table
   * query hands it, wraps it in a CacheEntry<> and inserts the
   * (entry.getCacheKey(), handle) pair into an AssociativeContainer
   * (typically a CommonMap<>/DumpableCommonMap<>). Used internally by
   * *SingleVersionSnapshotCacheData<>::reload() below.
   */
  template<class CacheKey, class Entry, class AssociativeContainer>
  class CommonCacheProcessor : public ResultsProcessor<Entry>
  {
  public:
    explicit CommonCacheProcessor(AssociativeContainer& sourceContainer)
      : _cacheContainer(sourceContainer)
    {}

    virtual bool process(const Entry& entry)
    {
      RCPtr<CacheEntry<Entry> > eh(new CacheEntry<Entry>(entry));

      if (!_cacheContainer.addDataEntry(entry.getCacheKey(), eh))
      {
        std::cerr << "CommonCacheProcessor::process(): addDataEntry() "
                     "failed for entry: " << entry.getString()
                  << " - ignored\n";
      }
      return true;
    }

  protected:
    AssociativeContainer& _cacheContainer;
  };

  /**
   * UnconstrainedSingleVersionSnapshotCacheData
   *
   * An in-memory, reference-counted cache of an entire table's
   * contents, queried and rebuilt from scratch on every reload() (no
   * incremental refresh -- see the file comment for what that would
   * need). Registers with DiagnosticDumpRegistry so it participates in
   * a process-wide dump() alongside CommonMap<> and friends.
   */
  template<class CacheKey, class Entry, class Table,
           class AssociativeContainer = DumpableCommonMap<CacheKey, RCPtr<CacheEntry<Entry> > >,
           class CacheProcessor = CommonCacheProcessor<CacheKey, Entry, AssociativeContainer> >
  class UnconstrainedSingleVersionSnapshotCacheData
    : public AssociativeContainer
  // AssociativeContainer (DumpableCommonMap<>/CommonMap<>, see
  // CommonMap.h) already derives from Dumpable 
  {
  public:
    explicit UnconstrainedSingleVersionSnapshotCacheData(const std::string& name)
      : AssociativeContainer(), _tableName(name)
    {
      DiagnosticDumpRegistry::registerDumpable(_tableName, this);
    }

    virtual ~UnconstrainedSingleVersionSnapshotCacheData()
    {
      DiagnosticDumpRegistry::deregisterDumpable(this);
    }

    /// Re-runs the table's unconstrained query() and atomically swaps
    /// the result in -- lookupEntry() never sees a half-populated cache.
    virtual void reload()
    {
      Table table;
      AssociativeContainer localContainer;

      CacheProcessor processor(localContainer);
      table.query(processor);

      AssociativeContainer::reload(localContainer);
    }

    /// Looks up a cached entry by CacheKey (which may be a projection
    /// of the table's full key -- see the file comment).
    virtual bool lookupEntry(const CacheKey& key, RCPtr<CacheEntry<Entry> >& eh) const
    {
      return this->lookupDataEntry(key, eh);
    }

    virtual void dump() const
    {
      std::lock_guard<std::mutex> guard(_instanceMutex);
      std::cout << _tableName << " CacheData Dump:\n"
                << "===================================================\n";
      AssociativeContainer::dump();
      std::cout << "===================================================\n";
    }

    const std::string& getTableName() const { return _tableName; }

    /// One shared instance per (CacheKey, Entry, Table, ...) instantiation.
    static void setInstance(UnconstrainedSingleVersionSnapshotCacheData* instance)
    {
      std::lock_guard<std::mutex> guard(_instanceMutex);
      if (_instance != 0)
      {
        throw std::logic_error("CacheData singleton instance already set");
      }
      _instance = instance;
    }

    static UnconstrainedSingleVersionSnapshotCacheData* getInstance()
    {
      std::lock_guard<std::mutex> guard(_instanceMutex);
      if (_instance == 0)
      {
        throw std::logic_error("CacheData singleton instance not set");
      }
      return _instance;
    }

    /// Clears the singleton pointer (does not delete it) 
    static void resetInstanceForTesting() { _instance = 0; }

  protected:
    static UnconstrainedSingleVersionSnapshotCacheData* _instance;
    static std::mutex _instanceMutex;

  private:
    UnconstrainedSingleVersionSnapshotCacheData(const UnconstrainedSingleVersionSnapshotCacheData&);
    UnconstrainedSingleVersionSnapshotCacheData& operator=(const UnconstrainedSingleVersionSnapshotCacheData&);

    const std::string _tableName;
  };

  template<class CacheKey, class Entry, class Table, class AssociativeContainer, class CacheProcessor>
  UnconstrainedSingleVersionSnapshotCacheData<CacheKey, Entry, Table, AssociativeContainer, CacheProcessor>*
  UnconstrainedSingleVersionSnapshotCacheData<CacheKey, Entry, Table, AssociativeContainer, CacheProcessor>::_instance = 0;

  template<class CacheKey, class Entry, class Table, class AssociativeContainer, class CacheProcessor>
  std::mutex
  UnconstrainedSingleVersionSnapshotCacheData<CacheKey, Entry, Table, AssociativeContainer, CacheProcessor>::_instanceMutex;

  /**
   * ConstrainedSingleVersionSnapshotCacheData
   *
   * Same as UnconstrainedSingleVersionSnapshotCacheData<>, but reload()
   * runs the table's *constrained* query(criteria, processor) instead
   * of its unconstrained one -- e.g. "only widgets in this category"
   * rather than "every widget". 
   */
  template<class CacheKey, class Entry, class Table,
           class AssociativeContainer = DumpableCommonMap<CacheKey, RCPtr<CacheEntry<Entry> > >,
           class CacheProcessor = CommonCacheProcessor<CacheKey, Entry, AssociativeContainer>,
           class Criteria = std::string>
  class ConstrainedSingleVersionSnapshotCacheData
    : public UnconstrainedSingleVersionSnapshotCacheData<CacheKey, Entry, Table, AssociativeContainer, CacheProcessor>
  {
  public:
    typedef UnconstrainedSingleVersionSnapshotCacheData<CacheKey, Entry, Table, AssociativeContainer, CacheProcessor> Base;

    ConstrainedSingleVersionSnapshotCacheData(const Criteria& criteria, const std::string& name)
      : Base(name), _criteria(criteria)
    {}

    virtual void reload()
    {
      Table table;
      AssociativeContainer localContainer;

      CacheProcessor processor(localContainer);
      table.query(_criteria, processor);

      AssociativeContainer::reload(localContainer);
    }

    const Criteria& getCriteria() const { return _criteria; }

  private:
    ConstrainedSingleVersionSnapshotCacheData(const ConstrainedSingleVersionSnapshotCacheData&);
    ConstrainedSingleVersionSnapshotCacheData& operator=(const ConstrainedSingleVersionSnapshotCacheData&);

    Criteria _criteria;
  };

} // namespace MyCommon

#endif // MYTEMPLATE_CACHEDATA_H
