/* -*- C++ -*- */
//
// Dump.h
//

#ifndef MYTEMPLATE_DUMP_H
#define MYTEMPLATE_DUMP_H

#include <map>
#include <mutex>
#include <string>

namespace MyCommon {

  /// Base class for objects capable of producing a diagnostic dump.
  /// Concrete subclasses implement dump(); an instance may register
  /// itself with DiagnosticDumpRegistry so that a single call to the
  /// registry's dump() walks every live dumpable object in the process.
  class Dumpable
  {
  public:
    virtual ~Dumpable() {}

    /// Produce a diagnostic dump of this object.
    virtual void dump() const = 0;
  };

  /// Thread-safe, process-wide singleton registry of Dumpable objects.
  class DiagnosticDumpRegistry
  {
  public:
    static DiagnosticDumpRegistry* getInstance();

    /// Dump every registered object.
    void dump() const;

    /// Dump only the objects registered under `name`.
    void dump(const std::string& name) const;

    static void registerDumpable(const std::string& name, Dumpable* obj)
    { getInstance()->registerObject(name, obj); }

    static void deregisterDumpable(Dumpable* obj)
    { getInstance()->deregisterObject(obj); }

    void registerObject(const std::string& name, Dumpable* obj);

    /// Removes the first registration found for `obj`. A no-op if `obj`
    /// was never registered (e.g. CommonMap's copy constructor -- see
    /// CommonMap.h 
    void deregisterObject(Dumpable* obj);

    unsigned int getNumberOfRegisteredObjects() const
    { return _dMap.size(); }

  protected:
    DiagnosticDumpRegistry() {}
    DiagnosticDumpRegistry(const DiagnosticDumpRegistry&) = delete;
    DiagnosticDumpRegistry& operator=(const DiagnosticDumpRegistry&) = delete;

  private:
    typedef std::multimap<std::string, Dumpable*> DumpableMap;

    static DiagnosticDumpRegistry* _instance;
    static std::recursive_mutex _mutex;
    DumpableMap _dMap;
  };

} // namespace MyCommon

#endif // MYTEMPLATE_DUMP_H
