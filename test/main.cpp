
#include <cute/cute.h>
#include <cute/cute_runner.h>
#include <cute/ide_listener.h>
#include <cute/cute_test.h>
#include <cute/cute_equals.h>
#include <cute/cute_suite.h>

#include <tipi_cute_ext.hpp>
#include <util.hpp>

#include <iostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

class OutTests {

public:
    int lifeTheUniverseAndEverything = 6*7;

    void mySimpleTest(){
        std::this_thread::sleep_for(50ms);
        ASSERT_EQUAL(42, lifeTheUniverseAndEverything);
    }

    void failingtest(){
        ASSERT_EQUAL(42, 0);
    }

    
    void throwingtest(){
        throw std::runtime_error("Blah!");
    }

    int anotherTest(){
        std::this_thread::sleep_for(50ms);
        ASSERT_EQUAL(42, 42);
        return 0;
    }

};


int main(int argc, char *argv[]){
    tipi::cute_ext::util::enable_vt100_support_windows10();
	tipi::cute_ext::wrapper wrapper(argc, argv);

    cute::suite s1{};
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s1_1"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s1_1"));

    cute::suite s2{};
    s2 += TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "1");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "2");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "3");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "4");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "5");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "6");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "7");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "8");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "9");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "10");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "11");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "12");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "13");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "14");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, failingtest, "15");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "16");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "17");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "18");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "19");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "20");

    wrapper.register_suite(s1, "Suite 1");
    wrapper.register_suite(s2, "Suite 2");

    try {
        wrapper.process_cmd();
    }
    catch(const std::exception &ex) {
        std::cout << "Failed to run\n" << ex.what() << std::endl;
        return -1;
    }
    
    return 0;
}