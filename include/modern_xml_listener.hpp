#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <atomic>

#include <cute/cute_listener.h>
#include <termcolor/termcolor.hpp>

namespace tipi::cute_ext
{
  using namespace std::string_literals;

  template <typename Listener = cute::null_listener>
  struct modern_xml_listener : Listener
  { 
    bool wrap_testsuite_elem;
    std::ostream &out;
    std::atomic<size_t> suite_expected = 0;
    std::atomic<size_t> suite_failures = 0;
    std::atomic<size_t> suite_errors = 0;
    std::atomic<size_t> suite_success = 0;

    std::map<const cute::suite*, std::chrono::steady_clock::time_point> suite_start_times{};
    std::map<const cute::test*, std::chrono::steady_clock::time_point> test_start_times{};

    // a bit more details in the test case and suite streams
    std::stringstream current_suite_stream{};
    std::stringstream current_tests_stream{};

    std::string current_suite_name;
    std::string current_test_name;

    std::vector<std::chrono::milliseconds> test_case_durations{};
    std::vector<std::string> failed_tests{};

  protected:

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
    modern_xml_listener(std::ostream &os = std::cerr, bool wrap_testsuite_elem = false)
      : wrap_testsuite_elem(wrap_testsuite_elem)
      , out(os)
    {
      if(wrap_testsuite_elem) {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            << "<testsuite>\n";
      }
    }

    ~modern_xml_listener() {

      using namespace std::chrono_literals;

      auto total_time_ms = 0ms;

      for(auto& v : test_case_durations) {
        total_time_ms += v;
      }

      auto total_time = std::chrono::duration_cast<std::chrono::duration<double>>(total_time_ms);

      // duration_s = total_time;
      const auto INDENT = "  ";
      out << INDENT << "<!--\n"
          << INDENT << INDENT << "tipi::cute_ext details \n"
          << INDENT << INDENT << "Test stats: \n"
          << INDENT << INDENT << " - suites executed:     " << suite_start_times.size() << "\n"
          << INDENT << INDENT << " - test cases executed: " << test_start_times.size() << "\n"
          << INDENT << INDENT << " - total duration:      " << total_time.count() << "s\n" 
          << INDENT << INDENT << "Result " << ((suite_failures == 0) ? "PASS" : "FAILED") << "\n"
          << INDENT << "-->\n";

      if(wrap_testsuite_elem) {
        out << "</testsuite>\n";
      }
    }

    void begin(cute::suite const &suite, char const *info, size_t n_of_tests)
    {
      suite_start_times.insert({ &suite, std::chrono::steady_clock::now() });
      suite_failures.exchange(0);
      suite_success.exchange(0);
      suite_errors.exchange(0);
      suite_expected.exchange(n_of_tests);

      current_suite_stream = std::stringstream{};
      current_suite_name = std::string(info);
    
      Listener::begin(suite, info, n_of_tests);
    }

    void end(cute::suite const &suite, char const *info)
    {
      auto suite_finished_ts = std::chrono::steady_clock::now();

      /**
       <testsuite errors="0" failures="0" id="0" name="my test suite" tests="1">
           .....
      </testsuite>
       */      

      const auto INDENT = "  "; // level 1 indenting

      out << INDENT
          << "<testsuite "
          <<  "errors=\"" << suite_errors << "\" "
          <<  "failures=\"" << suite_failures << "\" "
          <<  "name=\"" << mask_xml_chars(info) << "\" "
          <<  "tests=\"" << suite_expected << "\"";

      /* calulate how long the suite took */
      if(suite_start_times.find(&suite) != suite_start_times.end()) {
        auto suite_duration_s = std::chrono::duration_cast<std::chrono::duration<double>>(suite_finished_ts - suite_start_times[&suite]);
        out <<  " time=\"" << suite_duration_s.count() << "\"";
      }

      out << ">\n";

      out << current_suite_stream.str();
      
      out << INDENT
          << "</testsuite>\n";

      Listener::end(suite, info);
    }

    void start(cute::test const &test)
    {
      test_start_times.insert({ &test, std::chrono::steady_clock::now() });
      current_tests_stream = std::stringstream{};

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

      current_tests_stream 
        << INDENT
        << "<testcase "
        <<  "name=\"" << mask_xml_chars(test.name()) << "\" ";

      Listener::start(test);
    }

    void success(cute::test const &test, char const *msg)
    {
      auto test_finished_ts = std::chrono::steady_clock::now();
      suite_success++;
      
      /**
        The first bit was written in start(...)
        [... <testcase assertions="" classname="" name=""... ] time=""></testcase>
       */

      /* calulate how long the suite took */
      if(test_start_times.find(&test) != test_start_times.end()) {
        auto duration_s = std::chrono::duration_cast<std::chrono::milliseconds>(test_finished_ts - test_start_times[&test]);
        test_case_durations.push_back(duration_s);
        current_tests_stream << "time=\"" << duration_s.count() << "\"";
      }

      current_tests_stream 
        << "></testcase>\n";


      current_suite_stream << current_tests_stream.str();

      Listener::success(test, msg);
    }

    void failure(cute::test const &test, cute::test_failure const &e)
    {
      auto test_finished_ts = std::chrono::steady_clock::now();
      suite_failures++;

      const auto INDENT = "     ";

      /**
        [ ... <testcase assertions="" classname="" name="" ... ] time="">
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

      /* calulate how long the suite took */
      if(test_start_times.find(&test) != test_start_times.end()) {
        auto duration_s = std::chrono::duration_cast<std::chrono::milliseconds>(test_finished_ts - test_start_times[&test]);
        test_case_durations.push_back(duration_s);
        current_tests_stream << "time=\"" << duration_s.count() << "\"";
      }

      current_tests_stream 
        << ">\n";

      current_tests_stream 
        << INDENT 
        << "  " // +1/2
        << "<failure message=\"" << mask_xml_chars(e.reason) << " in " << mask_xml_chars(e.filename) << ":" << e.lineno << "\"/>\n";

      current_tests_stream 
        << INDENT
        << "</testcase>\n";

      current_suite_stream << current_tests_stream.str();
      Listener::failure(test, e);
    }
    void error(cute::test const &test, char const *what)
    {
      auto test_finished_ts = std::chrono::steady_clock::now();
      suite_errors++;

      const auto INDENT = "     ";

      /**
        [ ... <testcase assertions="" classname="" name="" ... ] time="">
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

      /* calulate how long the suite took */
      if(test_start_times.find(&test) != test_start_times.end()) {
        auto duration_s = std::chrono::duration_cast<std::chrono::milliseconds>(test_finished_ts - test_start_times[&test]);
        test_case_durations.push_back(duration_s);
        current_tests_stream << "time=\"" << duration_s.count() << "\"";
      }

      current_tests_stream 
        << ">\n";

      current_tests_stream 
        << INDENT 
        << "  " // +1/2
        << "<error message=\"" << what << "\" type=\"Unhandled exception\"/>\n";

      current_tests_stream 
        << INDENT
        << "</testcase>\n";

      current_suite_stream << current_tests_stream.str();
      Listener::error(test, what);
    }
  };

}