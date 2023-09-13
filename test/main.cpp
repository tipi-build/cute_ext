#include <cute/cute.h>
#include <cute/cute_runner.h>
#include <cute/ide_listener.h>
#include <cute/cute_test.h>
#include <cute/cute_equals.h>
#include <cute/cute_suite.h>

#include <util.hpp>

#include <parallel_listener.hpp>
#include <listener_wrapper.hpp>

#include <iostream>
#include <thread>
#include <chrono>

#include <termcolor/termcolor.hpp>

#if defined(_WIN32)

#include <windows.h>
#include <processthreadsapi.h>

#else

#include <sys/types.h>
#include <unistd.h>

#endif

using namespace std::chrono_literals;

extern cute::suite make_suite();

class OutTests {

public:
    int lifeTheUniverseAndEverything = 6*7;

    void mySimpleTest(){        
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        ASSERT_EQUAL(42, lifeTheUniverseAndEverything);
    }

    void mySimpleTestLong(){        
        //std::this_thread::sleep_for(std::chrono::seconds(5));
        ASSERT_EQUAL(42, lifeTheUniverseAndEverything);
    }

    void failingtest(){
        ASSERT_EQUAL(42, 1);
    }

    
    void throwingtest(){
        std::cout << "Hallo EH TDG ]]> blub " << std::endl;  
        throw std::runtime_error("Blah!");
    }

    int anotherTest(){
        //std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ASSERT_EQUAL(42, 42);
        return 0;
    }

};

tipi::cute_ext::ext_suite make_suite_ReadOnlyIniFileTest()
{
    tipi::cute_ext::ext_suite s{};
    s.run_setting = tipi::cute_ext::ext_run_setting::before_all;
    s.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "temp_0"));
    s.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "temp_1"));
    s.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "temp_2"));
    s.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTestLong, "temp_LONG2"));
    return s;
}

struct ynull_listener : public cute::null_listener { // defines Contract of runner parameter#

    static size_t cnt;

    size_t instanceCnt;

    ynull_listener() {

        instanceCnt = ynull_listener::cnt++;
        std::cout << "ynull_listener() / " << instanceCnt << " / " << this << std::endl;

    }

    ~ynull_listener() {
        std::cout << "~ynull_listener() / " << instanceCnt << std::endl;
    }
    
    void begin(cute::suite const &s, char const * info, size_t n_of_tests){
        std::cout << "ynull_listener::being() / " << this << " / " << info << std::endl;

    }
    void end(cute::suite const &, char const * /*info*/){}
    void start(cute::test const &){}
    void success(cute::test const &,char const * /*msg*/){}
    void failure(cute::test const &, cute::test_failure const &){}
    void error(cute::test const &,char const * /*what*/){}
};

size_t ynull_listener::cnt = 0;

using namespace std::string_literals;

void testfn(int argc, const char **argv) {
    tipi::cute_ext::util::enable_vt100_support_windows10();    

    //cute::xml_file_opener xmlfile(argc, argv);
    //tipi::cute_ext::modern_xml_listener < tipi::cute_ext::modern_listener<> > lis{xmlfile.out};    
    tipi::cute_ext::modern_listener lis{};
    //tipi::cute_ext::modern_xml_listener lis{};
    //cute::xml_listener<> lis{std::cout};    
	//cute::ide_listener<> lis{std::cout};    
	//cute::ostream_listener<> lis{std::cout};    
    //ynull_listener lis{};
	
    auto runner = cute::makeRunner(lis, argc, argv);


    if(!runner.is_autoparallel_child()) {
    
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
        boost::filesystem::create_directories(pathbase);

        std::cout << "\n---\n" << "ℹ️ All coverage analysis files will be written to: " << pathbase << "\n---\n" << std::endl;

        runner.set_on_before_autoparallel_child_process([&, pathbase](const auto& suite_name, const auto& unit_name, auto& args, auto& env) -> void {
            // place all the .gcda files as: /tmp/cute_ext_cov_{current_pid}/{suite_name}-{unit_name}/<gcov-generate-filename.gcda>
            std::stringstream path_ss{};
            path_ss << pathbase << clean_path(suite_name) << "-" << clean_path(unit_name); 

            std::string path = path_ss.str();
            boost::filesystem::create_directories(path);           

            env.emplace("GCOV_PREFIX", path);  
            env.emplace("GCOV_PREFIX_STRIP", "10000");  // clear all gcov generated folders
        });

    }
    else {
        std::cout << "ENV ℹ️ : ";
        auto env_gcov_prefix = std::getenv("GCOV_PREFIX");
        if(env_gcov_prefix) {
            std::cout << "GCOV_PREFIX=" << env_gcov_prefix << " ";
        }

        auto env_gcov_prefix_strip = std::getenv("GCOV_PREFIX_STRIP");

        if(env_gcov_prefix_strip) {
            std::cout << "GCOV_PREFIX_STRIP=" << env_gcov_prefix_strip << " ";
        }

        std::cout << "(END)" << std::endl;
    }

    cute::suite s1{};
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, mySimpleTest, "s1_1"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, anotherTest, "s1_1"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_1"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_2"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_3"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_4"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_5"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_6"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_7"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_8"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_9"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_10"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_11"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_12"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_13"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_14"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_15"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_16"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_17"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_18"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_19"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_20"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_21"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_22"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_23"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_24"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_25"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_26"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_27"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_28"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_29"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_30"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_31"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_32"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_33"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_34"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_35"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_36"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_37"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_38"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_39"));
    s1.push_back(TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s1_40"));
    

    cute::suite s2{};
    s2 += TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s2_0");
    
    s2 += TIPI_CUTE_SMEMFUN(OutTests, failingtest, "s2_0");
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
    for(size_t i = 0; i <= 200; i++) {
        std::string ctx = "s3_"s + std::to_string(i);
        s3 += TIPI_CUTE_SMEMFUN(OutTests, anotherTest, ctx.c_str());
    }

    s3 += TIPI_CUTE_SMEMFUN(OutTests, throwingtest, "s3_201");
    
    runner(s2, "Linear Suite AFTER_ALL", tipi::cute_ext::ext_run_setting::after_all, true);
    runner(s3, "Suite 3");
    runner(make_suite(), "LINEAR External suite ", tipi::cute_ext::ext_run_setting::normal, true);  
    runner(make_suite_ReadOnlyIniFileTest(), "Temp suite");
    runner(make_suite(), "External suite");  
    
    runner(s1, "Linear Suite BEFORE_ALL", tipi::cute_ext::ext_run_setting::before_all, true);
    
    /*try {
        wrapper.process_cmd();
    }
    catch(const std::exception &ex) {
        std::cout << "Failed to run\n" << ex.what() << std::endl;
        return -1;
    }*/
}

int main(int argc, const char **argv) {
    testfn(argc, argv);
    std::cerr << "----------DONE---------------" << std::endl;
    return 0;
}
