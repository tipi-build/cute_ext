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
#include "parallel_listener.hpp"

namespace tipi::cute_ext
{
  struct modern_listener : public parallel_listener
  {
  protected:
    const std::string SEPARATOR_THICK = "===============================================================================\n";
    const std::string SEPARATOR_THIN  = "-------------------------------------------------------------------------------\n";
  

  public:
    modern_listener(std::ostream &os = std::cerr) 
      : parallel_listener(os) {  
    }
    
    ~modern_listener() {
    }

    void virtual parallel_render_preamble() override {

      if(this->render_listener_info) {

        auto now = std::chrono::system_clock::now();
        std::time_t now_tm = std::chrono::system_clock::to_time_t(now);

        std::string datetimenowstr{std::ctime(&now_tm)};
        datetimenowstr.pop_back();  // f*ck that last \n

        this->out << "Awesome testing with " << termcolor::cyan << termcolor::bold << "tipi.build" << termcolor::reset << " + " << termcolor::yellow << "CUTE" << termcolor::reset << "\n"
            << " -> Starting test at: " << datetimenowstr << " - [" <<  now.time_since_epoch().count() << "]\n"
            << SEPARATOR_THIN
            << "\n";

      }
    }

    void virtual parallel_render_end() override {
      using namespace std::chrono_literals;

      if(this->render_listener_info) {
        double total_time_ms = 0;

        for(auto &[test, test_ptr] : this->tests) {
          total_time_ms += test_ptr->get_test_duration().count();         
        }

        auto user_total_time_ms = std::chrono::duration_cast<std::chrono::duration<double>>(this->listener_end.value_or(std::chrono::steady_clock::now()) - this->listener_start);        

        this->out 
          << "\n"
          << "Test stats: \n"
          << " - suites executed:     " << this->suites.size() << "\n"
          << " - suites passed:       " << this->suite_success << "\n";

        if(this->suite_failures > 0) { this->out << termcolor::red; }
        this->out << " - suites failed:       " << this->suite_failures << "\n";
        if(this->suite_failures > 0) { this->out << termcolor::reset; }

        this->out 
          << " - test cases executed: " << this->tests.size() << "\n"
          << " - total test time:     " << total_time_ms << "s\n" 
          << " - total user time:     " << user_total_time_ms.count() << "s\n"
          << "\n";
 
          
        if(this->suite_failures == 0) {
          this->out << termcolor::green << "✔  PASS";
        } 
        else {
          this->out << termcolor::red << "❌ FAILED";
        }

        this->out << termcolor::reset << std::endl;
      }
    }

    virtual void parallel_render_test_case_header(std::ostream &tco, const std::shared_ptr<test_run> &unit) override {
      if(this->render_test_info) {
        tco << " ▶ " << util::padRight(unit->name, 40, ' ') << std::flush;
      }        
    }

    virtual void parallel_render_test_case_start(const std::shared_ptr<test_run> &unit) override {
      auto &tco = (this->render_immediate_mode) ? this->out : unit->out;

      if(this->render_immediate_mode) {
        parallel_render_test_case_header(tco, unit);
      }
    }

    virtual void parallel_render_test_case_result(std::ostream &tco, const std::shared_ptr<test_run> &unit) override {

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

      if(unit->get_test_duration().count() < 1) {
         tco << "    (" << unit->get_test_duration().count() * 1000 << "ms)";
      }
      else {
        tco << "    (" << unit->get_test_duration().count() << "s)";
      }
     
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

    virtual void parallel_render_test_case_end(const std::shared_ptr<test_run> &unit) override {

      auto &tco = (this->render_immediate_mode) ? this->out : unit->out;

      if(this->render_test_info) {
        if(!this->render_immediate_mode) {
          parallel_render_test_case_header(tco, unit);
        }

        parallel_render_test_case_result(tco, unit);        
      }
      else {
        tco << unit->info.str();
      } 
    }

    virtual void parallel_render_suite_header(std::ostream &sot, const std::shared_ptr<suite_run> &suite_ptr) override {
      if(this->render_suite_info) {
        sot << " ● " << suite_ptr->name << "\n" << SEPARATOR_THICK << "\n";
      }
    }

    virtual void parallel_render_suite_footer(std::ostream &sot, const std::shared_ptr<suite_run> &suite_ptr) override {
      if(this->render_suite_info) {

        if(suite_ptr->count_errors + suite_ptr->count_failures == 0) {
          this->suite_success++;
          sot << "\n"
              << "  | Suite     ✔  PASS";
        }
        else {
          this->suite_failures++;
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
  };

}