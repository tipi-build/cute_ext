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

How-to
------

Add `tipi::cute_ext` to your `.tipi/deps` and adapt your test-executable entry point to use the `tipi::cute_ext::wrapper`

```cpp
#include <cute/cute.h>
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

Licence
-------

The sources in this repository are currently proprietarty software by tipi technologies AG.

CUTE test is licenced under the MIT licence and made by Peter Somerlad: https://github.com/PeterSommerlad/CUTE - see the respective LICENCE file.


