/* -*- C++ -*- */
//
// test_Dump.cpp
//
// Unit tests for MyCommon::Dumpable / DiagnosticDumpRegistry.
// DiagnosticDumpRegistry is a process-wide singleton, so each test
// registers and deregisters its own objects within its own scope and
// checks the *change* in registered-object count, rather than an
// absolute count.

#include "Dump.h"
#include "MiniTest.h"
#include <sstream>
#include <streambuf>

using namespace MyCommon;

namespace {

class RecordingDumpable : public Dumpable
{
public:
  explicit RecordingDumpable(std::string label) : _label(std::move(label)) {}

  virtual void dump() const
  {
    std::cout << "RecordingDumpable(" << _label << ")\n";
  }

private:
  std::string _label;
};

/// Redirects std::cout into a string for the duration of its scope.
class CoutCapture
{
public:
  CoutCapture() : _old(std::cout.rdbuf(_buf.rdbuf())) {}
  ~CoutCapture() { std::cout.rdbuf(_old); }
  std::string str() const { return _buf.str(); }

private:
  std::ostringstream _buf;
  std::streambuf* _old;
};

void test_GetInstanceReturnsSameSingleton(MiniTest& t)
{
  DiagnosticDumpRegistry* a = DiagnosticDumpRegistry::getInstance();
  DiagnosticDumpRegistry* b = DiagnosticDumpRegistry::getInstance();
  MT_CHECK(t, a != 0);
  MT_CHECK(t, a == b);
}

/// registerObject()/deregisterObject() are the instance methods the
/// static registerDumpable()/deregisterDumpable() helpers forward to;
/// call them directly here so they're each exercised on their own, not
/// only indirectly through the static wrappers.
void test_RegisterObjectAndDeregisterObjectDirectly(MiniTest& t)
{
  DiagnosticDumpRegistry* registry = DiagnosticDumpRegistry::getInstance();
  unsigned int before = registry->getNumberOfRegisteredObjects();

  RecordingDumpable obj("direct");
  registry->registerObject("RecordingDumpable", &obj);
  MT_CHECK(t, registry->getNumberOfRegisteredObjects() == before + 1);

  registry->deregisterObject(&obj);
  MT_CHECK(t, registry->getNumberOfRegisteredObjects() == before);
}

void test_RegisterAndDeregisterChangesCount(MiniTest& t)
{
  DiagnosticDumpRegistry* registry = DiagnosticDumpRegistry::getInstance();
  unsigned int before = registry->getNumberOfRegisteredObjects();

  RecordingDumpable obj("a");
  DiagnosticDumpRegistry::registerDumpable("RecordingDumpable", &obj);
  MT_CHECK(t, registry->getNumberOfRegisteredObjects() == before + 1);

  DiagnosticDumpRegistry::deregisterDumpable(&obj);
  MT_CHECK(t, registry->getNumberOfRegisteredObjects() == before);
}

void test_DeregisterUnknownObjectIsNoop(MiniTest& t)
{
  DiagnosticDumpRegistry* registry = DiagnosticDumpRegistry::getInstance();
  unsigned int before = registry->getNumberOfRegisteredObjects();

  RecordingDumpable neverRegistered("never-registered");
  MT_NO_THROW(t, DiagnosticDumpRegistry::deregisterDumpable(&neverRegistered));
  MT_CHECK(t, registry->getNumberOfRegisteredObjects() == before);
}

void test_DumpInvokesRegisteredObjects(MiniTest& t)
{
  DiagnosticDumpRegistry* registry = DiagnosticDumpRegistry::getInstance();

  RecordingDumpable obj("dump-me");
  DiagnosticDumpRegistry::registerDumpable("RecordingDumpable", &obj);

  std::string output;
  {
    CoutCapture capture;
    registry->dump();
    output = capture.str();
  }
  MT_CHECK(t, output.find("RecordingDumpable(dump-me)") != std::string::npos);

  DiagnosticDumpRegistry::deregisterDumpable(&obj);
}

void test_DumpByNameFiltersObjects(MiniTest& t)
{
  DiagnosticDumpRegistry* registry = DiagnosticDumpRegistry::getInstance();

  RecordingDumpable wanted("wanted");
  RecordingDumpable other("other");
  DiagnosticDumpRegistry::registerDumpable("WantedGroup", &wanted);
  DiagnosticDumpRegistry::registerDumpable("OtherGroup", &other);

  std::string output;
  {
    CoutCapture capture;
    registry->dump("WantedGroup");
    output = capture.str();
  }
  MT_CHECK(t, output.find("RecordingDumpable(wanted)") != std::string::npos);
  MT_CHECK(t, output.find("RecordingDumpable(other)") == std::string::npos);

  DiagnosticDumpRegistry::deregisterDumpable(&wanted);
  DiagnosticDumpRegistry::deregisterDumpable(&other);
}

} // namespace

int main()
{
  MiniTest t("test_Dump");

  MT_RUN(t, test_GetInstanceReturnsSameSingleton);
  MT_RUN(t, test_RegisterObjectAndDeregisterObjectDirectly);
  MT_RUN(t, test_RegisterAndDeregisterChangesCount);
  MT_RUN(t, test_DeregisterUnknownObjectIsNoop);
  MT_RUN(t, test_DumpInvokesRegisteredObjects);
  MT_RUN(t, test_DumpByNameFiltersObjects);

  return t.result();
}
