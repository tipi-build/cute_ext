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
#include <mutex>
#include <future>
#include <optional>
#include <type_traits>
#include <typeinfo>
#include <set>

#include <original/CUTE/cute/cute.h>

#include <original/CUTE/cute/cute_listener.h>
#include <original/CUTE/cute/cute_counting_listener.h>
#include <original/CUTE/cute/ostream_listener.h>
#include <original/CUTE/cute/ide_listener.h>
#include <original/CUTE/cute/tap_listener.h>
#include <original/CUTE/cute/xml_listener.h>

#include <flags.h>
#include <process.hpp>

#include "util.hpp"
#include "ext_listener.hpp"
#include "parallel_listener.hpp"
#include "modern_listener.hpp"
#include "modern_xml_listener.hpp"
#include "listener_wrapper.hpp"

#ifndef TIPI_CUTE_EXT
#define TIPI_CUTE_EXT
#endif 

namespace tipi::cute_ext {
  using namespace std::string_literals;

  enum ext_run_setting {
    normal,
    before_all,
    after_all
  };

  class ext_suite : public cute::suite {
  public:

    ext_run_setting run_setting;
    bool force_linear;

    ext_suite(const ext_suite& other) 
      : cute::suite(other)
      , run_setting(other.run_setting)
      , force_linear(other.force_linear)
    { }

    ext_suite(ext_suite&& other) noexcept 
      : cute::suite(std::move(other))
      , run_setting(other.run_setting)
      , force_linear(other.force_linear)
    { }
    
    ext_suite(ext_run_setting r = ext_run_setting::normal, bool force_linear = false) 
      : run_setting(r)
      , force_linear(force_linear)
    { }

    ext_suite(const cute::suite &suite, ext_run_setting r = ext_run_setting::normal, bool force_linear = false)
      : cute::suite(suite)
      , run_setting(r)
      , force_linear(force_linear)
    { }      
  };

  namespace detail {

    enum run_unit_result {
      passed,
      failed,   
      errored,
      skipped,
      unknown
    };

    inline int run_unit_result_to_ret(const run_unit_result val) {
      if(val == run_unit_result::passed) return 0;
      else if(val == run_unit_result::failed) return 1;
      else if(val == run_unit_result::errored) return 2;
      else if(val == run_unit_result::skipped) return 3;
      
      return 8;
    }

    inline run_unit_result ret_to_run_unit_result(const int val) {
      if(val == 0) return run_unit_result::passed;
      else if(val == 1) return run_unit_result::failed;
      else if(val == 2) return run_unit_result::errored;
      else if(val == 3) return run_unit_result::skipped;
      return run_unit_result::unknown;
    }

    template <typename app_listener_T = cute::null_listener>
    struct wrapper_options {
    private:
      flags::args args_;

      std::ostream* out_stream_ = &std::cout;
      std::shared_ptr<std::fstream> file_out_ptr_;
      std::shared_ptr<ext_listener> listener_ptr_;
    public:

      std::string program_exe_path{};

      bool run_explicit = false;
      bool show_help = false;
      bool list_testcases = false;
      bool skipped_as_pass = false;
      bool exit_on_destruction = false;

      bool parallel_run = true;
      bool parallel_child_run = false;
      bool force_cli_listener = false;

      std::string listener_arg{};
      std::string listener_out{};
      std::string auto_concurrent_unit{};
      std::string auto_concurrent_suite{};

      std::string filter_suite_value{};
      std::string filter_unit_value{};

      size_t parallel_strands_arg = 2;

      inline std::ostream& get_output() {
        return *out_stream_;
      }

      wrapper_options(app_listener_T& listener, int argc, const char **argv, bool exit_on_destruction = true)
        : args_(argc, const_cast<char **>(argv))
        , exit_on_destruction(exit_on_destruction)
      {
        program_exe_path      = std::string(argv[0]);

        listener_arg          = args_.get<std::string>("listener", "modern");
        listener_out          = args_.get<std::string>("output", "cout");
        force_cli_listener    = args_.get<bool>("force-listener", false);   
        
        filter_unit_value     = args_.get<std::string>("filter", "");
        filter_suite_value    = args_.get<std::string>("filter-suite", "");
        
        parallel_strands_arg  = args_.get<size_t>("j", std::thread::hardware_concurrency() + 1);
        parallel_run          = args_.get<bool>("parallel", false);
        run_explicit          = args_.get<bool>("run", false);
        
        list_testcases        = args_.get<bool>("list-testcases") || args_.get<bool>("ltc");
        show_help             = args_.get<bool>("help", false) || args_.get<bool>("h", false) || args_.get<bool>("?", false);
        skipped_as_pass       = args_.get<bool>("skipped-as-pass", false);      

        parallel_child_run    = args_.get<bool>("parallel-suite", false) || args_.get<bool>("parallel-unit", false);
        auto_concurrent_suite = args_.get<std::string>("parallel-suite", "");
        auto_concurrent_unit  = args_.get<std::string>("parallel-unit", "");

        if(listener_out == "cout" || listener_out == "console") {
          out_stream_ = &std::cout;
          std::cout << termcolor::colorize;
        }
        else if(listener_out == "cerr" || listener_out == "error") {
          out_stream_ = &std::cerr;
          std::cout << termcolor::colorize;
        }
        else {
          auto nowTs = std::to_string(std::time(nullptr));
          auto execPath = std::filesystem::path(program_exe_path);


          std::map<std::string, std::string> placeholders_replacements = {
            { "{executable}", execPath.filename().generic_string() },
            { "{timestamp}", nowTs }
          };

          for(const auto& [search, replace] : placeholders_replacements) {

            size_t findIx = listener_out.find(search);

            if(findIx != std::string::npos) {
              listener_out.replace(findIx, search.length(), replace);
            }
          }

          std::filesystem::path output_path{listener_out};
          std::filesystem::create_directories(std::filesystem::absolute(output_path).parent_path());

          file_out_ptr_ = std::make_shared<std::fstream>(output_path, std::ios::out | std::ios::trunc | std::ios::in | std::ios::binary);

          if (!file_out_ptr_->is_open()) {
            throw std::runtime_error("Failed to open file "s + output_path.generic_string() + " for writing");
          }

          auto fs = file_out_ptr_.get();
          out_stream_ = fs;
        }

        if(force_cli_listener) {
          if(listener_arg == "ide") { 
            listener_ptr_ = std::make_shared<listener_wrapper<cute::ide_listener<>>>(get_output());   
          }       
          else if(listener_arg == "classic") {
            listener_ptr_ = std::make_shared<listener_wrapper<cute::ostream_listener<>>>(get_output());
          }
          else if(listener_arg == "classicxml"){
            listener_ptr_ = std::make_shared<listener_wrapper<cute::xml_listener<>>>(get_output());
          }
          else if(listener_arg == "modernxml"){
            listener_ptr_ = std::make_shared<modern_xml_listener>(get_output());
          } 
          else if(listener_arg == "modern"){
            listener_ptr_ = std::make_shared<modern_listener>(get_output());
          }
          else {
            std::stringstream ss_err{};
            ss_err << "Unknown --listener option: " << listener_arg << " (valid options are: 'modern', 'modernxml', 'classic', 'classicxml' and 'ide')";
            throw std::runtime_error(ss_err.str());
          }
        }
        else {
          listener_ptr_ = std::make_shared<listener_wrapper<app_listener_T>>(listener);
        }
      }

      ~wrapper_options() {

        get_output() << std::flush;

        if(file_out_ptr_ && file_out_ptr_->is_open()) {
          file_out_ptr_->close();
        }
      }

      ext_listener& get_listener() {
        return *listener_ptr_;
      }
    };
  }

  template <typename app_listener_T = cute::null_listener>
  class wrapper {
  private:

    detail::wrapper_options<app_listener_T> opt;    

    /// @brief used to communicate the test case exit reason (pass: 0 / fail: 1 / error: 2)  in autoparallel child mode
    std::optional<int> force_destructor_exit_code;

    /// @brief all test suites registered (could be just one depending on the run mode)
    std::unordered_map<std::string, std::shared_ptr<ext_suite>> all_suites_{};

    std::atomic<size_t> test_exec_failures = 0;
    std::atomic<size_t> test_exec_errors = 0;

    
    /// @brief callback used to customize autoparallel process start parameters and environment prior to launching child processes
    /// @param const std::string& name of the suite
    /// @param const std::string& name of the test unit
    /// @param std::vector<std::string>& arguments for the process start
    /// @param std::map<std::string, std::string>& environment variable map
    std::function<
      void(
        const std::string&, 
        const std::string&, 
        std::vector<std::string>&, 
        std::unordered_map<std::string, std::string>&
      )> on_before_start_process_ = [](const auto& suitename, const auto& unitname, auto& args, auto& env) {};

    /// @brief Create a filtering lambda/fn given a (regex) string
    /// @param pattern 
    /// @return 
    std::function<bool(const std::string&)> make_filter_fn(const std::string& pattern) {
      
      auto filter = [pattern](const std::string &name) {
        const std::regex rx(pattern, std::regex_constants::icase);   
        
        std::smatch rx_match;
        return pattern.empty() || std::regex_search(name, rx_match, rx);
      };

      return std::move(filter);
    }

    void print_help() {      
      opt.get_output() 
          << opt.program_exe_path << " - usage\n" 
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

    /// @brief for -ltc / --list-testcases
    void print_all_tests() {

      auto& out = opt.get_output();

      for(const auto &[suite_info, suite_ptr] : all_suites_) {
        out << "Suite: '" << suite_info << "'\n";

        for(const auto &test_case : *suite_ptr) {
          out << " + " << test_case.name() << "\n";
        }

        out << "\n";
      }

      out << std::flush;
    }

    /// @brief represents a single test case run(ning) in an external process and 
    // wraps all the control logic for that
    struct autoparallel_testcase {

      autoparallel_testcase(
        const std::vector<std::string>& base_cmd, 
        const std::unordered_map<std::string, std::string> &base_env, 
        const std::string &suite, 
        const std::string &unit, 
        const cute::test &cute_unit,
        std::function<void(const std::string&, const std::string&, std::vector<std::string>&, std::unordered_map<std::string, std::string>&)> on_before_start_process)
        : suite_(suite)
        , unit_(unit)
        , process_base_cmd_(base_cmd)
        , process_base_env_(base_env)
        , cute_unit_(cute_unit)
        , out_ss_ptr_{std::make_shared<std::stringstream>()}
        , on_before_start_process_(on_before_start_process)
      {          
      }

      ~autoparallel_testcase() {
        if(process_thread_ptr && process_thread_ptr->joinable()) {            
          process_thread_ptr->join();            
        }
      }

      /// @brief start the process and handle the state machine in the background - returns "immediately"
      /// @param cb 
      void start(std::function<void(const autoparallel_testcase&)> cb) {
        bool expected = false;

        // thread safe "only once" barrier
        if(started_.compare_exchange_strong(expected, true)) {
          started_ = true; 
          running_ = true;
          process_thread_ptr = std::make_shared<std::thread>(&autoparallel_testcase::run_process, this, cb); 
        }
      }

      auto& get_suite_name() const { return suite_; }
      auto& get_unit_name() const { return unit_; }
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

        /* process args */
        std::vector<std::string> cmd_args{};

        #if defined(_WIN32) && !defined(MSYS_PROCESS_USE_SH)
        cmd_args.push_back("cmd");
        cmd_args.push_back("/c");
        #endif

        // add all of process_base_cmd_ to cmd_args
        std::copy(process_base_cmd_.begin(), process_base_cmd_.end(), std::back_inserter(cmd_args));

        // add the parallal-suite arg & value
        cmd_args.push_back("--parallel-suite");
        cmd_args.push_back(suite_);

        cmd_args.push_back("--parallel-unit");
        cmd_args.push_back(unit_);

        /* process env */
        std::unordered_map<std::string, std::string> cmd_env(process_base_env_);

        // call customization cb
        on_before_start_process_(suite_, unit_, cmd_args, cmd_env);

        /* process output */
        auto loc_ss_ptr = out_ss_ptr_;

        try { 
          auto processio = [loc_ss_ptr](const char *bytes, size_t n) {
            auto &out = *loc_ss_ptr;
            out << std::string(bytes, n);
          };

          process_ptr = std::make_shared<TinyProcessLib::Process>(
            cmd_args,   /* cmd + arg */
            "",         /* path */
            cmd_env,    /* env */
            processio,  /* stdout */
            processio,  /* stderr */
            false       /* don't open stdin */
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

      // this struct gets emplaced - std::atomic<> is not copy/move-able
      util::copyable_atomic<bool> started_ = false;

      bool running_ = false;
      bool done_    = false;
      bool success_ = false;
      std::string suite_;
      std::string unit_;
      std::vector<std::string> process_base_cmd_;
      std::unordered_map<std::string, std::string> process_base_env_;
      const cute::test &cute_unit_;
      
      std::shared_ptr<std::stringstream> out_ss_ptr_;
      std::shared_ptr<TinyProcessLib::Process> process_ptr;
      std::optional<int> process_exit_code{};

      std::shared_ptr<std::thread> process_thread_ptr;

      std::function<void(const std::string&, const std::string&, std::vector<std::string>&, std::unordered_map<std::string, std::string>&)> on_before_start_process_;
    };

    bool cmd_autoparallel() {
      
      auto &listener = opt.get_listener();
      
      auto filter_suite_enabled = make_filter_fn(opt.filter_suite_value);
      auto filter_unit_enabled = make_filter_fn(opt.filter_unit_value);

      // build the base command
      std::vector<std::string> base_process_cmd_args{};

      {
        base_process_cmd_args.push_back(opt.program_exe_path);

        if(opt.listener_arg.empty() == false) {
          base_process_cmd_args.push_back("--listener");
          base_process_cmd_args.push_back(opt.listener_arg);
        }

        if(opt.force_cli_listener) {
          base_process_cmd_args.push_back("--force-listener");
        }
      }

      // retrieve the full environment map of the current process
      // this will be used as base for the per-child-process environment map
      auto base_env = util::get_current_ENVIRONMENT();

      // collect all the tests
      std::vector<autoparallel_testcase> tasks{};

      for(const auto &[suite_name, suite] : all_suites_) {
        
        // respect --filter-suite
        if(filter_suite_enabled(suite_name) == false) {
          continue;
        }
        
        for(const auto &test_case : *suite) {

          // no need to spawn for disabled/skipped tests...
          if(filter_unit_enabled(test_case.name())) {
            tasks.emplace_back(base_process_cmd_args, base_env, suite_name, test_case.name(), test_case, on_before_start_process_);
          }
        }
      }

      /** HEPLPERS for the actual processing below */
      std::atomic<bool> linear_mode = false;
      std::string linear_mode_suite_name{};
      
      auto next_task_it = [&]() {        
        if(linear_mode) {
          return std::find_if(tasks.begin(), tasks.end(), [&](auto &t) { return t.was_started() == false && t.get_suite_name() == linear_mode_suite_name; });
        }
        else {
          // first try to look for "before_all suites"
          auto result = std::find_if(
            tasks.begin(), 
            tasks.end(), 
            [&](auto &t) { 
              auto suite_ptr = all_suites_.at(t.get_suite_name());
              return t.was_started() == false && suite_ptr->run_setting == ext_run_setting::before_all; 
            }
          );

          if(result != tasks.end()) {
            return result;
          }

          result = std::find_if(
            tasks.begin(), 
            tasks.end(), 
            [&](auto &t) { 
              auto suite_ptr = all_suites_.at(t.get_suite_name());
              return t.was_started() == false && suite_ptr->run_setting == ext_run_setting::normal; 
            }
          );

          if(result != tasks.end()) {
            return result;
          }
          
          result = std::find_if(
            tasks.begin(), 
            tasks.end(), 
            [&](auto &t) { 
              auto suite_ptr = all_suites_.at(t.get_suite_name());
              return t.was_started() == false && suite_ptr->run_setting == ext_run_setting::after_all; 
            }
          );

          if(result != tasks.end()) {
            return result;
          }

          return result;
        }
      };

      auto tasks_remaining = [&]() {
        auto all_done = std::all_of(tasks.begin(), tasks.end(), [](auto &t) { return t.is_done(); });
        auto all_started = std::all_of(tasks.begin(), tasks.end(), [](auto &t) { return t.was_started(); });
        return !all_done || !all_started;
      };

      std::mutex suites_started_mutex;
      std::vector<std::string> suites_started{};
      auto mark_suite_started = [&](const std::string suite_name) {
        bool inserted = false;

        {     
          // scoped / shortest possible lock   
          std::lock_guard<std::mutex> gg{suites_started_mutex};

          if(std::find(suites_started.begin(), suites_started.end(), suite_name) == suites_started.end()) {
            suites_started.push_back(suite_name);
            inserted = true;
          }          
        }

        if(inserted) {
          auto suite_ptr = all_suites_.at(suite_name);
          listener.suite_begin(*suite_ptr, suite_name.c_str(), suite_ptr->size());
        }        
      };

      std::atomic<size_t> tests_failed = 0;
      std::atomic<size_t> tests_running = 0;
      
      // we do a two stage status tracking here so we can be
      // sure we don't mix up the suite printing over linear <=> parallel
      // processing transitions
      std::mutex suites_printing_started_mutex;
      std::set<std::string> suites_printing_started{}; 
      std::set<std::string> suites_printed{};      

      auto was_suite_printed = [&](const std::string suite_name) {
        return std::find(suites_printed.begin(), suites_printed.end(), suite_name) != suites_printed.end();
      };

      auto all_suites_printed = [&]() {
        return suites_printed.size() == all_suites_.size();
      };

      auto print_suite = [&](const auto& suite, const std::string& suite_name) {
        auto print_now = false;
        
        // check if this is a *new insertion* in the printing started set...
        {
          std::lock_guard<std::mutex> gg{suites_printing_started_mutex};
          auto insert_result = suites_printing_started.insert(suite_name);
          print_now = insert_result.second; // true only if there was an insertion
        }
        
        // this should only happend one
        if(print_now) {
          listener.suite_end(suite, suite_name.c_str());

          // finally we update the status to "printed" so it's actually tracked as such
          suites_printed.insert(suite_name);
        }
      };

      auto linear_skip =[&]() {
        return (linear_mode == true) && (tests_running > 0);
      };

      auto suite_finished = [&](const std::string& suite) {
        size_t count_tests  = std::count_if(tasks.begin(), tasks.end(), [&](auto &t) { return t.get_suite_name() == suite; });
        size_t count_done   = std::count_if(tasks.begin(), tasks.end(), [&](auto &t) { return t.get_suite_name() == suite && t.is_done(); });
        return count_tests == count_done;
      };


      auto print_finished_suites = [&]() {
        // collect the results until all tasks of a suite are finished
        for(const auto &[suite_name, suite_ptr] : all_suites_) {

          if(suite_finished(suite_name) && !was_suite_printed(suite_name)) {
            print_suite(*suite_ptr, suite_name);
          }

          opt.get_output() << std::flush;  // makes sure ctest prints something while the test runs...
        }
      };
      


      auto start_next = [&]() {
        auto nextit = next_task_it();

        if(nextit != tasks.end()) {

          const std::string& suite_name = nextit->get_suite_name();
          auto suite_ptr = all_suites_.at(suite_name);

          // remember if this unit was started in linear mode
          // because only those should be able to decide to exit
          // the linear mode (avoid races to the bottom)
          bool unit_force_linear_mode = suite_ptr->force_linear;

          // we are not yet in linear mode... 
          // 1. wait here until no more tasks are running
          // 2. enter linear mode 
          // 3. start processing again until all tasks in linear mode are done
          if(unit_force_linear_mode) {
            if(linear_mode == false) {
              // wait for all running tasks to finish...
              while(tests_running > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));  // TODO: 1 or 10ms
              }

              print_finished_suites();

              linear_mode = true;
              linear_mode_suite_name = suite_name;              

              // go to immediate mode rendering
              listener.set_render_options(true, true, true, true);
            }
          }

          tests_running++;
          mark_suite_started(suite_name);

          /**note: CLANG requires us to copy capture unit_force_linear_mode (probably because of a thread locality specific behavior) */
          nextit->start([&, unit_force_linear_mode](const auto &task) {

            auto urr = task.get_run_unit_result();            

            if(urr == detail::run_unit_result::passed || urr == detail::run_unit_result::skipped) {
              listener.test_success(task.get_cute_unit(), "");
            }
            else if(urr == detail::run_unit_result::failed) {
              tests_failed++;
              cute::test_failure fail_info(task.get_output().c_str(), "", -1);
              listener.test_failure(task.get_cute_unit(), fail_info);
            }
            else {
              tests_failed++;
              listener.test_error(task.get_cute_unit(), task.get_output().c_str());
            }

            tests_running--;

            // all linear mode test are done
            // this is "thread safe" because we should never enter the case that
            // we have multiple linear mode tests running at once
            if(unit_force_linear_mode && linear_mode == true && tests_running == 0) {
              
              // peek next test and see if it's still the same suite
              auto next_peek = next_task_it();

              // next_task_it() does only return tasks of the same suite
              // in linear mode, so if we don't get any, we're done with the linear mode
              if(next_peek == tasks.end()) {

                // force printing of the suite end here so there's no race between 
                // immediate mode & parallel printing
                print_finished_suites();

                // them we can leave immediate mode rendering
                // (order is important, this has to be re-set before disabling linear_mode, 
                // otherwise there will be rendering hickups)
                listener.set_render_options(true, true, true, false);

                // finally revert to parallel mode processing
                linear_mode_suite_name = ""; // reset
                linear_mode = false;
              }              
            }
          });                    

          listener.test_start(nextit->get_cute_unit(), *suite_ptr);

          if(linear_mode == true) {
            opt.get_output() << std::flush;  // makes sure ctest prints something while the test runs...
          }
          return true;
        }

        return false;
      };     

      /** The actual running thing */
      listener.set_render_options(true, true, true, false);
      listener.render_preamble();
      
      size_t strand_limit = ( opt.parallel_strands_arg < 1024) ? opt.parallel_strands_arg : 1024;
      size_t loop_cnt = 0;

      while(tasks_remaining() || !all_suites_printed()) {
        
        // after a max of N-thread iterations we'd like to 
        // check if there's something to print just to keep things fluid
        loop_cnt = 0;

        // start as many tasks as we have "slots"
        while((loop_cnt++ < strand_limit) && !linear_skip() && tasks_remaining() && (tests_running < strand_limit)) {
          start_next();
        }        

        if(!linear_mode) {
          // this could otherwise be called in parallel in another thread
          // in the autoparallel_testcase completion callback   
          print_finished_suites();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      if(force_destructor_exit_code.value_or(0) == 0) { 
        force_destructor_exit_code = (tests_failed == 0) ? 0 : 1;
      }

      listener.render_end();
      return tests_failed == 0;
    }

    bool cmd_run_base(std::optional<std::string> suite_to_run = std::nullopt) {
      bool success = true;

      try {
        success = run_suites(suite_to_run);
      }
      catch(const std::exception& ex) {
        opt.get_output() << "tipi cute_ext failed: " << ex.what() << "\n";
        success = false;
      }

      if(!success && !force_destructor_exit_code.has_value()) {
        force_destructor_exit_code = 1;
      }

      return success;        
    }

  public:

    /// @brief Sums test_exec_failures and test_exec_errors
    /// @return 
    size_t get_total_test_exec_fail() {
      return test_exec_errors + test_exec_failures;
    }

    /// @brief Returns the total number of failures
    /// @return 
    size_t get_total_failure_count() {
      return test_exec_failures;
    }

    /// @brief Returns the total number of errors
    /// @return 
    size_t get_total_error_count() {
      return test_exec_failures;
    }
    

    wrapper(app_listener_T& listener, int argc, const char **argv, bool exit_on_destruction = true)
      : opt(listener, argc, argv, exit_on_destruction)
    {      
    }

    ~wrapper() {

      if(opt.parallel_run) {
        process_cmd();
      }

      // in linear mode we do this at the proper end
      if(!opt.parallel_run && !opt.parallel_child_run) {
        opt.get_listener().render_end();
        opt.get_output() << std::flush;
      }
      
      if(force_destructor_exit_code.has_value()) {
        std::exit(force_destructor_exit_code.value());
      }
    }

    bool process_cmd(std::optional<std::string> suite_to_run = std::nullopt) {      

      if(opt.show_help) {
        print_help();
        std::exit(0);
      }

      // list here
      if(opt.list_testcases) {
        print_all_tests();

        if(!opt.run_explicit) {
          std::exit(0);
        }
      }

      if(opt.parallel_child_run) {
        return cmd_run_base(suite_to_run);
      }

      // run the suite automatically if no additional params are provided OR and explicit --run is present
      if(!opt.list_testcases || opt.run_explicit) {

        if(opt.parallel_run) {
          return cmd_autoparallel();
        }      

        return cmd_run_base(suite_to_run);        
      }

      return true;
    }

    /// @brief Callback used to customize the startup command of every child process 
    /// launched in auto-parallel mode
    /// @param cb void fn with parameters: [const std::string&] suite_name, [const std::string&] unit_name, [std::vector<std::string>&] cmd_args, [std::unordered_map<std::string, std::string>&] environment_variables
    void set_on_before_autoparallel_child_process(std::function<void(const std::string&, const std::string&, std::vector<std::string>&, std::unordered_map<std::string, std::string>&)> cb) {
      on_before_start_process_ = cb;
    }

    /// @brief Returns true if the passed args signify this runner is executing in autoparallel_child mode
    /// @return 
    bool is_autoparallel_child() {
      return opt.parallel_child_run;
    }
    
    /// @brief Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite cute::suite
    /// @param info     
    void register_suite(std::shared_ptr<ext_suite> suite_ptr, const std::string& name) {
      all_suites_.insert({ name, suite_ptr });
      
      if(opt.parallel_run || (opt.list_testcases && !opt.run_explicit)) {
        // done
      }
      else {        
        // in single-TC mode (as parallel mode child process) we skip every suite that is not the
        // expected one
        if(!opt.parallel_run || (opt.parallel_child_run && name == opt.auto_concurrent_suite)) {
          process_cmd(name);
        }
      }     
    }

    /// @brief  Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite the cute::suite to execute
    /// @param name name of the suite
    /// @param run_setting ext_run_setting::normal / ext_run_setting::before_all / ext_run_setting::after_all
    /// @param force_linear set to true to force running this suite in linear mode
    void register_suite(const cute::suite&& suite, const std::string& name, ext_run_setting run_setting = ext_run_setting::normal, bool force_linear = false) { 
      register_suite(std::make_shared<ext_suite>(suite, run_setting, force_linear), name); 
    }

    /// @brief Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite cute::suite
    /// @param info name
    void operator()(const cute::suite& suite, const std::string& name, ext_run_setting run_setting = ext_run_setting::normal, bool force_linear = false) { 
      register_suite(std::make_shared<ext_suite>(suite, run_setting, force_linear), name); 
    }

    /// @brief Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite cute::suite
    /// @param info name
    void operator()(const cute::suite&& suite, const std::string& name, ext_run_setting run_setting = ext_run_setting::normal, bool force_linear = false) { 
      register_suite(std::make_shared<ext_suite>(suite, run_setting, force_linear), name); 
    }    

    /// @brief Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite ext_suite
    /// @param info name
    void register_suite(const ext_suite&& suite, const std::string& name, ext_run_setting run_setting = ext_run_setting::normal, bool force_linear = false) { 
      register_suite(std::make_shared<ext_suite>(suite, run_setting, force_linear), name); 
    }

    /// @brief Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite ext_suite
    /// @param info name
    void operator()(const ext_suite& suite, const std::string& name, ext_run_setting run_setting = ext_run_setting::normal, bool force_linear = false) { 
      register_suite(std::make_shared<ext_suite>(suite, run_setting, force_linear), name); 
    }

    /// @brief Register a new suite and - depending on CLI arguments - run the suite immediately
    /// @param suite ext_suite
    /// @param info name
    void operator()(const ext_suite&& suite, const std::string& name, ext_run_setting run_setting = ext_run_setting::normal, bool force_linear = false) { 
      register_suite(std::make_shared<ext_suite>(suite, run_setting, force_linear), name); 
    }  

    std::atomic<bool> first_run = true;
  
    bool run_suites(std::optional<std::string> suite_to_run = std::nullopt) {

      ext_listener& listener = opt.get_listener();

      auto filter_unit_enabled = make_filter_fn(opt.filter_unit_value);
      auto filter_suite_enabled = make_filter_fn(opt.filter_suite_value);

      if(first_run) {
        first_run = false;
        listener.render_preamble();        
      }

      // if we are in concurrent / parallel mode, disable all funny rendering
      listener.set_render_options(
        !opt.parallel_child_run,    /* render listener info */
        !opt.parallel_child_run,    /* render suite info */
        !opt.parallel_child_run,    /* render unit info */
        true                        /* immediate mode*/
      );

      auto run_unit = [&](const cute::test& t, const cute::suite& s) {

        if(filter_unit_enabled(t.name()) == false && !opt.parallel_child_run) {
          if(opt.skipped_as_pass) {
            listener.test_start(t, s);
            listener.test_success(t, "SKIPPED");
          }
          return detail::run_unit_result::skipped;
        }

        if(opt.parallel_child_run && t.name() != opt.auto_concurrent_unit) {
          return detail::run_unit_result::skipped;
        }

        detail::run_unit_result result;

        try {          
          listener.test_start(t, s);          
          t();
          listener.test_success(t, "OK");
          return detail::run_unit_result::passed;
        } catch(const cute::test_failure & e){
          listener.test_failure(t, e);
          result = detail::run_unit_result::failed;
        } catch(std::string & s){
          listener.test_error(t, s.c_str());
          result = detail::run_unit_result::errored;
        } catch(const char *&cs) {
          listener.test_error(t, cs);
          result = detail::run_unit_result::errored;
        } catch(const std::exception& e) {
          std::stringstream ss_err;
          ss_err  << "--- uncaught " << cute::demangle(typeid(e).name()) << "\n"
                  << "--- exception message start ---\n"
                  << e.what() << "\n"
                  << "--- exception message end ---";

          listener.test_error(t, ss_err.str().c_str());
          result = detail::run_unit_result::errored;
        } catch(...) {
          listener.test_error(t, "unknown exception thrown");
          result = detail::run_unit_result::errored;
        }
        
        return result;
      };

      bool result = true;

      for(const auto &[suite_name, suite_ptr] : all_suites_) {

        if(filter_suite_enabled(suite_name) && (suite_to_run.has_value() && suite_name == suite_to_run.value())) {

          const auto &suite = *suite_ptr;

          listener.suite_begin(suite, suite_name.c_str(), suite.size());

          for(auto &test : suite) {

            auto unit_result = run_unit(test, suite);

            if(unit_result == detail::run_unit_result::passed || unit_result == detail::run_unit_result::skipped) {
              result &= true;
            }
            else {
              test_exec_failures++;
              result &= false;
            }

            if(opt.parallel_child_run && unit_result != detail::run_unit_result::skipped) {
              force_destructor_exit_code = run_unit_result_to_ret(unit_result);
            }
          }
          
          listener.suite_end(suite, suite_name.c_str());
        }
        else {

          if(!opt.parallel_child_run && opt.skipped_as_pass) {
            std::string skipped_suite_info = suite_name + " [SKIPPED]";
            listener.suite_begin(*suite_ptr, skipped_suite_info.c_str(), 0);
            listener.suite_end(*suite_ptr, skipped_suite_info.c_str());
          }
        }        
      }

      return result;
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
    return wrapper(listener, argc, argv, exit_on_destruction);
  }

}

namespace cute {
  template <typename RunnerListener = cute::null_listener>
  inline tipi::cute_ext::wrapper<RunnerListener> makeRunner(RunnerListener& listener, int argc, const char **argv, bool exit_on_destruction = true) {
    return tipi::cute_ext::wrapper(listener, argc, argv, exit_on_destruction);
  }
}