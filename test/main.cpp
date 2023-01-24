#include <cute/cute.h>
#include <cute/cute_runner.h>
#include <cute/ide_listener.h>
#include <cute/cute_test.h>
#include <cute/cute_equals.h>
#include <cute/cute_suite.h>

#include <tipi_cute_ext.hpp>
#include <util.hpp>

#include <parallel_listener.hpp>
#include <cute_listener_wrapper.hpp>

#include <iostream>
#include <thread>
#include <chrono>

#include <termcolor/termcolor.hpp>

using namespace std::chrono_literals;

extern cute::suite make_suite();

class OutTests {

public:
    int lifeTheUniverseAndEverything = 6*7;

    void mySimpleTest(){
        std::this_thread::sleep_for(50ms);
        ASSERT_EQUAL(42, lifeTheUniverseAndEverything);
    }

    void failingtest(){
        ASSERT_EQUAL(42, 1);
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

int main(int argc, char *argv[]) {
    tipi::cute_ext::util::enable_vt100_support_windows10();    

    cute::xml_file_opener xmlfile(argc, argv);
    //tipi::cute_ext::modern_xml_listener < tipi::cute_ext::modern_listener<> > lis{xmlfile.out};    
    //tipi::cute_ext::modern_listener<> lis{};
    tipi::cute_ext::modern_xml_listener<> lis{};
    //cute::xml_listener<> lis{std::cout};    
	//cute::ide_listener<> lis{std::cout};    
	//cute::ostream_listener<> lis{std::cout};    
	
    auto runner = tipi::cute_ext::makeRunner(lis, argc, argv);

    cute::suite s1{};
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s1_1"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s1_1"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_1"));

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
    for(size_t i = 0; i < 20; i++) {
        std::string ctx = "s3_"s + std::to_string(i);
        s3 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, ctx.c_str());
    }

    s3 += TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s3_201");
    
    runner(s1, "Suite 1");
    runner(s2, "Suite 2");
    runner(s3, "Suite 3");
    runner(make_suite_ReadOnlyIniFileTest(), "Temp suite");
    runner(make_suite(), "External suite");    

    /*try {
        wrapper.process_cmd();
    }
    catch(const std::exception &ex) {
        std::cout << "Failed to run\n" << ex.what() << std::endl;
        return -1;
    }*/

  

    if(runner.get_failure_count() > 0) {
        return 1;
    }
    
    return 0;
}