#pragma once

#include <map>
#include <chrono>
#include <string>
#include <functional>
#include <iostream>
#include <optional>
#include <regex>
#include <filesystem>
#include <thread>
#include <future>
#include <optional>

#include <cute/cute.h>

#include <cute/cute_listener.h>
#include <cute/cute_counting_listener.h>
#include <cute/ostream_listener.h>
#include <cute/ide_listener.h>
#include <cute/tap_listener.h>
#include <cute/xml_listener.h>

#include <flags.h>
#include <process.hpp>

#include "util.hpp"
#include "modern_listener.hpp"
#include "modern_xml_listener.hpp"

namespace tipi::cute_ext {
  using namespace std::string_literals;

  namespace detail {
    cute::null_listener dummy_null_listener{};

  }

  template <typename User_Supplied_Listener=cute::null_listener>
  class wrapper {
  private:

    
    bool opt_run_explicit = false;
    bool opt_show_help = false;
    bool opt_list_testcases = false;
    bool opt_skipped_as_pass = false;
    bool opt_exit_on_destruction = true;


    bool opt_auto_concurrent_run = true;
    bool opt_auto_concurrent_tc_set = true;
    std::string opt_auto_concurrent_unit{};
    std::string opt_auto_concurrent_suite{};

    std::string opt_filter_suite_value{};
    std::string opt_filter_unit_value{};

    size_t opt_maniac_strands_arg;


    /***/
    std::unordered_map<std::string, cute::suite> all_suites_{};
    flags::args args_;
    std::string program_exe_path;

    std::shared_ptr<std::fstream> file_out_ptr_;
    std::ostream* out_stream_ = &std::cout;

    User_Supplied_Listener &user_defined_listener_;
    bool use_user_defined_listener_;


    std::atomic<size_t> test_exec_failures = 0;

    std::function<bool(const std::string&)> make_filter_fn(const std::string& pattern) {
      
      auto filter = [pattern](const std::string &name) {
        const std::regex rx(pattern, std::regex_constants::icase);   
        
        std::smatch rx_match;
        return pattern.empty() || std::regex_search(name, rx_match, rx);
      };

      return std::move(filter);
    }

    void print_help() {
      auto &out = get_output();
      
      out << program_exe_path << " - usage\n" 
          << "\n"
          << "  --listener                  Choose the CUTE listener/formatter used\n"
          << "                              valid options are: 'ide', 'xml', 'classic', 'modern' (default)\n"
          << "\n"
          << "  --output                    Output stream to use\n"
          << "                              valid options are:\n"
          << "                               - 'console': write to standard output (default)\n"
          << "                               - 'cout': alias of 'console'\n"
          << "                               - 'error': write to standard error output'\n"
          << "                               - 'cerr': alias of 'error'\n"
          << "                               - <file-path>: write to file (truncates previously existing content!)\n"
          << "\n"
          << "  --list-testcases,-ltc       List the registered test cases\n"
          << "\n"
          << "  --filter=<search>           Filter test cases by name - <search> is intepreted as regex in search\n"
          << "                              mode (partial hit is filter match)\n"
          << "\n"
          << "  --filter-suite=<search>     Filter test suites by name - <search> is intepreted as regex in search\n"
          << "                              mode (partial hit is filter match)\n"
          << "\n"
          << "  --run                       Force run tests (after --list-testcases for example)\n"
          << "\n"                      
          << "  --parallel                  Parallelize the execution of the test cases using as many high concurrency\n"
          << "                              as the executing machine allows (n_hw_threads + 1)\n"
          << "\n"
          << "  -j=<N>                      Override the concurrency level for --parallel mode\n"
          << "\n"
          << "  --help,-h,-?                Print this help\n"
          << std::endl;
    }

    void print_all_tests() {

      auto& out = get_output();

      for(const auto &[i, s] : all_suites_) {
        out << "Suite: '" << i << "'\n";

        for(const auto &test_case : s) {
          out << " + " << test_case.name() << "\n";
        }

        out << "\n";
      }

      out << std::flush;
    }

    bool cmd_maniac_mode_stdin_runner() {
      auto filter_suite_enabled = make_filter_fn(opt_filter_suite_value);      
      
      std::string listing_raw{};
      std::getline(std::cin, listing_raw);
      auto test_cases = tipi::cute_ext::util::split(listing_raw, ';');
      
      // build the suite
      cute::suite msuite{};

      for(const auto &[info, suite] : all_suites_) {
        // respect --filter-suite
        if(filter_suite_enabled(info) == false) {
          continue;
        }
        
        for(const auto &test : suite) {
          if(std::find(test_cases.begin(), test_cases.end(), test.name()) != test_cases.end()) {
            msuite.push_back(test); 
          }
        }
      }

      auto msmap = std::unordered_map<std::string, cute::suite>{
        { "maniac suite", msuite }
      };

      bool success = true;

      try {
        success = run_suites(msmap);
      }
      catch(const std::exception& ex) {
        get_output() << "tipi cute_ext failed: " << ex.what() << "\n";
        success = false;
      }

      return success;
    }

    bool cmd_maniac() {
      auto filter_suite_enabled = make_filter_fn(opt_filter_suite_value);
      auto filter_unit_enabled = make_filter_fn(opt_filter_unit_value);

      // how many strands do we want
      size_t maniac_strands = opt_maniac_strands_arg;

      // subdivide all the existing suites into new ones (like a maniac)
      std::vector<cute::suite> maniac_suites{maniac_strands};

      for(int i = 0; i < maniac_strands; i++) {
        maniac_suites.push_back(cute::suite());
      }
      
      auto next_msuite_ix = 0;
      size_t output_count_number_tests = 0;

      for(const auto &[info, suite] : all_suites_) {
        
        // respect --filter-suite
        if(filter_suite_enabled(info) == false) {
          continue;
        }
        
        for(const auto &test_case : suite) {
          
          if(filter_unit_enabled(test_case.name())) {
            maniac_suites.at(next_msuite_ix++).push_back(test_case);
            output_count_number_tests++;
            if(next_msuite_ix >= maniac_strands) {
              next_msuite_ix = 0;
            }
          }

        }
      }        
      

        
      struct maniac_proc {
        bool done;
        bool success;
        std::shared_ptr<TinyProcessLib::Process> process_ptr;
        std::shared_ptr<std::stringstream> out_ss_ptr;

        maniac_proc(const std::string& command, const cute::suite& suite) 
          : done(false)
          , success(false)
          , out_ss_ptr{std::make_shared<std::stringstream>()}
        {
          std::stringstream cmd_ss{};

          #if defined(_WIN32) && !defined(MSYS_PROCESS_USE_SH)
          cmd_ss << "cmd /c ";
          #endif

          cmd_ss << command << " --maniac-list-stdin";

          auto loc_ss_ptr = out_ss_ptr;

          process_ptr = std::make_shared<TinyProcessLib::Process>(
            cmd_ss.str(), 
            "", /* env */
            [loc_ss_ptr](const char *bytes, size_t n) {
              auto &out = *loc_ss_ptr;
              out << std::string(bytes, n);
            },
            nullptr,  /* no explicit stderr handling in this case */
            true      /* open stdin */
          );

          for(auto &test : suite) {
            process_ptr->write(test.name());
            process_ptr->write(";");
          }

          process_ptr->write("\n");
          process_ptr->close_stdin();
        }
      };
      

      std::vector<maniac_proc> processes{};

      std::stringstream cmd_ss{};
      {
        cmd_ss << program_exe_path;

        const auto filter_test_case_arg = args_.get<std::string>("filter", "");
        if(filter_test_case_arg.empty() == false) {
          cmd_ss << " --filter=\"" << filter_test_case_arg << "\"";
        }

        const auto skipped_as_pass = args_.get<bool>("skipped-as-pass", false);
        if(skipped_as_pass) {
          cmd_ss << "--skipped-as-pass";
        }

        const auto listener_arg = args_.get<std::string>("listener", "");
        if(listener_arg.empty() == false) {
          cmd_ss << " --listener=\"" << listener_arg << "\"";
        }
      }

      size_t output_count_suites = 0;
      for(auto &msuite : maniac_suites) {

        if(msuite.size() == 0) {
          continue;
        }

        processes.push_back({ cmd_ss.str(), msuite });
        output_count_suites++;
      }

      // run and output stuff
      auto &out = get_output();
      out << get_maniac_output_wrapper_start(output_count_number_tests);

      bool success = true;
      auto ts_start = std::chrono::steady_clock::now();

      while(std::any_of(processes.begin(), processes.end(), [](auto &e) { return e.done == false; })) {
        for(maniac_proc& sd : processes) {

          if(!sd.done && sd.process_ptr) {
            int exit_code;

            if(sd.process_ptr->try_get_exit_status(exit_code)) {
              sd.done = true;
              sd.success = (exit_code == 0);
              out << sd.out_ss_ptr->str();                

              success = success && sd.success;                
            }
          }
        }
      }

      auto ts_end = std::chrono::steady_clock::now();

      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(ts_end - ts_start);
      size_t output_count_suites_success = std::count_if(processes.begin(), processes.end(), [](auto &sd) { return sd.success == true; });
      size_t output_count_suites_failed  = std::count_if(processes.begin(), processes.end(), [](auto &sd) { return sd.success == false; });

      out << get_maniac_output_wrapper_end(output_count_suites, output_count_suites_success, output_count_suites_failed, duration);

      return success;
    }

    bool cmd_run_base() {
      bool success = true;

      try {
        success = run_suites(all_suites_);
      }
      catch(const std::exception& ex) {
        get_output() << "tipi cute_ext failed: " << ex.what() << "\n";
        success = false;
      }

      if(!success) {
        test_exec_failures++;
      }

      return success;        
    }

  public:
    inline std::ostream& get_output() {
      if(file_out_ptr_) {
        return *file_out_ptr_;
      }
      return *out_stream_;
    }

    wrapper(int argc, char *argv[], bool exit_on_destruction = true)
      : wrapper(argc, argv, detail::dummy_null_listener, exit_on_destruction)
    {
      use_user_defined_listener_ = false;
    }

    wrapper(int argc, char *argv[], User_Supplied_Listener& user_defined_listener, bool exit_on_destruction = true)
      : args_(argc, argv)
      , use_user_defined_listener_(true)
      , user_defined_listener_(user_defined_listener)
      , opt_exit_on_destruction(exit_on_destruction)
    {
      program_exe_path = std::string(argv[0]);

      opt_filter_unit_value   = args_.get<std::string>("filter", "");
      opt_filter_suite_value  = args_.get<std::string>("filter-suite", "");
      opt_maniac_strands_arg  = args_.get<size_t>("j", std::thread::hardware_concurrency() + 1);

      opt_auto_concurrent_run = args_.get<bool>("parallel", false);
      opt_list_testcases      = args_.get<bool>("list-testcases") || args_.get<bool>("ltc");
      opt_show_help           = args_.get<bool>("help", false) || args_.get<bool>("h", false) || args_.get<bool>("?", false);
      opt_skipped_as_pass     = args_.get<bool>("skipped-as-pass", false);

      opt_auto_concurrent_tc_set    = args_.get<bool>("parallel-suite", false) && args_.get<bool>("parallel-unit", false);
      opt_auto_concurrent_suite     = args_.get<std::string>("parallel-suite", "");
      opt_auto_concurrent_unit      = args_.get<std::string>("parallel-unit", "");

      const auto out_arg = args_.get<std::string>("output", "cout");
      
      if(out_arg == "cout" || out_arg == "console") {
        out_stream_ = &std::cout;
      }
      else if(out_arg == "cerr" || out_arg == "error") {
        out_stream_ = &std::cerr;
      }
      else {
        std::filesystem::path output_path{out_arg};
        std::filesystem::create_directories(std::filesystem::absolute(output_path).parent_path());

        file_out_ptr_ = std::make_shared<std::fstream>(output_path, std::ios::out | std::ios::trunc | std::ios::in | std::ios::binary);

        if (!file_out_ptr_->is_open()) {
          throw std::runtime_error("Failed to open file "s + output_path.generic_string() + " for writing");
        }

        auto fs = file_out_ptr_.get();
        out_stream_ = fs;
      }


    }

    ~wrapper() {

      // list here
      if(opt_list_testcases) {
        print_all_tests();
      }

      if(opt_auto_concurrent_run) {
        process_cmd();
      }

      if(file_out_ptr_ && file_out_ptr_->is_open()) {
        get_output() << std::flush; 
        file_out_ptr_->close();
      }
      
      if(opt_exit_on_destruction) {
        if(test_exec_failures > 0) {
          std::exit(1);
        }

        std::exit(0);
      }
    }

    size_t get_failure_count() {
      return test_exec_failures;
    }

    bool process_cmd() {      

      if(opt_show_help) {
        print_help();
        std::exit(0);
      }


      if(opt_auto_concurrent_tc_set) {

        std::cout << "[X] MANIAC MODE SINGLE TC: " << opt_auto_concurrent_suite << " -->" << opt_auto_concurrent_unit << std::endl;
        return cmd_run_base();
      }

      // run the suite automatically if no additional params are provided OR and explicit --run is present
      if(opt_run_explicit || (!opt_run_explicit && !opt_list_testcases) ) {

        if(opt_auto_concurrent_run) {
          return cmd_maniac();
        }      

        return cmd_run_base();;
      }
    }
    
    void register_suite(const cute::suite& suite, const std::string& info) {
      if(opt_auto_concurrent_run || (opt_list_testcases && !opt_run_explicit)) {
        all_suites_.insert({ info, suite });
      }
      else {
        
        // in single-TC mode (as parallel mode child process) we skip every suite that is not the
        // expected one
        if(!opt_auto_concurrent_tc_set || info == opt_auto_concurrent_suite) {

          all_suites_ = std::unordered_map<std::string, cute::suite>{
            { info, suite }
          };

          process_cmd();

        }
      }     
    }

    void register_suite(const cute::suite&& suite, const std::string& info) {
        register_suite(suite, info);
    }

    template <typename Listener=cute::null_listener>
    bool run_templated(Listener & listener, const std::unordered_map<std::string, cute::suite> &suites) {

      auto filter_unit_enabled = make_filter_fn(opt_filter_unit_value);
      auto filter_suite_enabled = make_filter_fn(opt_filter_suite_value);

      auto run_unit = [&](const cute::test& t) {

        if(filter_unit_enabled(t.name()) == false && !opt_auto_concurrent_tc_set) {
          if(opt_skipped_as_pass) {
            listener.start(t);
            listener.success(t, "SKIPPED");
          }
          return true;
        }

        if(opt_auto_concurrent_tc_set && t.name() != opt_auto_concurrent_unit) {
          return true;
        }

        try {          
          listener.start(t);          
          t();
          listener.success(t, "OK");
          return true;
        } catch(const cute::test_failure & e){
          listener.failure(t, e);
        } catch(const std::exception & exc){
          listener.error(t, cute::demangle(exc.what()).c_str());
        } catch(std::string & s){
          listener.error(t, s.c_str());
        } catch(const char *&cs) {
          listener.error(t, cs);
        } catch(...) {
          listener.error(t, "unknown exception thrown");
        }

        test_exec_failures++;
        return false;
      };

      bool result = true;

      for(const auto &[suite_name, suite] : suites) {

        if(filter_suite_enabled(suite_name)) {

          if(!opt_auto_concurrent_tc_set) {
            listener.begin(suite, suite_name.c_str(), suite.size());
          }          

          for(auto &test : suite) {
            result &= run_unit(test);
          }
          
          if(!opt_auto_concurrent_tc_set) {
            listener.end(suite, suite_name.c_str());
          }
        }
        else {

          if(!opt_auto_concurrent_tc_set && opt_skipped_as_pass) {
            std::string skipped_suite_info = suite_name + " [SKIPPED]";
            listener.begin(suite, skipped_suite_info.c_str(), 0);
            listener.end(suite, skipped_suite_info.c_str());
          }
        }        
      }

      return result;
    }

    bool run_suites(const std::unordered_map<std::string, cute::suite> &suites, std::ostream& output_stream) {
      
      const auto listener_arg = args_.get<std::string>("listener", "modern");


      if(use_user_defined_listener_) {
        return run_templated<>(user_defined_listener_, suites);
      }
      else if(listener_arg == "ide") { 
        cute::ide_listener<> listener(output_stream); 
        return run_templated<>(listener, suites);
      }
      else if(listener_arg == "classic") {
        cute::ostream_listener<> listener(output_stream);
        return run_templated<>(listener, suites);
      }
      else if(listener_arg == "xml"){
        cute_ext::modern_xml_listener<> listener(output_stream, !opt_auto_concurrent_tc_set);
        return run_templated<>(listener, suites);
      }
      else if(listener_arg == "modern"){        
        cute_ext::modern_listener<> listener(!opt_auto_concurrent_tc_set, output_stream);
        return run_templated<>(listener, suites);
      }
      else {
        std::stringstream ss_err{};
        ss_err << "Unknown --listener option: " << listener_arg << " (valid options are: 'modern', 'classic', 'ide', 'xml')";
        throw std::runtime_error(ss_err.str());
      }
    }

    std::string get_maniac_output_wrapper_start(size_t no_of_tests) {
      
      const auto listener_arg = args_.get<std::string>("listener", "modern");

      if(listener_arg == "ide") { 
        return "\n";        
      }
      else if(listener_arg == "classic") {
        return "\n";
      }
      else if(listener_arg == "xml"){        
        return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites>\n"s;
      }
      else if(listener_arg == "modern"){        
        return "🏃🏃🏃🏃🏃 Starting MANIAC test run 🏃🏃🏃🏃🏃\n----------------------------------------------\n";
      }

      throw std::runtime_error("Unknown --listener option"); // should have thrown earlier, but anyways
    }

    std::string get_maniac_output_wrapper_end(
      size_t suite_count, 
      size_t suite_success, 
      size_t suite_failures, 
      std::chrono::milliseconds total_time
    ) {
      
      const auto listener_arg = args_.get<std::string>("listener", "modern");

      if(listener_arg == "ide") { 
        return "\n";        
      }
      else if(listener_arg == "classic") {
        return "\n";
      }
      else if(listener_arg == "xml"){        
        return "</testsuites>";
      }
      else if(listener_arg == "modern"){        
        std::stringstream out{};

        out << "----------------------------------------------\n"
            << "MANIAC test stats       " << ((suite_failures == 0) ? "🟢 PASS" : "🟥 FAILED") << "\n"
            << " - suites executed:     " << suite_count << "\n";

        if(suite_success == suite_count) { out << termcolor::green; } else { out << termcolor::reset; }
        out << " - suites pass:         " << suite_success << termcolor::reset << "\n";

        if(suite_failures > 0) {
          out << termcolor::red << " - suites failed:       " << suite_failures << termcolor::reset << "\n";
        }

        auto duration_s = std::chrono::duration_cast<std::chrono::duration<double>>(total_time);
        out << " - total duration:      " << duration_s.count() << "s\n" << std::endl;

        return out.str();
      }

      throw std::runtime_error("Unknown --listener option"); // should have thrown earlier, but anyways
    }

    bool run_suites(const std::unordered_map<std::string, cute::suite> &suites) { 
      return run_suites(suites, get_output());
    }

  };    
}