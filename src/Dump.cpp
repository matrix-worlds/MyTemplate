/* -*- C++ -*- */
//
// Dump.cpp
//

#include "Dump.h"
#include <iostream>

namespace MyCommon {

DiagnosticDumpRegistry* DiagnosticDumpRegistry::_instance = 0;
std::recursive_mutex DiagnosticDumpRegistry::_mutex;

DiagnosticDumpRegistry*
DiagnosticDumpRegistry::getInstance()
{
  if (_instance == 0)
  {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    if (_instance == 0)
    {
      _instance = new DiagnosticDumpRegistry;
    }
  }
  return _instance;
}

void
DiagnosticDumpRegistry::dump() const
{
  std::lock_guard<std::recursive_mutex> guard(_mutex);

  std::cout << "\n------------------------------------------------\n";
  for (DumpableMap::const_iterator i = _dMap.begin(); i != _dMap.end(); ++i)
  {
    std::cout << "- - - - - - - - - - - - - - - -\n"
              << "Name: \"" << i->first << "\", object: " << i->second << "\n";
    i->second->dump();
  }
  std::cout << "------------------------------------------------\n";
}

void
DiagnosticDumpRegistry::dump(const std::string& name) const
{
  std::lock_guard<std::recursive_mutex> guard(_mutex);

  std::cout << "\n------------------------------------------------\n";
  for (DumpableMap::const_iterator i = _dMap.find(name);
       i != _dMap.end() && i->first == name; ++i)
  {
    std::cout << "- - - - - - - - - - - - - - - -\n"
              << "Name: \"" << i->first << "\", object: " << i->second << "\n";
    i->second->dump();
  }
  std::cout << "------------------------------------------------\n";
}

void
DiagnosticDumpRegistry::registerObject(const std::string& name, Dumpable* obj)
{
  std::lock_guard<std::recursive_mutex> guard(_mutex);
  _dMap.insert(DumpableMap::value_type(name, obj));
}

void
DiagnosticDumpRegistry::deregisterObject(Dumpable* obj)
{
  std::lock_guard<std::recursive_mutex> guard(_mutex);
  for (DumpableMap::iterator i = _dMap.begin(); i != _dMap.end(); ++i)
  {
    if (i->second == obj)
    {
      _dMap.erase(i);
      return;
    }
  }
}

} // namespace MyCommon
