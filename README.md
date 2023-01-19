tipi::cute_ext - CUTEtest makeover
==================================

Making CUTEtest a bit... cuter?

Features
--------

- proper command line args handling
- beautiful output (with `--listener=modern` / by default) with execution times etc
- useful filtering (now regex based, so `--filter="something|does_not_thow"` *works*) 
- filter tests-suites `--filter-suite`
- listing test cases `--list-testcases` / `-ltc`
- select `cute::listerner` / formatter with `--listener`; valid options are: `ide`, `classic`, `xml`, `modern` (default)
- select output type with `--output`; valid options are: `console` / `cout` (default), `error` / `cerr` or `<file-path>` (truncates previously existing content!)
- `--help` / `-h` / `-?`
- 🚀 `--maniac` mode: brute-force accelerate the test execution by rearanging the test suites to fit the maximum hardware parallelity for the executing machine (has a bit of ☣️ potential depending on the test design and selected process model)
    - `--maniac=thread` (default) run all maniac suites in the same process on as many threads as deemed necessary
    - `--maniac=process` spwans child processes as deemed necessary

The output of `test/main --run`

```bash
1: 🧫 Test suite: Suite 1 (2 tests)
1:
1:  🧪 OutTests::mySimpleTest::s1_1            🟢 PASS    (56ms)
1:  🧪 OutTests::anotherTest::s1_1             🟢 PASS    (62ms)
1:
1:   | Suite   🟢 PASS (0.118707s)
1:   | Tests   2
1:   | Pass    2
1:
1: 🧫 Test suite: Suite 2 (20 tests)
1:
1:  🧪 OutTests::throwingtest::1               🟥 FAILED  (0ms)
1: -------------------------------------------------------------------------------
1: Unhandled exception:
1: Blah!
1: -------------------------------------------------------------------------------
1:  🧪 OutTests::anotherTest::2                🟢 PASS    (62ms)
1:  🧪 OutTests::mySimpleTest::3               🟢 PASS    (63ms)
1:  🧪 OutTests::anotherTest::4                🟢 PASS    (63ms)
1:  🧪 OutTests::mySimpleTest::5               🟢 PASS    (62ms)
1:  🧪 OutTests::anotherTest::6                🟢 PASS    (62ms)
1:  🧪 OutTests::mySimpleTest::7               🟢 PASS    (62ms)
1:  🧪 OutTests::anotherTest::8                🟢 PASS    (63ms)
1:  🧪 OutTests::mySimpleTest::9               🟢 PASS    (62ms)
1:  🧪 OutTests::anotherTest::10               🟢 PASS    (62ms)
1:  🧪 OutTests::mySimpleTest::11              🟢 PASS    (62ms)
1:  🧪 OutTests::anotherTest::12               🟢 PASS    (62ms)
1:  🧪 OutTests::mySimpleTest::13              🟢 PASS    (61ms)
1:  🧪 OutTests::anotherTest::14               🟢 PASS    (62ms)
1:  🧪 OutTests::failingtest::15               🟥 FAILED  (0ms)
1: -------------------------------------------------------------------------------
1: C:\.tipi\v3.w\37dda8d-cute-ext\test\main.cpp:29
1: OutTests::failingtest: 42 == 0 expected:     42      but was:        0
1: -------------------------------------------------------------------------------
1:  🧪 OutTests::anotherTest::16               🟢 PASS    (62ms)
1:  🧪 OutTests::mySimpleTest::17              🟢 PASS    (62ms)
1:  🧪 OutTests::anotherTest::18               🟢 PASS    (62ms)
1:  🧪 OutTests::mySimpleTest::19              🟢 PASS    (63ms)
1:  🧪 OutTests::anotherTest::20               🟢 PASS    (61ms)
1:
1:   | Suite   🟥 FAILED (1.12822s)
1:   | Tests   20
1:   | Pass    18
1:   | Failed  2
1:
1:
1: Test stats:
1:  - suites executed:     2
1:  - test cases executed: 22
1:  - total duration:      1.236s
1:
1: Result 🟥 FAILED
```

How-to
------

Add `tipi::cute_ext` to your `.tipi/deps` and adapt your test-executable entry point to use the `tipi::cute_ext::wrapper`

```cpp
#include <cute/cute.h>
#include <tipi_cute_ext.hpp>

/* [snip - test functions - see /test/main.cpp for full example]*/

int main(int argc, char *argv[]){
	tipi::cute_ext::wrapper wrapper(argc, argv);

    cute::suite s1{};
    s1.push_back(CUTE_SMEMFUN(OutTests, mySimpleTest));
    s1.push_back(CUTE_SMEMFUN(OutTests, anotherTest));
    wrapper.register_suite(s1, "Suite 1");
    // .... rince and repeat the above

    try {
        wrapper.process_cmd();
    }
    catch(const std::exception &ex) {
        std::cout << "Failed to run\n" << ex.what() << std::endl;
        return -1;
    }
    
    return 0;
}
```

Licence
-------

The sources in this repository are currently proprietarty software by tipi technologies AG.

CUTE test is licenced under the MIT licence and made by Peter Somerlad: https://github.com/PeterSommerlad/CUTE - see the respective LICENCE file.


