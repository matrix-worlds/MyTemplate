/* -*- C++ -*- */
//
// Data.h
//

#ifndef MYTEMPLATE_DATA_H
#define MYTEMPLATE_DATA_H

#include "DataFactory.h"
#include "Stringable.h"
#include <iostream>
#include <sstream>
#include <string>

namespace MyCommon {

  /// Base class for row/entry types (rows in a table, or a query
  /// result). Concrete <schema>Entry classes derive from CommonEntry<>
  /// below rather than from this directly.
  class Data : public Stringable
  {
  public:
    virtual ~Data() {}
    virtual std::string getString() const { return "Data::getString()"; }
  };

  /**
   * CommonEntry
   *
   * A row: a Key plus whatever the Database base class contributes
   * (defaults to just Data). Concrete entry types add the non-key
   * columns and override getString() to include them -- see
   * WidgetTable.h for a worked example.
   */
  template<class Key, class Database = Data>
  class CommonEntry : public Database
  {
  public:
    CommonEntry()
      : Database(), _key(Key())
    {}

    explicit CommonEntry(const Key& key)
      : Database(), _key(key)
    {}

    virtual ~CommonEntry() {}

    const Key& getKey() const { return _key; }
    void setKey(const Key& key) { _key = key; }

    virtual std::string getString() const;

  protected:
    Key _key;
  };

  template<class Key, class Database>
  std::string CommonEntry<Key, Database>::getString() const
  {
    std::ostringstream ost;
    ost << " [ Key: " << _key << " ] ";
    return ost.str();
  }

  /// Callback interface a query() call feeds each matching row to, one
  /// at a time. process() returns false to stop early.
  template<class T>
  class ResultsProcessor
  {
  public:
    virtual ~ResultsProcessor() {}
    virtual bool process(const T& t) = 0;
  };

  /// Copies each result to `dest`, advancing it each time. 
  template<class Result, class OutputIterator>
  class CopyProcessor : public ResultsProcessor<Result>
  {
  public:
    explicit CopyProcessor(OutputIterator dest) : _dest(dest) {}
    virtual ~CopyProcessor() {}

    virtual bool process(const Result& result)
    {
      *_dest = result;
      ++_dest;
      return true;
    }

  private:
    OutputIterator _dest;
  };

  /// Populates `entry` with the first result seen (if any) and sets
  /// `found` accordingly; stops the query after that first row.
  template<class T>
  class FindProcessor : public ResultsProcessor<T>
  {
  public:
    FindProcessor(T& entry, bool& found) : _entry(entry), _found(found)
    { _found = false; }

    virtual ~FindProcessor() {}

    virtual bool process(const T& e)
    {
      _entry = e;
      _found = true;
      return false; // one match is enough
    }

  private:
    T& _entry;
    bool& _found;
  };

  /// Counts matching rows without keeping them.
  template<class T>
  class CountProcessorTemplate : public ResultsProcessor<T>
  {
  public:
    explicit CountProcessorTemplate(int& count) : _count(count) { _count = 0; }
    virtual ~CountProcessorTemplate() {}

    virtual bool process(const T&) { ++_count; return true; }

  private:
    int& _count;
  };

  /// Deletes every matching row from `table` (which must provide a
  /// `remove(Key)` method -- concrete tables add this alongside the
  /// query()s CommonTable<> requires).
  template<class T, class E>
  class DeleteProcessorTemplate : public ResultsProcessor<E>
  {
  public:
    explicit DeleteProcessorTemplate(T& table) : _table(table) {}
    virtual ~DeleteProcessorTemplate() {}

    virtual bool process(const E& e)
    {
      _table.remove(e.getKey());
      return true;
    }

  private:
    T& _table;
  };

  /// Prints each matching row's getString() to std::cout. Used by
  /// CommonTable<>::dump() below.
  template<class Entry>
  class CommonDumpProcessor : public ResultsProcessor<Entry>
  {
  public:
    CommonDumpProcessor() {}
    virtual ~CommonDumpProcessor() {}

    virtual bool process(const Entry& entry)
    {
      std::cout << entry.getString() << "\n";
      return true;
    }
  };

  /**
   * CommonTable
   *
   * Common interface every concrete database table implements: query
   * with a key, query without one, and (provided here, not overridden)
   * dump(). CommonDataFactory defaults to DataFactory (see
   * DataFactory.h) but can be swapped out, e.g. by a test double.
   */
  template<class Key, class Entry, class CommonDataFactory = DataFactory>
  class CommonTable : public CommonDataFactory
  {
  public:
    explicit CommonTable(const std::string& tableName)
      : CommonDataFactory(tableName)
    {}

    virtual ~CommonTable() {}

    /// Move-only, matching DataFactory's own contract (see DataFactory.h):
    /// a table wraps a connection-like resource and shouldn't be silently
    /// duplicated. This has to be spelled out explicitly here (rather than
    /// left to the compiler) because CommonTable's own user-declared
    /// destructor above suppresses implicit generation of its move
    /// members -- without this, a helper like
    /// `Table freshTable() { Table t; ...; return t; }` would only
    /// compile by accident, whenever NRVO happens to apply.
    CommonTable(const CommonTable&) = delete;
    CommonTable& operator=(const CommonTable&) = delete;
    CommonTable(CommonTable&&) = default;
    CommonTable& operator=(CommonTable&&) = default;

    /// Query constrained to the row identified by `key`.
    virtual void query(const Key& key, ResultsProcessor<Entry>&) = 0;

    /// Unconstrained query: every row in the table.
    virtual void query(ResultsProcessor<Entry>&) = 0;

    virtual void dump();
  };

  template<class Key, class Entry, class CommonDataFactory>
  void CommonTable<Key, Entry, CommonDataFactory>::dump()
  {
    CommonDumpProcessor<Entry> processor;

    std::cout << this->getTableName() << " Dump:\n"
              << "===================================================\n";
    query(processor);
    std::cout << "===================================================\n";
  }

  /**
   * runQuery
   *
   * Drives a SELECT through to completion against `df`: executes `sql`,
   * converts each fetched row via `mapper`, and feeds it to
   * `rp.process()`, stopping early if that returns false. Always frees
   * the MySQL result set, including when process()/mapRow() throws.
   */
  template<class Entry>
  void runQuery(DataFactory& df,
                const std::string& sql,
                const RowMapper<Entry>& mapper,
                ResultsProcessor<Entry>& rp)
  {
    MYSQL_RES* result = df.executeQuery(sql);
    if (result == 0) return; // empty result set (e.g. an INSERT...RETURNING-less statement)

    try
    {
      bool more = true;
      MYSQL_ROW row;
      while (more && (row = mysql_fetch_row(result)) != 0)
      {
        unsigned long* lengths = mysql_fetch_lengths(result);
        Entry entry = mapper.mapRow(row, lengths);
        more = rp.process(entry);
      }
    }
    catch (...)
    {
      mysql_free_result(result);
      throw;
    }
    mysql_free_result(result);
  }

} // namespace MyCommon

#endif // MYTEMPLATE_DATA_H
