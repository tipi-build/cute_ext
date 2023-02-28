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

#include <original/CUTE/cute/cute.h>
#include <termcolor/termcolor.hpp>
#include "parallel_listener.hpp"
#include "util.hpp"

namespace tipi::cute_ext
{
  struct modern_listener : public parallel_listener
  {
  protected:
    const std::string SEPARATOR_THICK = "===============================================================================\n";
    const std::string SEPARATOR_THIN  = "-------------------------------------------------------------------------------\n";

    size_t test_case_name_padding = 40;

  public:
    modern_listener(std::ostream &os = std::cerr) 
      : parallel_listener(os) 
    {
    }
    
    ~modern_listener() {
    }

    void virtual parallel_render_preamble() override {

      if(this->render_listener_info) {

        auto now = std::chrono::system_clock::now();
        std::time_t now_tm = std::chrono::system_clock::to_time_t(now);

        std::string datetimenowstr{std::ctime(&now_tm)};
        datetimenowstr.pop_back();  // f*ck that last \n

        const auto lock = this->acquirex(this->out);
        this->out << util::symbols::run_icon << "Awesome testing with " << termcolor::cyan << termcolor::bold << "tipi.build" << termcolor::reset << " + " << termcolor::yellow << "CUTE" << termcolor::reset << "\n"
            << " -> Starting test at: " << datetimenowstr << " - [" <<  now.time_since_epoch().count() << "]\n"
            << SEPARATOR_THIN
            << std::endl;

      }
    }

    void virtual parallel_render_end() override {
      using namespace std::chrono_literals;

      const auto lock = this->acquirex(this->out);

      if(this->render_listener_info) {
        double total_time_ms = 0;

        for(auto &[test, test_ptr] : this->tests) {
          total_time_ms += test_ptr->get_test_duration().count();         
        }

        auto user_total_time_ms = std::chrono::duration_cast<std::chrono::duration<double>>(this->listener_end.value_or(std::chrono::steady_clock::now()) - this->listener_start);
        auto count_suites_pass = std::count_if(this->suites.begin(), this->suites.end(), [](auto &p) { return p.second->is_success(); });
        auto count_suites_fail = std::count_if(this->suites.begin(), this->suites.end(), [](auto &p) { return p.second->is_success() == false; });  


        this->out 
          << "\n"
          << "Test stats: \n"
          << " - suites executed:     " << this->suites.size() << "\n"
          << " - suites passed:       " << count_suites_pass << "\n";

        if(count_suites_fail > 0) { this->out << termcolor::red; }
        this->out << " - suites failed:       " << count_suites_fail << termcolor::reset << "\n";

        this->out 
          << " - test cases executed: " << this->tests.size() << "\n"
          << " - total test time:     " << total_time_ms << "s\n" 
          << " - total user time:     " << user_total_time_ms.count() << "s\n"
          << "\n";
 
          
        if(count_suites_fail == 0) {
          this->out << termcolor::green << util::symbols::suite_pass << " PASS " << util::symbols::suite_pass;
        } 
        else {
          this->out << termcolor::red << util::symbols::suite_fail << " FAILED " <<  util::symbols::suite_fail;
        }

        this->out << termcolor::reset << std::endl;
      }
    }

    virtual void parallel_render_test_case_header(std::ostream &tco, const std::shared_ptr<test_run> &unit) override {
      if(this->render_test_info) {
        tco << " " << util::symbols::test_icon << " " << util::padRight(unit->name, test_case_name_padding, ' ');

        if(unit->name.size() > test_case_name_padding) {
          tco << "\n    " << util::padRight("", test_case_name_padding, ' ');
        }

        tco << std::flush;
      }        
    }

    virtual void parallel_render_test_case_start(const std::shared_ptr<test_run> &unit) override {
      if(this->render_immediate_mode) {
        const auto lock = this->acquirex(this->out);
        parallel_render_test_case_header(this->out, unit);
      }
    }

    std::string print_line_prefix_colorize_red(std::istream& input, std::string line_prefix) {

      std::stringstream output{};

      if(termcolor::_internal::is_colorized(this->out)) {
        output << termcolor::colorize;
      }      

      std::string line;
      while (std::getline(input, line)) {
        output << line_prefix << termcolor::red << line << termcolor::reset << "\n";
      }

      return output.str();
    }

    virtual void parallel_render_test_case_result(std::ostream &tco, const std::shared_ptr<test_run> &unit) override {
      if(unit->outcome == test_run_outcome::Pass) {
        tco << termcolor::green << util::symbols::test_pass << util::padRight(" PASS", 11, ' ') << termcolor::reset;
      }
      else if(unit->outcome == test_run_outcome::Fail) {
        tco << termcolor::red << util::symbols::test_fail << util::padRight(" FAILED", 11, ' ') << termcolor::reset;
      }
      else if(unit->outcome == test_run_outcome::Error) {
        tco << termcolor::red << util::symbols::test_error << util::padRight(" ERROR", 11, ' ') << termcolor::reset;
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

      tco << "\n";
     
      if(unit->outcome == test_run_outcome::Fail) {
        tco << print_line_prefix_colorize_red(unit->info, " :> ")
            << termcolor::reset
            << "\n";
      }
      else if(unit->outcome == test_run_outcome::Error) {
        tco << print_line_prefix_colorize_red(unit->info, " :> ")
            << termcolor::reset
            << "\n";
      }
    }

    virtual void parallel_render_test_case_end(const std::shared_ptr<test_run> &unit) override {

      if(this->render_immediate_mode) {
        const auto lock = this->acquirex(this->out);
        auto &tco = this->out;

        if(this->render_test_info) {
          parallel_render_test_case_result(tco, unit);
        }
        else {
          tco << unit->info.str();
        }

        tco << std::flush;
      }
      else {
        const auto lock = this->acquirex(unit->out);
        auto &tco = unit->out;

        if(this->render_test_info) {
          parallel_render_test_case_header(tco, unit);          
          parallel_render_test_case_result(tco, unit);
        }
        else {
          //const auto lock_unit = this->acquirex();
          tco << unit->info.str();
        }
      }     
    }

    virtual void parallel_render_suite_header(std::ostream &sot, const std::shared_ptr<suite_run> &suite_ptr) override {
      if(this->render_suite_info) {
        sot << " " << util::symbols::suite_icon << " " << suite_ptr->name << "\n" << SEPARATOR_THICK << "\n";
      }
    }

    virtual void parallel_render_suite_footer(std::ostream &sot, const std::shared_ptr<suite_run> &suite_ptr) override {
      if(this->render_suite_info) {

        if(suite_ptr->count_errors + suite_ptr->count_failures == 0) {
          sot << "\n"
              << "  | Suite     " << util::symbols::suite_pass << " PASS";
        }
        else {
          sot << "\n"
              << "  | Suite     " << util::symbols::suite_fail << " FAILED";
        }

        /* calulate how long the suite took */
        sot << " (" << suite_ptr->get_suite_duration().count() << "s)";

        sot << "\n";

        sot << "  | Tests     " << suite_ptr->count_expected  << "\n";

        sot << "  | ";
        if(suite_ptr->count_success == suite_ptr->count_expected) { sot << termcolor::green; }
        sot << "Pass      " << suite_ptr->count_success << termcolor::reset << "\n";
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