tipi::cute_ext - CUTEtest makeover
==================================

Making CUTEtest a bit... cuter?

Features
--------

* Beautiful output (with `--listener=modern` / by default if you pass a `tipi::cute_ext::modern_listener{}` to the cute runner)
* Test execution times
* Simple filtering (now regex based, so `--filter="something|does_not_thow"` *works*) 
* Filter tests-suites `--filter-suite`
* Listing test cases `--list-testcases` / `-ltc`
* Select the output formatter with `--listener`; valid options are: `ide`, `classic`, `classicxml`, `modern` (default), `modernxml` 
* Select output type with `--output`; valid options are: `console` / `cout` (default), `error` / `cerr` or `<file-path>` (truncates previously existing content!)
* `--help` / `-h` / `-?`
* 🚀 `--parallel` mode: brute-force accelerate the test execution by rearanging the test suites to fit the maximum hardware parallelity for the executing machine
    - `--parallel` (default) run all maniac suites by spwaning child processes as deemed necessary
    - `-j=<n-streads>` choose the level of concurrency if you like - defaults to `N hardware-threads + 1`

The output of `test/main --parallel --force-listener --listener=modern`

```bash
🏃Awesome testing with tipi.build + CUTE
 -> Starting test at: Thu Jan 26 13:41:26 2023 - [16747368865755976]
-------------------------------------------------------------------------------


 (....SNIP....)


 🧫 Suite 3
===============================================================================

 🧪 OutTests::anotherTest::s3_0             🟢 PASS          (884.69ms)
 🧪 OutTests::anotherTest::s3_1             🟢 PASS          (897.751ms)
 🧪 OutTests::anotherTest::s3_2             🟢 PASS          (897.342ms)
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

 (....SNIP....)


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

1. Add `tipi-build/cute_ext` to your `.tipi/deps`
2. cute\_ext is a drop-in replacement and replaces the original Petersommerlad/CUTE by wrapping it, no code changes required.
3. That's it : ` your-test.exe --force-listener --listener=modern --parallel`

🚀 Want speed ? Add  `--parallel` to the line.


```cpp
#include <cute/cute.h>

int main(int argc, const char *argv[])
{
    cute::xml_file_opener xmlfile(argc, argv);
    cute::xml_listener < cute::ide_listener<> > lis(xmlfile.out);

    auto runner = cute::makeRunner(lis, argc, argv);

   ...
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
using tipi::cute_ext; // Load the order of execution parameters

auto runner = cute::makeRunner(lis, argc, argv);

/**
 * @brief  Register a new suite and - depending on CLI arguments - run the suite immediately
 * @param suite the cute::suite to execute
 * @param name name of the suite
 * @param run_setting ext_run_setting::normal / ext_run_setting::before_all / ext_run_setting::after_all
 * @param force_linear set to true to force running this suite in linear mode
 */
 void runner::operator()(const cute::suite& suite, const std::string& name, ext_run_setting run_setting = ext_run_setting::normal, bool force_linear = false);

runner(suite_1, "Suite 1");  /* implicit, run_setting = ext_run_setting::normal, force_linear = false */
runner(suite_2, "Suite 2");
runner(suite_3, "Suite 3",   ext_run_setting::before_all,  false);
runner(suite_4, "Suite 4",   ext_run_setting::before_all,  false);
runner(suite_5, "Suite 5",   ext_run_setting::before_all,  true);
runner(suite_6, "Suite 6",   ext_run_setting::normal,      true);
runner(suite_7, "Suite 7",   ext_run_setting::normal,      false);
runner(suite_8, "Suite 8",   ext_run_setting::after_all,   false);
runner(suite_8, "Suite 9",   ext_run_setting::after_all,   false);
runner(suite_8, "Suite 10",  ext_run_setting::after_all,   false);
runner(suite_9, "Suite 11",  ext_run_setting::after_all,   true);
```


Advanced topics
---------------
  * [Controlling autoparallel child parameter and environment (gcov support)](docs/ADVANCED_CHILD_PROCESS_ENV_AND_PARAMS.md)


Licence
-------

The sources in this repository are currently proprietary software by tipi technologies AG and licensed to owner of tipi.build pro subcriptions.

CUTE test is licenced under the MIT licence and made by Peter Somerlad: https://github.com/PeterSommerlad/CUTE - see the respective LICENCE file.


