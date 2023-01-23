#pragma once

#include <chrono>
#include <atomic>
#include <string>
#include <iostream>
#include <map>
#include <sstream>
#include <optional>
#include <atomic>
#include <vector>
#include <mutex>

#include <cute/cute_listener.h>
#include <termcolor/termcolor.hpp>

namespace tipi::cute_ext
{
  using namespace std::string_literals;

  enum test_run_outcome {
    Unknown,
    Pass,
    Fail,
    Error
  };

  struct suite_run;

  struct test_run {
    std::string name;
    std::chrono::steady_clock::time_point start;
    std::optional<std::chrono::steady_clock::time_point> end;
    test_run_outcome outcome;
    std::shared_ptr<tipi::cute_ext::suite_run> suite_run;
    std::stringstream out{};
    std::stringstream info{};

    test_run(std::string name, std::shared_ptr<tipi::cute_ext::suite_run> suite_ptr)
      : name(name)
      , suite_run(suite_ptr)
      , start(std::chrono::steady_clock::now())
      , outcome(test_run_outcome::Unknown)
    {
    }

    void done(test_run_outcome outcome, const std::string &info);

    /// @brief Return the test run duration. If the test has not ended yet returns the difference to now()
    /// @return 
    std::chrono::duration<double> get_test_duration() {
      return std::chrono::duration_cast<std::chrono::duration<double>>(end.value_or(std::chrono::steady_clock::now()) - start);
    }

  };

  struct suite_run {
    std::string name;
    std::chrono::steady_clock::time_point start;
    std::optional<std::chrono::steady_clock::time_point> end;

    std::atomic<size_t> count_expected = 0;
    std::atomic<size_t> count_failures = 0;
    std::atomic<size_t> count_errors = 0;
    std::atomic<size_t> count_success = 0;

    std::vector<std::shared_ptr<test_run>> tests{};
    std::stringstream out{};
    std::stringstream info{};

    suite_run(std::string name, size_t number_of_tests)
      : name(name)
      , count_expected(number_of_tests)
      , start(std::chrono::steady_clock::now())
    {

    }

    void done(std::string info) {
      this->end = std::chrono::steady_clock::now();
      this->info << info;
    }

    /// @brief Return the test run duration. If the test has not ended yet returns the difference to now()
    /// @return 
    std::chrono::duration<double> get_suite_duration() {
      return std::chrono::duration_cast<std::chrono::duration<double>>(end.value_or(std::chrono::steady_clock::now()) - start);
    }
  };

  void test_run::done(test_run_outcome outcome, const std::string &info) {
    this->outcome = outcome;
    this->end = std::chrono::steady_clock::now();
    this->info << info;   

    if(outcome == test_run_outcome::Pass) {
      this->suite_run->count_success++;
    }
    else if(outcome == test_run_outcome::Fail) {
      this->suite_run->count_failures++;
    }
    else if(outcome == test_run_outcome::Error) {
      this->suite_run->count_errors++;
    }
  }

  template <typename Listener = cute::null_listener>
  struct parallel_listener : public Listener
  {
    bool render_listener_info;
    bool render_suite_info;
    bool render_test_info;
    bool render_immediate_mode;

    std::ostream &out;

    std::mutex suites_i_mutex;
    std::unordered_map<const cute::suite*, std::shared_ptr<suite_run>> suites{};
    
    std::mutex tests_i_mutex;
    std::unordered_map<const cute::test*, std::shared_ptr<test_run>> tests{};

    std::shared_ptr<suite_run> current_suite_;
    std::shared_ptr<test_run> current_test_;

    std::atomic<size_t> suite_failures = 0;
    std::atomic<size_t> suite_success = 0;

    std::chrono::steady_clock::time_point listener_start;
    std::optional<std::chrono::steady_clock::time_point> listener_end;
    
    const std::string SEPARATOR_THICK = "===============================================================================\n";
    const std::string SEPARATOR_THIN  = "-------------------------------------------------------------------------------\n";
  protected:

  public:
    parallel_listener(std::ostream &os = std::cerr)
      : render_suite_info(true)
      , render_test_info(true)
      , render_immediate_mode(true)
      , out(os)
      , listener_start(std::chrono::steady_clock::now())
    {

      render_listener_start();

    }

    ~parallel_listener() {
      listener_end = std::chrono::steady_clock::now();
      render_listener_end();
    }

    virtual void render_listener_start() {

      if(render_listener_info) {

      }

    }

    virtual void render_listener_end() {
      using namespace std::chrono_literals;

      if(render_listener_info) {
        double total_time_ms = 0;

        for(auto &[test, test_ptr] : tests) {
          total_time_ms += test_ptr->get_test_duration().count();         
        }

        auto user_total_time_ms = std::chrono::duration_cast<std::chrono::duration<double>>(listener_end.value_or(std::chrono::steady_clock::now()) - listener_start);

        // duration_s = total_time;

        out << "\n"
            << "Test stats: \n"
            << " - suites executed:     " << suites.size() << "\n"
            << " - suites passed:       " << suite_success << "\n";

        if(suite_failures > 0) { out << termcolor::red; }
        out << " - suites failed:       " << suite_failures << "\n";
        if(suite_failures > 0) { out << termcolor::reset; }

        out << " - test cases executed: " << tests.size() << "\n"
            << " - total test time:     " << total_time_ms << "s\n" 
            << " - total user time:     " << user_total_time_ms.count() << "s\n"
            << "\n";

          
        if(suite_failures == 0) {
          out << termcolor::green << "✔  PASS";
        } 
        else {
          out << termcolor::red << "❌ FAILED";
        }

        out << termcolor::reset << std::endl;
      }
    }

    void set_render_options(bool render_listener_info, bool render_suite_info, bool render_test_info, bool immediate_mode) {
      this->render_listener_info = render_listener_info;
      this->render_suite_info = render_suite_info;
      this->render_test_info = render_test_info;
      this->render_immediate_mode = immediate_mode;
    }

    void begin(cute::suite const &suite, char const *info, size_t n_of_tests)
    {
      auto suite_run_ptr = std::make_shared<suite_run>(std::string{info}, n_of_tests);
      {
        const std::lock_guard<std::mutex> lock(suites_i_mutex);
        const cute::suite* suite_ptr = &suite;

        auto er = suites.emplace(suite_ptr, suite_run_ptr);
        if(er.second) {
          current_suite_ = suite_run_ptr;   // store for linear mode
        }
      }

      render_suite_start(suite_run_ptr);
      Listener::begin(suite, info, n_of_tests);
    }

    void end(cute::suite const &suite, char const *info)
    {
      {
        auto suite_ptr = suites.at(&suite);
        suite_ptr->done(info);
        render_suite_end(suite_ptr);
      }

      Listener::end(suite, info);
    }

    void start_internal(cute::test const &test, const std::shared_ptr<suite_run> sr) {  

      const std::lock_guard<std::mutex> lock(tests_i_mutex);  
      auto test_run_ptr = std::make_shared<test_run>(test.name(), sr);  

      auto er = tests.emplace(&test, test_run_ptr);
      if(er.second) {
        current_test_ = test_run_ptr;

        if(sr) {
          sr->tests.emplace_back(test_run_ptr);
        }
      }        

      render_test_case_start(test_run_ptr);      
      Listener::start(test);
    }

    void start(cute::test const &test) {
      start_internal(test, current_suite_);
    }

    void start(cute::test const &test, const cute::suite& suite)
    {
      auto suite_ptr = suites.at(&suite);
      start_internal(test, suite_ptr);
    }

    void success(cute::test const &test, char const *msg)
    {
      auto test_run_ptr = tests.at(&test);
      test_run_ptr->done(test_run_outcome::Pass, msg);

      render_test_case_end(test_run_ptr);
      Listener::success(test, msg);
    }

    void failure(cute::test const &test, cute::test_failure const &e)
    {      
      std::stringstream ss{};
      ss << "[" << e.filename << ":" << e.lineno << "]\n" << e.reason;    
      
      auto test_run_ptr = tests.at(&test);
      test_run_ptr->done(test_run_outcome::Fail, ss.str());
      
      render_test_case_end(test_run_ptr);
      Listener::failure(test, e);
    }

    void failure(cute::test const &test, char const *what)
    {
      auto test_run_ptr = tests.at(&test);
      test_run_ptr->done(test_run_outcome::Fail, what);

      render_test_case_end(test_run_ptr);
      
      cute::test_failure e(what, "", 0);
      Listener::failure(test, e);
    }

    void error(cute::test const &test, char const *what)
    {
      auto test_run_ptr = tests.at(&test);
      test_run_ptr->done(test_run_outcome::Error, what);

      render_test_case_end(test_run_ptr);
      Listener::error(test, what);
    }


    virtual void render_test_case_header(std::ostream &tco, const std::shared_ptr<test_run> &unit) {
      if(render_test_info) {
        tco << " ▶ " << util::padRight(unit->name, 40, ' ') << std::flush;
      }        
    }

    virtual void render_test_case_start(const std::shared_ptr<test_run> &unit) {
      auto &tco = (render_immediate_mode) ? out : unit->out;

      if(render_immediate_mode) {
        render_test_case_header(tco, unit);
      }
    }

    virtual void render_test_case_result(std::ostream &tco, const std::shared_ptr<test_run> &unit) {

      if(unit->outcome == test_run_outcome::Pass) {
        tco << termcolor::green << util::padRight("✔  PASS", 12, ' ') << termcolor::reset;
      }
      else if(unit->outcome == test_run_outcome::Fail) {
        tco << termcolor::red << util::padRight("❌ FAILED", 11, ' ') << termcolor::reset;
      }
      else if(unit->outcome == test_run_outcome::Error) {
        tco << termcolor::red << util::padRight("❌ ERROR", 11, ' ') << termcolor::reset;
      }
      else {
        tco << termcolor::cyan << "O RUNNING/Unknown" << termcolor::reset;
      }

      tco << "    (" << unit->get_test_duration().count() << "ms)";

      if(unit->outcome == test_run_outcome::Fail) {
        tco << "\n"
            << SEPARATOR_THIN
            << unit->info.str()
            << "\n"
            << termcolor::reset
            << SEPARATOR_THIN;
      }
      else if(unit->outcome == test_run_outcome::Error) {
        tco << "\n"
            << SEPARATOR_THIN
            << "Unhandled exception:\n"
            << unit->info.str()
            << "\n"
            << termcolor::reset
            << SEPARATOR_THIN;
      }
      else {
        tco << "\n";
      }
    
    }

    virtual void render_test_case_end(const std::shared_ptr<test_run> &unit) {

      auto &tco = (render_immediate_mode) ? out : unit->out;

      if(render_test_info) {
        if(!render_immediate_mode) {
          render_test_case_header(tco, unit);
        }

        render_test_case_result(tco, unit);        
      }
      else {
        tco << unit->info.str();
      } 
    }


    virtual void render_suite_header(std::ostream &sot, const std::shared_ptr<suite_run> &suite_ptr) {
      if(render_suite_info) {
        sot << "● " << suite_ptr->name << "\n" << SEPARATOR_THICK << "\n";
      }
    }

    virtual void render_suite_footer(std::ostream &sot, const std::shared_ptr<suite_run> &suite_ptr) {
      if(render_suite_info) {

        if(suite_ptr->count_errors + suite_ptr->count_failures == 0) {
          suite_success++;
          sot << "\n"
              << "  | Suite     ✔  PASS";
        }
        else {
          suite_failures++;
          sot << "\n"
              << "  | Suite     ❌ FAILED";
        }

        /* calulate how long the suite took */
        sot << " (" << suite_ptr->get_suite_duration().count() << "s)";

        sot << "\n";

        sot << "  | Tests     " << suite_ptr->count_expected  << "\n";

        sot << "  | ";
        if(suite_ptr->count_success == suite_ptr->count_expected) { sot << termcolor::green; }
        sot << "Pass      " << suite_ptr->count_success << "\n";
        if(suite_ptr->count_success == suite_ptr->count_expected) { sot << termcolor::reset; } 

        if(suite_ptr->count_failures > 0) { 
          sot << "  | " << termcolor::red << "Failed    " << suite_ptr->count_failures << termcolor::reset << "\n";         
        }

        if(suite_ptr->count_errors > 0) { 
          sot << "  | " << termcolor::red << "Errored   " << suite_ptr->count_errors << termcolor::reset << "\n";         
        }

        auto count_skipped = suite_ptr->count_expected - suite_ptr->count_failures - suite_ptr->count_errors - suite_ptr->count_success;

        if(count_skipped > 0) {
          sot << "  | " << termcolor::yellow << "Skipped   " << std::to_string(count_skipped) << termcolor::reset << "\n";
        }
          
        sot << "\n\n";
      }
    }
    
    virtual void render_suite_start(const std::shared_ptr<suite_run> &suite_ptr) {

      auto &sot = (render_immediate_mode) ? out : suite_ptr->out;

      if(render_immediate_mode) {
        render_suite_header(sot, suite_ptr);
      }

    }

    virtual void render_suite_end(const std::shared_ptr<suite_run> &suite_ptr) {

      auto &sot = (render_immediate_mode) ? out : suite_ptr->out;

      if(!render_immediate_mode) {
        render_suite_header(sot, suite_ptr);

        for(const auto &t : suite_ptr->tests) {
          sot << t->out.str();
        }
      }

      render_suite_footer(sot, suite_ptr);

      if(!render_immediate_mode) {
        out << suite_ptr->out.str();
      }
    }
  };

}