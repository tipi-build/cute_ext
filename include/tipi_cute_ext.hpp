#pragma once

#include <algorithm>
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
#include <type_traits>

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
#include "parallel_listener.hpp"
#include "modern_xml_listener.hpp"
#include "cute_listener_wrapper.hpp"

namespace tipi::cute_ext {
  using namespace std::string_literals;

  namespace detail {
    cute::null_listener dummy_null_listener{};

    enum run_unit_result {
      passed,
      failed,   
      errored,
      skipped,
      unknown
    };

    int run_unit_result_to_ret(const run_unit_result val) {
      if(val == run_unit_result::passed) return 0;
      else if(val == run_unit_result::failed) return 1;
      else if(val == run_unit_result::errored) return 2;
      else if(val == run_unit_result::skipped) return 3;
      
      return 8;
    }

    static run_unit_result ret_to_run_unit_result(const int val) {
      if(val == 0) return run_unit_result::passed;
      else if(val == 1) return run_unit_result::failed;
      else if(val == 2) return run_unit_result::errored;
      else if(val == 3) return run_unit_result::skipped;
      return run_unit_result::unknown;
    }  
  }

  template <typename RunnerListener = cute::null_listener>
  class wrapper {
  private:

    RunnerListener& runner_listener_{};
    
    bool opt_run_explicit = false;
    bool opt_show_help = false;
    bool opt_list_testcases = false;
    bool opt_skipped_as_pass = false;
    bool opt_exit_on_destruction = false;
    bool opt_auto_concurrent_run = true;
    bool opt_auto_concurrent_tc_set = true;
    bool opt_force_cli_listener = false;

    std::string opt_listener{};
    std::string opt_auto_concurrent_unit{};
    std::string opt_auto_concurrent_suite{};

    std::string opt_filter_suite_value{};
    std::string opt_filter_unit_value{};

    size_t opt_maniac_strands_arg;

    /// @brief used to communicate the test case exit reason (pass: 0 / fail: 1 / error: 2)  in autoparallel child mode
    std::optional<int> force_destructor_exit_code;


    /***/
    std::unordered_map<std::string, cute::suite> all_suites_{};
    flags::args args_;
    std::string program_exe_path;

    std::shared_ptr<std::fstream> file_out_ptr_;
    std::ostream* out_stream_ = &std::cout;

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

    struct autoparallel_testcase {

      autoparallel_testcase(const std::string& base_cmd, const std::string &suite, const std::string &unit, const cute::test &cute_unit)
        : suite_(suite)
        , unit_(unit)
        , process_base_cmd_(base_cmd)
        , cute_unit_(cute_unit)
        , out_ss_ptr_{std::make_shared<std::stringstream>()}
      {          
      }

      ~autoparallel_testcase() {
        if(process_thread_ptr && process_thread_ptr->joinable()) {            
          process_thread_ptr->join();            
        }
      }

      void start(std::function<void(const autoparallel_testcase&)> cb) {
        // YS: not thread safe but idc for now
        if(!started_) {
          started_ = true; 
          running_ = true;
          process_thread_ptr = std::make_shared<std::thread>(&autoparallel_testcase::run_process, this, cb); 
        }
      }

      auto get_suite_name() const { return suite_; }
      auto get_unit_name() const { return unit_; }
      const cute::test& get_cute_unit() const { return cute_unit_; }

      bool is_running() const { return running_; }
      bool was_started() const { return started_; }
      bool is_done() const { return done_; }
      bool was_success() const { return success_; }

      tipi::cute_ext::detail::run_unit_result get_run_unit_result() const {
        auto rc = return_code();
        if(rc.has_value()) {
          return detail::ret_to_run_unit_result(rc.value());
        }

        return tipi::cute_ext::detail::run_unit_result::unknown;
      }

      std::optional<int> return_code() const {
        return process_exit_code;
      }
    
      std::string get_output() const {
        if(out_ss_ptr_ != nullptr){
          return out_ss_ptr_->str();
        }       
        
        return "";
      }

    private:      

      void run_process(std::function<void(const autoparallel_testcase&)> cb) {
        std::stringstream cmd_ss{};
        #if defined(_WIN32) && !defined(MSYS_PROCESS_USE_SH)
        cmd_ss << "cmd /c ";
        #endif

        cmd_ss << process_base_cmd_ << " --parallel-suite=\"" << suite_ << "\" --parallel-unit=\"" << unit_ << "\"";

        auto loc_ss_ptr = out_ss_ptr_;

        try { 
          auto processio = [loc_ss_ptr](const char *bytes, size_t n) {
            auto &out = *loc_ss_ptr;
            out << std::string(bytes, n);
          };

          process_ptr = std::make_shared<TinyProcessLib::Process>(
            cmd_ss.str(), 
            "", /* env */
            processio,  /*stdout */
            processio,  /*stderr */
            false     /* don't open stdin */
          );

          int exit_code;
          while(!process_ptr->try_get_exit_status(exit_code)) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
          }

          running_ = false;
          success_ = exit_code == 0;
          process_exit_code = exit_code;
        }
        catch(...) {
          running_ = false;
          success_ = false;
        }

        done_ = true;

        cb(*this);
      }


      bool started_ = false;
      bool running_ = false;
      bool done_    = false;
      bool success_ = false;
      std::string suite_;
      std::string unit_;
      std::string process_base_cmd_;
      const cute::test &cute_unit_;
      
      std::shared_ptr<std::stringstream> out_ss_ptr_;
      std::shared_ptr<TinyProcessLib::Process> process_ptr;
      std::optional<int> process_exit_code{};

      std::shared_ptr<std::thread> process_thread_ptr;
    };

    template <typename Listener>
    typename std::enable_if<!util::has_set_render_options_t<Listener>::value, void>::type
    listener_render_end(Listener & listener) {
      //
    }

    template <typename Listener>
    typename std::enable_if<util::has_set_render_options_t<Listener>::value, void>::type
    listener_render_end(Listener & listener) {
      listener.render_end();
    }

    template <typename Listener>
    typename std::enable_if<!util::has_set_render_options_t<Listener>::value, void>::type
    listener_render_preamble(Listener & listener) {
      //
    }

    template <typename Listener>
    typename std::enable_if<util::has_set_render_options_t<Listener>::value, void>::type
    listener_render_preamble(Listener & listener) {
      listener.render_preamble();
    }

    template <typename Listener>
    typename std::enable_if<!util::has_set_render_options_t<Listener>::value, bool>::type
    cmd_autoparallel_wrap(Listener & listener) {
      auto wrapped = cute_listener_wrapper(&listener);
      return cmd_autoparallel<>(wrapped);
    }

    template <typename Listener>
    typename std::enable_if<util::has_set_render_options_t<Listener>::value, bool>::type
    cmd_autoparallel_wrap(Listener & listener) {
      return cmd_autoparallel<>(listener);
    }

    template <typename Listener>
    bool cmd_autoparallel(Listener & listener) {

      auto filter_suite_enabled = make_filter_fn(opt_filter_suite_value);
      auto filter_unit_enabled = make_filter_fn(opt_filter_unit_value);

      // build the base command
      std::string base_process_cmd{};

      {
        std::stringstream cmd_ss{};
        cmd_ss << program_exe_path;

        if(opt_listener.empty() == false) {
        cmd_ss << " --listener=\"" << opt_listener << "\"";
        }

        if(opt_force_cli_listener) {
          cmd_ss << " --force-listener ";
        }

        base_process_cmd = cmd_ss.str();
      }

      // collect all the tests
      std::vector<autoparallel_testcase> tasks{};

      for(const auto &[info, suite] : all_suites_) {
        
        // respect --filter-suite
        if(filter_suite_enabled(info) == false) {
          continue;
        }
        
        for(const auto &test_case : suite) {

          // no need to spawn for disabled/skipped tests...
          if(filter_unit_enabled(test_case.name())) {
            tasks.emplace_back(base_process_cmd, info, test_case.name(), test_case);
          }
        }
      }

      /** HEPLPERS for the actual processing below */
      auto next_task_it = [&]() {
        return std::find_if(tasks.begin(), tasks.end(), [](auto &t) { return t.was_started() == false; });
      };

      auto tasks_remaining = [&]() {
        auto all_done = std::all_of(tasks.begin(), tasks.end(), [](auto &t) { return t.is_done(); });
        auto all_started = std::all_of(tasks.begin(), tasks.end(), [](auto &t) { return t.was_started(); });
        return !all_done || !all_started;
      };

      std::vector<std::string> suites_started{};
      auto mark_suite_started = [&](const std::string& suite_name) {
        if(std::find(suites_started.begin(), suites_started.end(), suite_name) != suites_started.end()) {

          auto suite = all_suites_[suite_name];
          listener.begin(suite, suite_name.c_str(), suite.size());

        }
      };
      
      std::atomic<size_t> tests_failed = 0;
      std::atomic<size_t> tests_running = 0;

      auto start_next = [&]() {
        auto nextit = next_task_it();

        if(nextit != tasks.end()) {
          tests_running++;

          nextit->start([&](const auto &task) {

            auto urr = task.get_run_unit_result();            

            if(urr == detail::run_unit_result::passed || urr == detail::run_unit_result::skipped) {
              listener.success(task.get_cute_unit(), "");
            }
            else if(urr == detail::run_unit_result::failed) {
              tests_failed++;
              listener.failure(task.get_cute_unit(), task.get_output().c_str());
            }
            else {
              tests_failed++;
              listener.error(task.get_cute_unit(), task.get_output().c_str());
            }

            tests_running--;
          });          

          auto suite_name = nextit->get_suite_name();
          mark_suite_started(suite_name);

          const cute::suite& suite = all_suites_.at(suite_name);
          listener.begin(suite, suite_name.c_str(), suite.size());
          listener.start(nextit->get_cute_unit(), suite);
          return true;
        }

        return false;
      };

      auto suite_finished = [&](const std::string& suite) {
        size_t count_tests  = std::count_if(tasks.begin(), tasks.end(), [&](auto &t) { return t.get_suite_name() == suite; });
        size_t count_done   = std::count_if(tasks.begin(), tasks.end(), [&](auto &t) { return t.get_suite_name() == suite && t.is_done(); });
        return count_tests == count_done;
      };

      std::vector<std::string> suites_printed{};
      

      auto was_suite_printed = [&](const std::string suite_name) {
        return std::find(suites_printed.begin(), suites_printed.end(), suite_name) != suites_printed.end();
      };

      auto all_suites_printed = [&]() {
        return suites_printed.size() == all_suites_.size();
      };

      auto print_suite = [&](const auto& suite, const std::string& suite_name) {
        suites_printed.push_back(suite_name);        
        listener.end(suite, suite_name.c_str());
      };


      /** The actual running thing */
      util::set_render_options(listener, true, true, true, false);
      listener.render_preamble();
      
      size_t strand_limit = ( opt_maniac_strands_arg < 1024) ? opt_maniac_strands_arg : 1024;

      while(tasks_remaining() && !all_suites_printed()) {

        // start as many tasks as we have "slots"
        while(tasks_remaining() && tests_running < strand_limit) {
          start_next();
        }        

        // collect the results until all tasks of a suite are finished
        for(const auto &[suite_name, suite] : all_suites_) {
          
          if(suite_finished(suite_name) && !was_suite_printed(suite_name)) {
            print_suite(suite, suite_name);
          }
        }
      }

      if(force_destructor_exit_code.value_or(0) == 0) { 
        force_destructor_exit_code = (tests_failed == 0) ? 0 : 1;
      }

      return tests_failed == 0;
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

      if(!success && !force_destructor_exit_code.has_value()) {
        force_destructor_exit_code = 1;
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

    wrapper(RunnerListener& listener, int argc, const char **argv, bool exit_on_destruction = true)
      : runner_listener_(listener)
      , opt_exit_on_destruction(exit_on_destruction)
      , args_(argc, const_cast<char **>(argv))
    {
      program_exe_path        = std::string(argv[0]);

      opt_listener            = args_.get<std::string>("listener", "modern");
      opt_filter_unit_value   = args_.get<std::string>("filter", "");
      opt_filter_suite_value  = args_.get<std::string>("filter-suite", "");
      opt_maniac_strands_arg  = args_.get<size_t>("j", std::thread::hardware_concurrency() + 1);

      opt_auto_concurrent_run = args_.get<bool>("parallel", false);
      opt_list_testcases      = args_.get<bool>("list-testcases") || args_.get<bool>("ltc");
      opt_show_help           = args_.get<bool>("help", false) || args_.get<bool>("h", false) || args_.get<bool>("?", false);
      opt_skipped_as_pass     = args_.get<bool>("skipped-as-pass", false);      
      opt_force_cli_listener  = args_.get<bool>("force-listener", false);   

      opt_auto_concurrent_tc_set    = args_.get<bool>("parallel-suite", false) || args_.get<bool>("parallel-unit", false);
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


        if(!opt_force_cli_listener) {
          listener_render_preamble(runner_listener_);
        }
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

      if(!opt_force_cli_listener) {
        listener_render_end(runner_listener_);
      }
      
      if(force_destructor_exit_code.has_value()) {
        std::exit(force_destructor_exit_code.value());
      }

/*
      if(file_out_ptr_ && file_out_ptr_->is_open()) {
        get_output() << std::flush; 
        file_out_ptr_->close();
      }
      
      if(opt_exit_on_destruction) {
        if(test_exec_failures > 0) {
          std::exit(1);
        }

        std::exit(0);
      }*/
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
        return cmd_run_base();
      }

      // run the suite automatically if no additional params are provided OR and explicit --run is present
      if(opt_run_explicit || (!opt_run_explicit && !opt_list_testcases) ) {

        if(opt_auto_concurrent_run) {

          auto& output_stream = get_output();

          if(!opt_force_cli_listener) {
            return cmd_autoparallel_wrap<RunnerListener>(runner_listener_);
          }
          else if(opt_listener == "ide") { 
            cute::ide_listener<> listener(output_stream); 
            return cmd_autoparallel_wrap<>(listener);
          }
          else if(opt_listener == "classic") {
            cute::ostream_listener<> listener(output_stream);
            return cmd_autoparallel_wrap<>(listener);
          }
          /* /!\ exe BOMB here? */
          else if(opt_listener == "classicxml"){
            cute::xml_listener<> listener(output_stream);
            return cmd_autoparallel_wrap<>(listener);
          }
          else if(opt_listener == "modernxml"){
            cute_ext::modern_xml_listener<> listener(output_stream);
            return cmd_autoparallel<>(listener);
          } //*/
          else if(opt_listener == "modern"){        
            cute_ext::modern_listener<> listener(output_stream);
            return cmd_autoparallel<>(listener);
          }
          else {
            std::stringstream ss_err{};
            ss_err << "Unknown --listener option: " << opt_listener << " (valid options are: 'modern', 'modernxml', 'classic', 'classicxml' and 'ide')";
            throw std::runtime_error(ss_err.str());
          }
        }      

        return cmd_run_base();;
      }

      return true;
    }
    
    /// @brief Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite cute::suite
    /// @param info name
    void register_suite(const cute::suite& suite, const std::string& name) {
      if(opt_auto_concurrent_run || (opt_list_testcases && !opt_run_explicit)) {
        all_suites_.insert({ name, suite });
      }
      else {        
        // in single-TC mode (as parallel mode child process) we skip every suite that is not the
        // expected one
        if(!opt_auto_concurrent_tc_set || name == opt_auto_concurrent_suite) {

          all_suites_ = std::unordered_map<std::string, cute::suite>{
            { name, suite }
          };

          process_cmd();
        }
      }     
    }

    /// @brief Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite cute::suite
    /// @param info name
    void register_suite(const cute::suite&& suite, const std::string& name) { register_suite(suite, name); }

    /// @brief Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite cute::suite
    /// @param info name
    void operator()(const cute::suite& suite, const std::string& name) { register_suite(suite, name); }

    /// @brief Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite cute::suite
    /// @param info name
    void operator()(const cute::suite&& suite, const std::string& name) { register_suite(suite, name); }
  
    template <typename FnListener>
    typename std::enable_if<!util::has_set_render_options_t<FnListener>::value, bool>::type
    run_templated_wrap(FnListener & listener, const std::unordered_map<std::string, cute::suite> &suites) {
      auto wrapped = cute_listener_wrapper(&listener);
      return run_templated<>(wrapped, suites);
    }

    template <typename FnListener>
    typename std::enable_if<util::has_set_render_options_t<FnListener>::value, bool>::type
    run_templated_wrap(FnListener & listener, const std::unordered_map<std::string, cute::suite> &suites) { 
      return run_templated<>(listener, suites);
    }


    template <typename FnListener=cute_ext::parallel_listener<>>
    bool run_templated(FnListener& listener, const std::unordered_map<std::string, cute::suite> &suites) {

      auto filter_unit_enabled = make_filter_fn(opt_filter_unit_value);
      auto filter_suite_enabled = make_filter_fn(opt_filter_suite_value);

      // if we are in concurrent / parallel mode, disable all funny rendering
      util::set_render_options(listener, !opt_auto_concurrent_tc_set, !opt_auto_concurrent_tc_set, !opt_auto_concurrent_tc_set, opt_auto_concurrent_tc_set);

      auto run_unit = [&](const cute::test& t) {

        if(filter_unit_enabled(t.name()) == false && !opt_auto_concurrent_tc_set) {
          if(opt_skipped_as_pass) {
            listener.start(t);
            listener.success(t, "SKIPPED");
          }
          return detail::run_unit_result::skipped;
        }

        if(opt_auto_concurrent_tc_set && t.name() != opt_auto_concurrent_unit) {
          return detail::run_unit_result::skipped;
        }

        detail::run_unit_result result;

        try {          
          listener.start(t);          
          t();
          listener.success(t, "OK");
          return detail::run_unit_result::passed;
        } catch(const cute::test_failure & e){
          listener.failure(t, e);
          result = detail::run_unit_result::failed;
        } catch(const std::exception & exc){
          listener.error(t, cute::demangle(exc.what()).c_str());
          result = detail::run_unit_result::errored;
        } catch(std::string & s){
          listener.error(t, s.c_str());
          result = detail::run_unit_result::errored;
        } catch(const char *&cs) {
          listener.error(t, cs);
          result = detail::run_unit_result::errored;
        } catch(...) {
          listener.error(t, "unknown exception thrown");
          result = detail::run_unit_result::errored;
        }

        
        return result;
      };

      bool result = true;

      for(const auto &[suite_name, suite] : suites) {

        if(filter_suite_enabled(suite_name)) {

          listener.begin(suite, suite_name.c_str(), suite.size());

          for(auto &test : suite) {

            auto unit_result = run_unit(test);

            if(unit_result == detail::run_unit_result::passed || unit_result == detail::run_unit_result::skipped) {
              result &= true;
            }
            else {
              test_exec_failures++;
              result &= false;
            }

            if(opt_auto_concurrent_tc_set && unit_result != detail::run_unit_result::skipped) {
              force_destructor_exit_code = run_unit_result_to_ret(unit_result);
            }
          }
          
          listener.end(suite, suite_name.c_str());
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
      
      if(!opt_force_cli_listener) {
        return run_templated_wrap<RunnerListener>(runner_listener_, suites);
      }
      else if(opt_listener == "ide") { 
        cute::ide_listener<> listener(output_stream);
        return run_templated_wrap<>(listener, suites);
      }
      else if(opt_listener == "classic") {
        cute::ostream_listener<> listener(output_stream);
        return run_templated_wrap<>(listener, suites);
      }
      //* // /!\ exe BOMB here?
      else if(opt_listener == "classicxml"){
        cute::xml_listener<> listener(output_stream);
        return run_templated_wrap<>(listener, suites);
      }
      else if(opt_listener == "modernxml"){
        cute_ext::modern_xml_listener<> listener(output_stream);
        return run_templated<>(listener, suites);
      } //*/
      else if(opt_listener == "modern"){
        cute_ext::modern_listener<> listener(output_stream);
        return run_templated<>(listener, suites);
      }
      else {
        std::stringstream ss_err{};
        ss_err << "XXX Unknown --listener option: " << opt_listener << " (valid options are: 'modern', 'modernxml', 'classic', 'classicxml' and 'ide')";
        throw std::runtime_error(ss_err.str());
      }
    }

    bool run_suites(const std::unordered_map<std::string, cute::suite> &suites) { 
      return run_suites(suites, get_output());
    }

  };    

  /// @brief Create a new cute_ext runner
  /// @tparam RunnerListener 
  /// @param listener
  /// @param argc 
  /// @param argv 
  /// @param exit_on_destruction 
  /// @return 
  template <typename RunnerListener = cute::null_listener>
  inline wrapper<RunnerListener> makeRunner(RunnerListener& listener, int argc, const char **argv, bool exit_on_destruction = true) {
    return wrapper<RunnerListener>(listener, argc, argv, exit_on_destruction);
  }

}