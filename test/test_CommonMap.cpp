/* -*- C++ -*- */
//
// test_CommonMap.cpp
//
// Unit tests for MyCommon::CommonMap<> / DumpableCommonMap<>.

#include "CommonMap.h"
#include "Dump.h"
#include "MiniTest.h"
#include <algorithm>
#include <sstream>
#include <streambuf>
#include <string>

using namespace MyCommon;

namespace {

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

void test_AddLookupRemove(MiniTest& t)
{
  CommonMap<int, std::string> map;

  MT_CHECK(t, map.addDataEntry(1, "one"));
  MT_CHECK(t, map.addDataEntry(2, "two"));
  MT_CHECK(t, !map.addDataEntry(1, "uno")); // duplicate key: rejected
  MT_CHECK(t, map.size() == 2);

  std::string data;
  MT_CHECK(t, map.lookupDataEntry(1, data));
  MT_CHECK(t, data == "one");

  MT_CHECK(t, !map.lookupDataEntry(99, data));
  MT_CHECK(t, data == "one"); // unmodified on miss

  map.removeDataEntry(1);
  MT_CHECK(t, map.size() == 1);
  MT_CHECK(t, !map.lookupDataEntry(1, data));
}

void test_KeyVectorTracksInsertAndRemove(MiniTest& t)
{
  CommonMap<int, std::string> map;
  map.addDataEntry(1, "one");
  map.addDataEntry(2, "two");
  map.addDataEntry(3, "three");

  MT_CHECK(t, map.getKeyVector().size() == 3);

  map.removeDataEntry(2);
  const CommonMap<int, std::string>::KeyVector& keys = map.getKeyVector();
  MT_CHECK(t, keys.size() == 2);
  MT_CHECK(t, std::find(keys.begin(), keys.end(), 2) == keys.end());
  MT_CHECK(t, std::find(keys.begin(), keys.end(), 1) != keys.end());
  MT_CHECK(t, std::find(keys.begin(), keys.end(), 3) != keys.end());
}

void test_ReloadReplacesContents(MiniTest& t)
{
  CommonMap<int, std::string> source;
  source.addDataEntry(10, "ten");
  source.addDataEntry(20, "twenty");

  CommonMap<int, std::string> target;
  target.addDataEntry(1, "stale");

  target.reload(source);

  MT_CHECK(t, target.size() == 2);
  std::string data;
  MT_CHECK(t, target.lookupDataEntry(10, data) && data == "ten");
  MT_CHECK(t, target.lookupDataEntry(20, data) && data == "twenty");
  MT_CHECK(t, !target.lookupDataEntry(1, data)); // old entry gone
  MT_CHECK(t, target.getKeyVector().size() == 2);
}

/// getDataMap(): both the const and the non-const overload.
void test_GetDataMap(MiniTest& t)
{
  CommonMap<int, std::string> map;
  map.addDataEntry(1, "one");

  const CommonMap<int, std::string>& constMap = map;
  const CommonMap<int, std::string>::DataMap& dataMap = constMap.getDataMap();
  MT_CHECK(t, dataMap.size() == 1);
  MT_CHECK(t, dataMap.at(1) == "one");

  // Non-const overload: caller can mutate the underlying std::map
  // directly through the returned reference.
  map.getDataMap()[2] = "two";
  MT_CHECK(t, map.size() == 2);
  std::string data;
  MT_CHECK(t, map.lookupDataEntry(2, data) && data == "two");
}

/// getMutex(): returns a reference callers can lock themselves. Default
/// Mutex is std::recursive_mutex, so locking it twice on the same
/// thread must not deadlock.
void test_GetMutex(MiniTest& t)
{
  CommonMap<int, std::string> map;

  std::lock_guard<std::recursive_mutex> outer(map.getMutex());

  bool reenteredWithoutDeadlock = false;
  {
    std::lock_guard<std::recursive_mutex> inner(map.getMutex());
    reenteredWithoutDeadlock = true;
  }
  MT_CHECK(t, reenteredWithoutDeadlock);
}

void test_CopyConstructorAndAssignment(MiniTest& t)
{
  CommonMap<int, std::string> a;
  a.addDataEntry(1, "one");

  CommonMap<int, std::string> b(a); // copy constructor
  MT_CHECK(t, b.size() == 1);

  CommonMap<int, std::string> c;
  c = a; // assignment
  MT_CHECK(t, c.size() == 1);

  // Independent afterwards.
  a.addDataEntry(2, "two");
  MT_CHECK(t, a.size() == 2);
  MT_CHECK(t, b.size() == 1);
  MT_CHECK(t, c.size() == 1);
}

/// Data type whose destructor is observable, so clearDataMap() (which
/// requires Data to be an owning pointer type) can be verified to
/// actually delete what it erases.
struct Counted
{
  explicit Counted(int* counter) : _counter(counter) { ++(*_counter); }
  ~Counted() { --(*_counter); }
  int* _counter;
};

void test_ClearDataMapDeletesPointerData(MiniTest& t)
{
  int liveCount = 0;
  CommonMap<int, Counted*> map;
  map.addDataEntry(1, new Counted(&liveCount));
  map.addDataEntry(2, new Counted(&liveCount));
  MT_CHECK(t, liveCount == 2);
  MT_CHECK(t, map.size() == 2);

  map.clearDataMap();
  MT_CHECK(t, liveCount == 0); // both Counted objects were deleted
  MT_CHECK(t, map.size() == 0);
}

void test_CommonMapDumpPrintsKeys(MiniTest& t)
{
  CommonMap<int, std::string> map;
  map.addDataEntry(42, "the answer");

  std::string output;
  {
    CoutCapture capture;
    map.dump();
    output = capture.str();
  }
  MT_CHECK(t, output.find("Key: 42") != std::string::npos);
}

void test_DumpableCommonMapDumpPrintsKeysAndValues(MiniTest& t)
{
  DumpableCommonMap<int, std::string> map;
  map.addDataEntry(7, "seven");

  std::string output;
  {
    CoutCapture capture;
    map.dump();
    output = capture.str();
  }
  MT_CHECK(t, output.find("Key: 7") != std::string::npos);
  MT_CHECK(t, output.find("Data: seven") != std::string::npos);
}

void test_RegistersWithDiagnosticDumpRegistry(MiniTest& t)
{
  DiagnosticDumpRegistry* registry = DiagnosticDumpRegistry::getInstance();
  unsigned int before = registry->getNumberOfRegisteredObjects();

  {
    CommonMap<int, std::string> map;
    MT_CHECK(t, registry->getNumberOfRegisteredObjects() == before + 1);
  }
  MT_CHECK(t, registry->getNumberOfRegisteredObjects() == before);
}

} // namespace

int main()
{
  MiniTest t("test_CommonMap");

  MT_RUN(t, test_AddLookupRemove);
  MT_RUN(t, test_GetDataMap);
  MT_RUN(t, test_GetMutex);
  MT_RUN(t, test_KeyVectorTracksInsertAndRemove);
  MT_RUN(t, test_ReloadReplacesContents);
  MT_RUN(t, test_CopyConstructorAndAssignment);
  MT_RUN(t, test_ClearDataMapDeletesPointerData);
  MT_RUN(t, test_CommonMapDumpPrintsKeys);
  MT_RUN(t, test_DumpableCommonMapDumpPrintsKeysAndValues);
  MT_RUN(t, test_RegistersWithDiagnosticDumpRegistry);

  return t.result();
}
