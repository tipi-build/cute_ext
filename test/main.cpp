
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

cute::suite make_suite_ReadOnlyIniFileTest()
{
    cute::suite s {};
    s.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "temp_0"));
    s.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "temp_1"));
    s.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "temp_2"));
    return s;
}


using namespace std::string_literals;

int main(int argc, char *argv[]){
    tipi::cute_ext::util::enable_vt100_support_windows10();
	tipi::cute_ext::wrapper wrapper(argc, argv);

    cute::suite s1{};
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s1_1"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s1_1"));

    cute::suite s2{};
    s2 += TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s2_0");
    
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s2_0");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s2_1");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s2_2");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s2_3");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s2_4");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s2_5");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s2_6");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s2_7");

    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s2_0");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s2_1");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s2_2");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s2_3");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s2_4");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s2_5");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s2_6");
    s2 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s2_7");


    cute::suite s3{};
    for(size_t i = 0; i < 200; i++) {
        std::string ctx = "s3_"s + std::to_string(i);
        s3 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, ctx.c_str());
    }

    s3 += TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s3_201");
    
    wrapper.register_suite(s1, "Suite 1");
    wrapper.register_suite(s2, "Suite 2");
    wrapper.register_suite(s3, "Suite 3");
    wrapper.register_suite(make_suite_ReadOnlyIniFileTest(), "Temp suite");

    try {
        wrapper.process_cmd();
    }
    catch(const std::exception &ex) {
        std::cout << "Failed to run\n" << ex.what() << std::endl;
        return -1;
    }
    
    return 0;
}