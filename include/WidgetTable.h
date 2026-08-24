/* -*- C++ -*- */
//
// WidgetTable.h
//
// A worked example of the CommonKey1<>/CommonEntry<>/CommonTable<>
// pattern from Data.h, in the same shape as a real per-schema data
// header from the original project (key type, entry type, table type
// all in one file): a Key, an Entry, and a Table backed by a MySQL
// `widget` table:
//
//   CREATE TABLE widget (
//     id       INT PRIMARY KEY,
//     name     VARCHAR(255) NOT NULL,
//     category VARCHAR(255) NOT NULL,
//     quantity INT NOT NULL DEFAULT 0
//   );
//
// `category` and the constrained query(category, rp) overload exist to
// give CacheData.h's ConstrainedSingleVersionSnapshotCacheData<> (whose
// Criteria defaults to std::string) something real to filter by, the
// way the original filtered by SwitchId. getCacheKey() exists for
// CacheData.h's CommonCacheProcessor<>; here it's the same type/value
// as the table key (WidgetKey), which -- same as in the original -- is
// the common case, not a hard requirement (a cache key may instead be a
// projection of a compound table key).
//
// The SQL-building methods are `static` and public specifically so
// test/test_WidgetTable.cpp can verify the generated SQL text, and
// RowMapperImpl::mapRow() can be exercised with a hand-built MYSQL_ROW,
// without either one needing a live MySQL server.

#ifndef MYTEMPLATE_WIDGETTABLE_H
#define MYTEMPLATE_WIDGETTABLE_H

#include "CommonKey.h"
#include "Data.h"

namespace MyCommon {

  class WidgetKey : public CommonKey1<int32>
  {
  public:
    explicit WidgetKey(int32 id = 0) : CommonKey1<int32>(id) {}
    WidgetKey(const WidgetKey& k) : CommonKey1<int32>(k) {}
    virtual ~WidgetKey() {}

    int32 getId() const { return getA(); }
    void setId(int32 id) { setA(id); }

    virtual std::string getString() const;
  };

  class WidgetEntry : public CommonEntry<WidgetKey>
  {
  public:
    WidgetEntry()
      : CommonEntry<WidgetKey>(), _name(), _category(), _quantity(0)
    {}

    WidgetEntry(const WidgetKey& key, const std::string& name,
                const std::string& category, int32 quantity)
      : CommonEntry<WidgetKey>(key), _name(name), _category(category),
        _quantity(quantity)
    {}

    virtual ~WidgetEntry() {}

    const std::string& getName() const { return _name; }
    void setName(const std::string& name) { _name = name; }

    const std::string& getCategory() const { return _category; }
    void setCategory(const std::string& category) { _category = category; }

    int32 getQuantity() const { return _quantity; }
    void setQuantity(int32 quantity) { _quantity = quantity; }

    /// The key CacheData.h's CommonCacheProcessor<> indexes this entry
    /// under. Same type and value as the table key here -- see the file
    /// comment for when the two would differ.
    const WidgetKey& getCacheKey() const { return getKey(); }

    virtual std::string getString() const;

  private:
    std::string _name;
    std::string _category;
    int32 _quantity;
  };

  class WidgetTable : public CommonTable<WidgetKey, WidgetEntry>
  {
  public:
    WidgetTable() : CommonTable<WidgetKey, WidgetEntry>("widget") {}
    virtual ~WidgetTable() {}

    /// Same reasoning as CommonTable<>'s own move-only declarations
    /// (Data.h): this class's own destructor above again suppresses
    /// implicit move generation, so it has to be re-stated here too.
    WidgetTable(const WidgetTable&) = delete;
    WidgetTable& operator=(const WidgetTable&) = delete;
    WidgetTable(WidgetTable&&) = default;
    WidgetTable& operator=(WidgetTable&&) = default;

    void insert(const WidgetEntry& entry)
    { executeModificationSql(buildInsertSql(entry)); }

    void update(const WidgetEntry& entry)
    { executeModificationSql(buildUpdateSql(entry)); }

    void remove(const WidgetKey& key)
    { executeModificationSql(buildDeleteSql(key)); }

    virtual void query(const WidgetKey& key, ResultsProcessor<WidgetEntry>& rp)
    {
      RowMapperImpl mapper;
      runQuery(*this, buildSelectByKeySql(key), mapper, rp);
    }

    virtual void query(ResultsProcessor<WidgetEntry>& rp)
    {
      RowMapperImpl mapper;
      runQuery(*this, buildSelectAllSql(), mapper, rp);
    }

    /// Constrained query: only widgets in `category`. This is what
    /// ConstrainedSingleVersionSnapshotCacheData<> (CacheData.h) calls
    /// during reload().
    void query(const std::string& category, ResultsProcessor<WidgetEntry>& rp)
    {
      RowMapperImpl mapper;
      runQuery(*this, buildSelectByCategorySql(category), mapper, rp);
    }

    // SQL builders: pure functions of their arguments, no I/O. Public
    // so they can be unit tested directly.
    static std::string buildInsertSql(const WidgetEntry& entry);
    static std::string buildUpdateSql(const WidgetEntry& entry);
    static std::string buildDeleteSql(const WidgetKey& key);
    static std::string buildSelectByKeySql(const WidgetKey& key);
    static std::string buildSelectAllSql();
    static std::string buildSelectByCategorySql(const std::string& category);

    /// Column order for both the SELECT lists above and mapRow() below:
    /// id, name, category, quantity.
    class RowMapperImpl : public RowMapper<WidgetEntry>
    {
    public:
      virtual WidgetEntry mapRow(MYSQL_ROW row, unsigned long* lengths) const;
    };
  };

} // namespace MyCommon

#endif // MYTEMPLATE_WIDGETTABLE_H
