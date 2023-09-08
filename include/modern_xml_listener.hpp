#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <atomic>
#include <algorithm>

#include <termcolor/termcolor.hpp>
#include <original/CUTE/cute/cute.h>

#include "parallel_listener.hpp"

namespace tipi::cute_ext
{
  using namespace std::string_literals;

  struct modern_xml_listener : public parallel_listener
  {
    protected:

    /// @brief replaces ]]> with ]]]]><![CDATA[> in CDATA sections
    /// @param in 
    /// @return 
    std::string escape_cdata(std::string in) {
      const std::string search = "]]>";
      const std::string replace = "]]]]><![CDATA[>";
      
      size_t pos = 0;
      while((pos = in.find(search, pos)) != std::string::npos) {
        in.replace(pos, search.length(), replace);
        pos += replace.length();
      }

      return in;
    }

    std::string mask_xml_chars(std::string in){
			std::string::size_type pos=0;
			while((pos=in.find_first_of("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0b\x0c\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\"&'<>", pos, 34))!=std::string::npos){
				switch(in[pos]){
				case '&': in.replace(pos,1,"&amp;"); pos +=5; break;
				case '<': in.replace(pos,1,"&lt;"); pos += 4; break;
				case '>': in.replace(pos,1,"&gt;"); pos += 4; break;
				case '"': in.replace(pos,1,"&quot;"); pos+=6; break;
				case '\'':in.replace(pos,1,"&apos;"); pos+=6; break;
				default:
					char c = in[pos];
					std::string replacement = "0x" + cute::cute_to_string::hexit(c);
					in.replace(pos, 1, replacement); pos += replacement.size(); break;
					break;
				}
			}
			return in;
		}

  public:

    modern_xml_listener(std::ostream &os = std::cerr) : parallel_listener(os) {
    }
    
    ~modern_xml_listener() {
    }

    void virtual parallel_render_preamble() override {
      if(this->render_listener_info) {
        this->out 
          << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          << "<testsuites>\n";
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
        auto count_suites_pass = std::count_if(this->suites.begin(), this->suites.end(), [](auto &p) { return p.second->is_success(); });
        auto count_suites_fail = std::count_if(this->suites.begin(), this->suites.end(), [](auto &p) { return p.second->is_success() == false; });    

        const auto INDENT = "  ";
    
        this->out 
          << INDENT << "<!--\n"
          << INDENT << INDENT << "tipi::cute_ext details \n"
          << INDENT << INDENT << "Test stats: \n"
          << INDENT << INDENT << " - suites executed:     " << this->suites.size() << "\n"
          << INDENT << INDENT << " - suites passed:       " << count_suites_pass << "\n"
          << INDENT << INDENT << " - suites failed:       " << count_suites_fail << "\n"
          << INDENT << INDENT << "\n"
          << INDENT << INDENT << " - test cases executed: " << this->tests.size() << "\n"
          << INDENT << INDENT << " - total test time:     " << total_time_ms << "s\n" 
          << INDENT << INDENT << " - total user time:     " << user_total_time_ms.count() << "s\n"
          << INDENT << "-->\n";

        this->out << "</testsuites>\n";
        this->out << std::flush;
      }
    }

    virtual void parallel_render_suite_header(std::ostream &sot, const std::shared_ptr<suite_run> &suite_ptr) override {
      if(this->render_suite_info) {

        const auto INDENT = "  "; // level 1 indenting

        sot << INDENT << "<testsuite "
            << "name=\"" << mask_xml_chars(suite_ptr->name) << "\" "
            << "tests=\"" << suite_ptr->count_expected << "\"";

        if(suite_ptr->end.has_value()) {
          sot << " "
              << "errors=\"" << suite_ptr->count_errors << "\" "
              << "failures=\"" << suite_ptr->count_failures << "\"";
        }

        sot << ">\n";
      }
    }

    virtual void parallel_render_suite_footer(std::ostream &sot, const std::shared_ptr<suite_run> &suite_ptr) override {
      if(this->render_suite_info) {
        
        const auto INDENT = "  "; // level 1 indenting
        sot << INDENT << "</testsuite>\n";
      }
    }

    virtual void parallel_render_test_case_header(std::ostream &tco, const std::shared_ptr<test_run> &unit) override {
      if(this->render_test_info) {
        
        const auto INDENT = "    ";
        
        /**
          <testcase assertions="" classname="" name="" [ remainder for the rest...  time="">
              <skipped/>
              <error message="" type=""/>
              <failure message="" type=""/>
              <system-out/>
              <system-err/>
          </testcase> ]

          see:
            - success(...)
            - failure(...)
            - error(...)
        */

        tco << INDENT
            << "<testcase "
            <<  "name=\"" << mask_xml_chars(unit->name) << "\" ";

      }        
    }

    virtual void parallel_render_test_case_start(const std::shared_ptr<test_run> &unit) override {
      auto &tco = (this->render_immediate_mode) ? this->out : unit->out;

      if(this->render_immediate_mode) {
        this->parallel_render_test_case_header(tco, unit);
      }
    }

    virtual void parallel_render_test_case_result(std::ostream &tco, const std::shared_ptr<test_run> &unit) override {
      

      const auto INDENT = "    ";

      /**
        The first bit was written by render_test_case_start(...)
        [... <testcase assertions="" classname="" name=""... ]
       */

      /* render the time the test took */
      tco << "time=\"" << unit->get_test_duration().count() << "\"";

      /** PASS xml:
       * 
       * [ <testcase assertions="" classname="" name=""... ]  time="..."></testcase>
       */
      if(unit->outcome == test_run_outcome::Pass) {
        tco << "></testcase>\n";
      }
      /**
       * FAIL xml:
       * 
       * [<testcase assertions="" classname="" name=""... ] time="">
       *    <failure message="" type=""/>
       *    <system-out/>
       *    <system-err/>
       * </testcase>
       */
      else if(unit->outcome == test_run_outcome::Fail) {

        tco << ">\n"
            << INDENT 
            << "  " // +1/2
            << "<failure />\n"
            << INDENT
            << "  " // +1/2
            << "<system-out><![CDATA[\n"
            << escape_cdata(unit->info.str())
            << "]]></system-out>\n"
            << INDENT
            << "</testcase>\n";
      }
      else if(unit->outcome == test_run_outcome::Error) {
        tco << ">\n"
            << INDENT 
            << "  " // +1/2 
            << "<error type=\"Unhandled exception\"/>\n"
            << INDENT
            << "  " // +1/2
            << "<system-out><![CDATA[\n"
            << escape_cdata(unit->info.str())
            << "]]></system-out>\n"
            << INDENT
            << "</testcase>\n";
      }
      else {
        tco << ">\n"
            << INDENT 
            << "  " // +1/2 
            << "<error message=\"RUNNING/Unknown\" type=\"RUNNING/Unknown\"/>\n"
            << INDENT
            << "</testcase>\n";
      }

      tco << std::flush;
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
  };


}