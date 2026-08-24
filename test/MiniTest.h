/* -*- C++ -*- */
//
// MiniTest.h
//
// A tiny, dependency-free test harness used across this project's tests.
// This header offers equivalent CHECK()/CHECK_NO_THROW() macros, plus
// MT_RUN() to run a named test function and report PASS/FAIL for it
// individually (not just an aggregate count of checks), with no
// dependency beyond <iostream>/<string>, so the test project builds
// standalone.

#ifndef MYTEMPLATE_MINITEST_H
#define MYTEMPLATE_MINITEST_H

#include <iostream>
#include <string>

namespace MyCommon {

  class MiniTest {
  public:
    explicit MiniTest(std::string name)
      : _name(std::move(name)),
        _passed(0), _failed(0),
        _casesRun(0), _casesFailed(0)
    {}

    ~MiniTest()
    {
      std::cout << "[" << _name << "] " << _casesRun << " test case(s): "
                << (_casesRun - _casesFailed) << " passed, " << _casesFailed
                << " failed (" << _passed << " checks passed, "
                << _failed << " checks failed)" << std::endl;
    }

    void check(bool cond, const char* expr, const char* file, int line)
    {
      if (cond) {
        ++_passed;
      } else {
        ++_failed;
        std::cout << "    " << file << ":" << line << ": FAILED: " << expr
                  << std::endl;
      }
    }

    /// Runs one named test function, immediately printing PASS/FAIL for
    /// it based on whether any check() inside it failed.
    template<class Fn>
    void run(const std::string& name, Fn fn)
    {
      int failedBefore = _failed;
      ++_casesRun;
      fn(*this);
      bool ok = (_failed == failedBefore);
      if (!ok) ++_casesFailed;
      std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << name
                << std::endl;
    }

    int result() const { return _failed == 0 ? 0 : 1; }

  private:
    std::string _name;
    int _passed;
    int _failed;
    int _casesRun;
    int _casesFailed;
  };

} // namespace MyCommon

#define MT_CHECK(t, expr) (t).check((expr), #expr, __FILE__, __LINE__)

#define MT_NO_THROW(t, stmt)                                                \
  do {                                                                      \
    try { stmt; (t).check(true, #stmt " (no throw)", __FILE__, __LINE__); } \
    catch (...) { (t).check(false, #stmt " (threw)", __FILE__, __LINE__); } \
  } while (0)

/// Runs `fn` (a `void fn(MiniTest&)` test function) and prints PASS/FAIL
/// for it by name.
#define MT_RUN(t, fn) (t).run(#fn, fn)

#endif // MYTEMPLATE_MINITEST_H
