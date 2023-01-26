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

#include <termcolor/termcolor.hpp>

#include "ext_listener.hpp"

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
      , start(std::chrono::steady_clock::now())
      , outcome(test_run_outcome::Unknown)
      , suite_run(suite_ptr)
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
      , start(std::chrono::steady_clock::now())
      , count_expected(number_of_tests)
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

  struct parallel_listener : public ext_listener
  {

  protected:
    bool render_listener_info = true;
    bool render_suite_info = true;
    bool render_test_info = true;
    bool render_immediate_mode = true;

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
    
  public:
    parallel_listener(std::ostream &os = std::cerr)
      : out(os)
      , listener_start(std::chrono::steady_clock::now())
    {
    }

    ~parallel_listener() {
      listener_end = std::chrono::steady_clock::now();
    }

    void set_render_options(bool render_listener_info, bool render_suite_info, bool render_test_info, bool immediate_mode) {
      this->render_listener_info = render_listener_info;
      this->render_suite_info = render_suite_info;
      this->render_test_info = render_test_info;
      this->render_immediate_mode = immediate_mode;
    }

    void render_preamble() override {
      parallel_render_preamble();
    }

    void render_end() override {
      parallel_render_end();
    }

    void suite_begin(cute::suite const &suite, char const *info, size_t n_of_tests) override 
    {
      auto suite_run_ptr = std::make_shared<suite_run>(std::string{info}, n_of_tests);
      {
        const std::lock_guard<std::mutex> lock(suites_i_mutex);

        auto er = suites.emplace(&suite, suite_run_ptr);
        if(er.second) {
          current_suite_ = suite_run_ptr;   // store for linear mode
        }
      }

      if(render_immediate_mode) {
        parallel_render_suite_header(out, suite_run_ptr);
      }
    }

    void suite_end(cute::suite const &suite, char const *info) override
    {
      if(suites.find(&suite) != suites.end()) {
        auto suite_ptr = suites.at(&suite);
        suite_ptr->done(info);      

        auto &sot = (render_immediate_mode) ? out : suite_ptr->out;

        if(!render_immediate_mode) {
          parallel_render_suite_header(sot, suite_ptr);

          for(const auto &t : suite_ptr->tests) {
            sot << t->out.str();
          }
        }

        parallel_render_suite_footer(sot, suite_ptr);

        if(!render_immediate_mode) {
          out << suite_ptr->out.str();
        }
      }
    }

    void test_start(cute::test const &test, const cute::suite& suite)
    {
      auto suite_ptr = suites.at(&suite);
      const std::lock_guard<std::mutex> lock(tests_i_mutex);  
      auto test_run_ptr = std::make_shared<test_run>(test.name(), suite_ptr);  

      auto er = tests.emplace(&test, test_run_ptr);
      if(er.second) {
        current_test_ = test_run_ptr;

        if(suite_ptr) {
          suite_ptr->tests.emplace_back(test_run_ptr);
        }
      }        

      parallel_render_test_case_start(test_run_ptr);
    }

    void test_success(cute::test const &test, char const *msg)
    {
      auto test_run_ptr = tests.at(&test);
      test_run_ptr->done(test_run_outcome::Pass, msg);

      parallel_render_test_case_end(test_run_ptr);
    }

    void test_failure(cute::test const &test, cute::test_failure const &e)
    {      
      std::stringstream ss{};
      ss << "[" << e.filename << ":" << e.lineno << "]\n" << e.reason;    
      
      auto test_run_ptr = tests.at(&test);
      test_run_ptr->done(test_run_outcome::Fail, ss.str());
      
      parallel_render_test_case_end(test_run_ptr);
    }

    void test_error(cute::test const &test, char const *what)
    {
      auto test_run_ptr = tests.at(&test);
      test_run_ptr->done(test_run_outcome::Error, what);

      parallel_render_test_case_end(test_run_ptr);
    }

    virtual void parallel_render_preamble() = 0;
    virtual void parallel_render_end() = 0;
    virtual void parallel_render_test_case_header(std::ostream &tco, const std::shared_ptr<test_run> &unit) = 0;
    virtual void parallel_render_test_case_start(const std::shared_ptr<test_run> &unit) = 0;
    virtual void parallel_render_test_case_result(std::ostream &tco, const std::shared_ptr<test_run> &unit) = 0;
    virtual void parallel_render_test_case_end(const std::shared_ptr<test_run> &unit) = 0;
    virtual void parallel_render_suite_header(std::ostream &sot, const std::shared_ptr<suite_run> &suite_ptr) = 0;
    virtual void parallel_render_suite_footer(std::ostream &sot, const std::shared_ptr<suite_run> &suite_ptr) = 0;
    
  };

}