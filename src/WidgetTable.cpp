/* -*- C++ -*- */
//
// WidgetTable.cpp
//
// See WidgetTable.h.

#include "WidgetTable.h"
#include <cstdlib>
#include <sstream>

namespace MyCommon {

std::string
WidgetKey::getString() const
{
  std::ostringstream ost;
  ost << "[ WidgetKey: id=" << getId() << " ]";
  return ost.str();
}

std::string
WidgetEntry::getString() const
{
  std::ostringstream ost;
  ost << "[ WidgetEntry: " << getKey().getString()
      << ", name=" << _name
      << ", category=" << _category
      << ", quantity=" << _quantity
      << " ]";
  return ost.str();
}

std::string
WidgetTable::buildInsertSql(const WidgetEntry& entry)
{
  std::ostringstream sql;
  sql << "INSERT INTO widget (id, name, category, quantity) VALUES ("
      << entry.getKey().getId() << ", "
      << DataFactory::getNullableStringColumnValue(entry.getName()) << ", "
      << DataFactory::getNullableStringColumnValue(entry.getCategory()) << ", "
      << entry.getQuantity() << ")";
  return sql.str();
}

std::string
WidgetTable::buildUpdateSql(const WidgetEntry& entry)
{
  std::ostringstream sql;
  sql << "UPDATE widget SET name = "
      << DataFactory::getNullableStringColumnValue(entry.getName())
      << ", category = "
      << DataFactory::getNullableStringColumnValue(entry.getCategory())
      << ", quantity = " << entry.getQuantity()
      << " WHERE id = " << entry.getKey().getId();
  return sql.str();
}

std::string
WidgetTable::buildDeleteSql(const WidgetKey& key)
{
  std::ostringstream sql;
  sql << "DELETE FROM widget WHERE id = " << key.getId();
  return sql.str();
}

std::string
WidgetTable::buildSelectByKeySql(const WidgetKey& key)
{
  std::ostringstream sql;
  sql << "SELECT id, name, category, quantity FROM widget WHERE id = " << key.getId();
  return sql.str();
}

std::string
WidgetTable::buildSelectAllSql()
{
  return "SELECT id, name, category, quantity FROM widget";
}

std::string
WidgetTable::buildSelectByCategorySql(const std::string& category)
{
  std::ostringstream sql;
  sql << "SELECT id, name, category, quantity FROM widget WHERE category = "
      << DataFactory::getNullableStringColumnValue(category);
  return sql.str();
}

WidgetEntry
WidgetTable::RowMapperImpl::mapRow(MYSQL_ROW row, unsigned long* /*lengths*/) const
{
  WidgetEntry entry;
  entry.setKey(WidgetKey(row[0] == 0 ? 0 : std::atoi(row[0])));
  entry.setName(DataFactory::fetchNullableStringColumnValue(row[1]));
  entry.setCategory(DataFactory::fetchNullableStringColumnValue(row[2]));
  entry.setQuantity(DataFactory::fetchNullableIntegerColumnValue(row[3]));
  return entry;
}

} // namespace MyCommon
