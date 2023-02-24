tipi::cute_ext - CUTEtest makeover
==================================

Making CUTEtest a bit... cuter?

Features
--------

- proper command line args handling
- beautiful output (with `--listener=modern` / by default if you pass a `tipi::cute_ext::modern_listener{}` to the cute_ext runner) with execution times etc
- useful filtering (now regex based, so `--filter="something|does_not_thow"` *works*) 
- filter tests-suites `--filter-suite`
- listing test cases `--list-testcases` / `-ltc`
- select the output formatter with `--listener`; valid options are: `ide`, `classic`, `classicxml`, `modern` (default), `modernxml` 
- select output type with `--output`; valid options are: `console` / `cout` (default), `error` / `cerr` or `<file-path>` (truncates previously existing content!)
- `--help` / `-h` / `-?`
- 🚀 `--parallel` mode: brute-force accelerate the test execution by rearanging the test suites to fit the maximum hardware parallelity for the executing machine
    - `--parallel` (default) run all maniac suites by spwaning child processes as deemed necessary
    - `-j=<n-streads>` choose the level of concurrency if you like - defaults to `N hardware-threads + 1`

The output of `test/main --parallel --force-listener --listener=modern`

```bash
🏃Awesome testing with tipi.build + CUTE
 -> Starting test at: Thu Jan 26 13:41:26 2023 - [16747368865755976]
-------------------------------------------------------------------------------

 🧫 External suite
===============================================================================

 🧪 TestClass::Test1_pass                   🟢 PASS          (79.2724ms)
 🧪 TestClass::Test2_fail                   🟥 FAILED        (59.8359ms)
 :> [C:\.tipi\v3.w\40999a5-cute_ext\test\extern_testsuite.cpp:14]
 :> TestClass::Test2_fail: false

 🧪 TestClass::Test3_throws                 ❌ ERROR         (59.7406ms)
 :> --- uncaught std::invalid_argument
 :> --- exception message start ---
 :> invalid stoi argument
 :> --- exception message end ---


  | Suite     🟥 FAILED (0.143114s)
  | Tests     3
  | Pass      1
  | Failed    1
  | Errored   1


 🧫 Suite 1
===============================================================================

 🧪 OutTests::mySimpleTest::s1_1            🟢 PASS          (1.12498s)
 🧪 OutTests::anotherTest::s1_1             🟢 PASS          (131.336ms)
 🧪 OutTests::throwingtest::s1_1            ❌ ERROR         (90.4427ms)
 :> Hallo EH TDG ]]> blub
 :> --- uncaught std::runtime_error
 :> --- exception message start ---
 :> Blah!
 :> --- exception message end ---


  | Suite     🟥 FAILED (1.12841s)
  | Tests     3
  | Pass      2
  | Errored   1


 🧫 Suite 2
===============================================================================

 🧪 OutTests::throwingtest::s2_0            ❌ ERROR         (89.8572ms)
 :> Hallo EH TDG ]]> blub
 :> --- uncaught std::runtime_error
 :> --- exception message start ---
 :> Blah!
 :> --- exception message end ---

 🧪 OutTests::failingtest::s2_0             🟥 FAILED        (88.7723ms)
 :> [C:\.tipi\v3.w\40999a5-cute_ext\test\main.cpp:35]
 :> OutTests::failingtest: 42 == 1 expected:    42      but was:        1

 🧪 OutTests::mySimpleTest::s2_1            🟢 PASS          (1.13932s)
 🧪 OutTests::mySimpleTest::s2_2            🟢 PASS          (1.15636s)
 🧪 OutTests::mySimpleTest::s2_3            🟢 PASS          (1.17108s)
 🧪 OutTests::mySimpleTest::s2_4            🟢 PASS          (1.15599s)
 🧪 OutTests::mySimpleTest::s2_5            🟢 PASS          (1.18789s)
 🧪 OutTests::mySimpleTest::s2_6            🟢 PASS          (1.20534s)
 🧪 OutTests::mySimpleTest::s2_7            🟢 PASS          (1.20644s)
 🧪 OutTests::anotherTest::s2_0             🟢 PASS          (210.189ms)
 🧪 OutTests::anotherTest::s2_1             🟢 PASS          (225.256ms)
 🧪 OutTests::anotherTest::s2_2             🟢 PASS          (225.168ms)
 🧪 OutTests::anotherTest::s2_3             🟢 PASS          (225.043ms)
 🧪 OutTests::anotherTest::s2_4             🟢 PASS          (240.149ms)
 🧪 OutTests::anotherTest::s2_5             🟢 PASS          (181.295ms)
 🧪 OutTests::anotherTest::s2_6             🟢 PASS          (150.1ms)
 🧪 OutTests::anotherTest::s2_7             🟢 PASS          (166.148ms)

  | Suite     🟥 FAILED (1.21028s)
  | Tests     17
  | Pass      15
  | Failed    1
  | Errored   1


 🧫 Suite 3
===============================================================================

 🧪 OutTests::anotherTest::s3_0             🟢 PASS          (884.69ms)
 🧪 OutTests::anotherTest::s3_1             🟢 PASS          (897.751ms)
 🧪 OutTests::anotherTest::s3_2             🟢 PASS          (897.342ms)
 🧪 OutTests::anotherTest::s3_3             🟢 PASS          (882.282ms)
 🧪 OutTests::anotherTest::s3_4             🟢 PASS          (880.389ms)
 🧪 OutTests::anotherTest::s3_5             🟢 PASS          (866.919ms)
 🧪 OutTests::anotherTest::s3_6             🟢 PASS          (851.255ms)
 🧪 OutTests::anotherTest::s3_7             🟢 PASS          (835.717ms)
 🧪 OutTests::anotherTest::s3_8             🟢 PASS          (851.293ms)
 🧪 OutTests::anotherTest::s3_9             🟢 PASS          (171.145ms)
 🧪 OutTests::anotherTest::s3_10            🟢 PASS          (183.657ms)
 🧪 OutTests::anotherTest::s3_11            🟢 PASS          (156.977ms)
 🧪 OutTests::anotherTest::s3_12            🟢 PASS          (183.708ms)
 🧪 OutTests::anotherTest::s3_13            🟢 PASS          (199.112ms)
 🧪 OutTests::anotherTest::s3_14            🟢 PASS          (168.515ms)
 🧪 OutTests::anotherTest::s3_15            🟢 PASS          (168.402ms)
 🧪 OutTests::anotherTest::s3_16            🟢 PASS          (231.402ms)
 🧪 OutTests::anotherTest::s3_17            🟢 PASS          (230.886ms)
 (....SNIP....)
 🧪 OutTests::anotherTest::s3_193           🟢 PASS          (219.966ms)
 🧪 OutTests::anotherTest::s3_194           🟢 PASS          (201.506ms)
 🧪 OutTests::anotherTest::s3_195           🟢 PASS          (183.948ms)
 🧪 OutTests::anotherTest::s3_196           🟢 PASS          (185.702ms)
 🧪 OutTests::anotherTest::s3_197           🟢 PASS          (201.634ms)
 🧪 OutTests::anotherTest::s3_198           🟢 PASS          (188.536ms)
 🧪 OutTests::anotherTest::s3_199           🟢 PASS          (205.571ms)
 🧪 OutTests::throwingtest::s3_201          ❌ ERROR         (141.919ms)
 :> Hallo EH TDG ]]> blub
 :> --- uncaught std::runtime_error
 :> --- exception message start ---
 :> Blah!
 :> --- exception message end ---


  | Suite     🟥 FAILED (4.38208s)
  | Tests     201
  | Pass      200
  | Errored   1


 🧫 Temp suite
===============================================================================

 🧪 OutTests::mySimpleTest::temp_0          🟢 PASS          (1.14973s)
 🧪 OutTests::mySimpleTest::temp_1          🟢 PASS          (1.14541s)
 🧪 OutTests::mySimpleTest::temp_2          🟢 PASS          (1.14521s)

  | Suite     🟢 PASS (1.17398s)
  | Tests     3
  | Pass      3



Test stats:
 - suites executed:     5
 - suites passed:       1
 - suites failed:       4
 - test cases executed: 227
 - total test time:     63.0452s
 - total user time:     4.62761s

🟥 FAILED 🟥
```

How-to migrate from the original cute
--------------

1. Add `tipi::cute_ext` to your `.tipi/deps`
2. Add `#include <tipi_cute_ext.hpp>` and replace `cute::makeRunner` by `tipi::cute_ext::makeRunner`.
3. That's it run with nice ouput : ` your-test.exe --force-listener --listener=modern --parallel`

🚀 Want speed ? Add  `--parallel` to the line.


```cpp
#include <original/CUTE/cute/cute.h>
#include <tipi_cute_ext.hpp>

int main(int argc, const char *argv[])
{
    cute::xml_file_opener xmlfile(argc, argv);
    cute::xml_listener < cute::ide_listener<> > lis(xmlfile.out);

    auto runner = tipi::cute_ext::makeRunner(lis, argc, argv);

   ...
```

Work with the new API only
--------------------------

```cpp
#include <original/CUTE/cute/cute.h>
#include <tipi_cute_ext.hpp>

/* [snip - test functions - see /test/main.cpp for full example]*/

int main(int argc, char *argv[]){

  tipi::cute_ext::modern_listener lis{};
  auto runner = tipi::cute_ext::makeRunner(lis, argc, argv);


  cute::suite s1{};
  s1.push_back(CUTE_SMEMFUN(OutTests, mySimpleTest));
  s1.push_back(CUTE_SMEMFUN(OutTests, anotherTest));
  wrapper.register_suite(s1, "Suite 1");
  // .... rince and repeat the above

  // the cute_ext runner takes care to comply with the passed CLI args
  // and runs the required tests / sets process exit code as required by default
  //
  // different behavior can be set using the runner's public API
  return 0;
}
```

Controlling the test case execution in auto-parallel mode
---------------------------------------------------------

Executing the test suites in auto-parallel mode can uncover execution order dependencies and/or shared state issues, which can - in some cases - be due to the test design or system architecture.

To enable using the automatic parallelization on parts of the test suite only whitout creating additional test executables one can add a options to force executing singular suites in linear mode, and or forcing a point in time of execution (`tipi::cute_ext::ext_run_setting::before_all`, `tipi::cute_ext::ext_run_setting::normal` or `tipi::cute_ext::ext_run_setting::after_all`).

Forcing the execution to `force_linear` has the following behavior:

- waiting for all tests running in parallel mode to finish
- run the `force_linear`'ed suite one test case at a time, in the order their are added to the suite
- switch back to auto-parallel mode after all tests are finished

**NOTE:** The run settings only apply when running the test executable in `--parallel` mode.

- `tipi::cute_ext::ext_run_setting::normal`: default / no change
- `tipi::cute_ext::ext_run_setting::before_all`: the registered suite is executed before all `normal` ones. If multiple suites are registered as `before_all` the registration order in that subset is taken into account again.
- `tipi::cute_ext::ext_run_setting::after_all`: the registered suite is executed after all `normal` ones. If multiple suites are registered as `after_all` the registration order in that subset is taken into account again.


#### Order of execution example:

The order of execution of the following sample in auto-parallel mode would be

```
                    // ext_run_setting  ; force_linear
      S.3           // ::before_all     ; false
       |  S.4       // ::before_all     ; false
       \   /        //
        S.5         // ::before_all     ; *true*
       /   \        //
      S.1  |        // ::normal         ; false
       |  S.2       // ::normal         ; false
       \   /        // 
        S.6         // ::normal         ; *true*
        /|\         //
   /---+-+-+---\    //
   |   |   |   |    //
  S.7  |   |   |    //
   |  S.8  |   |    // ::after_all      ; false
   |   |  S.9  |    // ::after_all      ; false
   |   |   |  S.10  // ::after_all      ; false
   |   |   |   |    // 
   \---+-+-+---/    //
        \|/         // 
        S.11        // ::after_all      ; *true*
```

```cpp
// [snip] suites declaration
using tipi::cute_ext;

auto runner = cute_ext::makeRunner(lis, argc, argv);

/// @brief  Register a new suite and - depending on CLI arguments - run the suite immediately
/// @param suite the cute::suite to execute
/// @param name name of the suite
/// @param run_setting ext_run_setting::normal / ext_run_setting::before_all / ext_run_setting::after_all
/// @param force_linear set to true to force running this suite in linear mode
/// void register_suite(const cute::suite& suite, const std::string& name, ext_run_setting run_setting = ext_run_setting::normal, bool force_linear = false)

runner.register_suite(suite_1, "Suite 1");  /* implicit, run_setting = ext_run_setting::normal, force_linear = false */
runner.register_suite(suite_2, "Suite 2");
runner.register_suite(suite_3, "Suite 3",   ext_run_setting::before_all,  false);
runner.register_suite(suite_4, "Suite 4",   ext_run_setting::before_all,  false);
runner.register_suite(suite_5, "Suite 5",   ext_run_setting::before_all,  true);
runner.register_suite(suite_6, "Suite 6",   ext_run_setting::normal,      true);
runner.register_suite(suite_7, "Suite 7",   ext_run_setting::normal,      false);
runner.register_suite(suite_8, "Suite 8",   ext_run_setting::after_all,   false);
runner.register_suite(suite_8, "Suite 9",   ext_run_setting::after_all,   false);
runner.register_suite(suite_8, "Suite 10",  ext_run_setting::after_all,   false);
runner.register_suite(suite_9, "Suite 11",  ext_run_setting::after_all,   true);
```

Controlling auto-parallel child process parameters and environment
------------------------------------------------------------------

**cute_ext** launches one process per test unit in `--parallel` mode. In order to enable customizations to the start parameters and environment variables
presented to each of these child processes you can register a callback that enables modifying startup parameters:

```cpp
runner.set_on_before_autoparallel_child_process([](const auto& suite_name, const auto& unit_name, auto& args, auto& env) -> void {
  /**
   * suite_name and unit_name are the std::string values used during the registration of the suites/units
   */

  /**
   * args is the pre-populated command + arguments vector that is used to start the child process
   * 
   * /!\ each parameter should be inserted into its own entry
   */

  /**
   * env in an std::unsorted_map<std::string, std::string> that contains a copy of the environment that 
   * was presented to the currently running executable
   */

  /**
   * EXAMPLES:
   */
  
  // add CUSTOM_VARIABLE=42 to env
  env.emplace("CUSTOM_VARIABLE", "42");
  
  // add --custom-option VALUE to command line arguments
  args.push_back("--custom-option");
  args.push_back("VALUE");
});
```

As a concrete example, to enable generating a per-process `gcov` report:

```cpp

#include <filesystem>

#if defined(_WIN32)

#include <windows.h>
#include <processthreadsapi.h>

#else

#include <sys/types.h>
#include <unistd.h>

#endif


auto get_pid = []() -> size_t {
  #if defined(_WIN32)

  return ::GetCurrentProcessId();

  #else

  return getpid();

  #endif
};

auto clean_path = [](std::string path) {
  static std::string forbidden_path_chars( "!$%&()[]{}§*+#:?\"<>|" );
  std::transform(
    path.begin(), path.end(), path.begin(), 
    [&](char c) { return forbidden_path_chars.find(c) != std::string::npos ? '_' : c; }
  );

  return path;
};

auto current_pid = get_pid();

std::stringstream pathbase_ss{};

#if defined(_WIN32)
pathbase_ss << "C:/temp/"; 
#else
pathbase_ss << "/tmp/"; 
#endif

pathbase_ss << "cute_ext_cov_" << current_pid << "/";
std::string pathbase = pathbase_ss.str();
std::filesystem::create_directories(pathbase);

std::cout << "\n---\n" << "ℹ️ All coverage analysis files will be written to: " << pathbase << "\n---\n" << std::endl;

runner.set_on_before_autoparallel_child_process([&, pathbase](const auto& suite_name, const auto& unit_name, auto& args, auto& env) -> void {
    // place all the .gcda files as: /tmp/cute_ext_cov_{current_pid}/{suite_name}-{unit_name}/<gcov-generate-filename.gcda>
    std::stringstream path_ss{};
    path_ss << pathbase << clean_path(suite_name) << "-" << clean_path(unit_name); 

    std::string path = path_ss.str();
    std::filesystem::create_directories(path);           

    env.emplace("GCOV_PREFIX", path);  
    env.emplace("GCOV_PREFIX_STRIP", "10000");  // clear all gcov generated folders
});

```

Other helpers
-------------

```cpp
if(!runner.is_autoparallel_child()) { /* .... */ }
```

Licence
-------

The sources in this repository are currently proprietarty software by tipi technologies AG.

CUTE test is licenced under the MIT licence and made by Peter Somerlad: https://github.com/PeterSommerlad/CUTE - see the respective LICENCE file.


