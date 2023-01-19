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
  struct modern_listener : Listener
  {
    std::ostream &out;
    bool detailed;

    const std::string SEPARATOR_THICK = "===============================================================================\n";    
    const std::string SEPARATOR_THIN  = "-------------------------------------------------------------------------------\n";

    std::atomic<size_t> suite_expected = 0;
    std::atomic<size_t> suite_failures = 0;
    std::atomic<size_t> suite_success = 0;

    std::map<const cute::suite*, std::chrono::steady_clock::time_point> suite_start_times{};
    std::map<const cute::test*, std::chrono::steady_clock::time_point> test_start_times{};

    std::vector<std::chrono::milliseconds> test_case_durations{};
    std::vector<std::string> failed_tests{};

  public:
    modern_listener(std::ostream &os = std::cerr, bool detailed = false)
      : out(os)
      , detailed(detailed) 
    {}

    ~modern_listener() {
      using namespace std::chrono_literals;

      auto total_time_ms = 0ms;

      for(auto& v : test_case_durations) {
        total_time_ms += v;
      }

      auto total_time = std::chrono::duration_cast<std::chrono::duration<double>>(total_time_ms);

      // duration_s = total_time;

      out << "\n"
          << "Test stats: \n"
          << " - suites executed:     " << suite_start_times.size() << "\n"
          << " - test cases executed: " << test_start_times.size() << "\n"
          << " - total duration:      " << total_time.count() << "s\n" 
          << "\n"
          << "Result " << ((suite_failures == 0) ? "🟢 PASS" : "🟥 FAILED")
          << "\n"
          << std::endl;
    }

    std::string padRight(std::string str, const size_t num, const char paddingChar = ' ') 
    {
        if(num > str.size())
            str.append(num - str.size(), paddingChar);

        return str;
    }


    void begin(cute::suite const &suite, char const *info, size_t n_of_tests)
    {
      suite_start_times.insert({ &suite, std::chrono::steady_clock::now() });

      suite_failures.exchange(0);
      suite_success.exchange(0);
      suite_expected.exchange(n_of_tests);

      out << termcolor::bold << "🧫 Test suite: " << info << " (" << n_of_tests << " tests)" << termcolor::bold << "\n\n"; 

      Listener::begin(suite, info, n_of_tests);
    }

    void end(cute::suite const &suite, char const *info)
    {
      auto suite_finished_ts = std::chrono::steady_clock::now();

      out << "\n"
          << "  | Suite   " << ((suite_failures == 0) ? "🟢 PASS" : "🟥 FAILED");

      /* calulate how long the suite took */
      if(suite_start_times.find(&suite) != suite_start_times.end()) {
        auto suite_duration_s = std::chrono::duration_cast<std::chrono::duration<double>>(suite_finished_ts - suite_start_times[&suite]);
        out << " (" << suite_duration_s.count() << "s)";
      }

      out << "\n";

      out << "  | Tests   " << suite_expected  << "\n";

      out << "  | ";
      if(suite_success == suite_expected) { out << termcolor::green; } else { out << termcolor::reset; }
      out << "Pass    " << suite_success << "\n";

      if(suite_failures > 0) { 
        out << "  | " << termcolor::red << "Failed  " << suite_failures 
            << termcolor::reset << "\n"; 
        
      } else { 
        out << termcolor::reset;
      }      

      if((suite_expected - suite_failures - suite_success) > 0) {
        out << "  | " << termcolor::yellow << "Skipped " << std::to_string(suite_expected - suite_failures - suite_success) << termcolor::reset << "\n";
      }
        
      out << std::endl;
      Listener::end(suite, info);
    }

    void start(cute::test const &test)
    {
      test_start_times.insert({ &test, std::chrono::steady_clock::now() });
      out << " 🧪 " << padRight(test.name(), 40, ' ');      
      Listener::start(test);
    }

    void success(cute::test const &test, char const *msg)
    {
      auto test_finished_ts = std::chrono::steady_clock::now();
      suite_success++;

      out << termcolor::bold << "🟢 PASS";

      /* calulate how long the suite took */
      if(test_start_times.find(&test) != test_start_times.end()) {
        auto duration_s = std::chrono::duration_cast<std::chrono::milliseconds>(test_finished_ts - test_start_times[&test]);
        test_case_durations.push_back(duration_s);
        out << "    (" << duration_s.count() << "ms)";
      }

      out << termcolor::reset << std::endl;

      Listener::success(test, msg);
    }
    void failure(cute::test const &test, cute::test_failure const &e)
    {
      auto test_finished_ts = std::chrono::steady_clock::now();
      suite_failures++;

      failed_tests.push_back(test.name());

      out << termcolor::bold << "🟥 FAILED" << termcolor::reset;

      /* calulate how long the cast took */
      if(test_start_times.find(&test) != test_start_times.end()) {
        auto duration_s = std::chrono::duration_cast<std::chrono::milliseconds>(test_finished_ts - test_start_times[&test]);
        test_case_durations.push_back(duration_s);
        out << "  (" << duration_s.count() << "ms)";
      }

      out << "\n"
          << SEPARATOR_THIN
          << termcolor::bright_grey
          << e.filename << ":" << e.lineno << "\n"
          << termcolor::red
          << e.reason << "\n"
          << termcolor::reset
          << SEPARATOR_THIN;

      Listener::failure(test, e);
    }
    void error(cute::test const &test, char const *what)
    {
      auto test_finished_ts = std::chrono::steady_clock::now();
      suite_failures++;
      out << termcolor::bold <<  "🟥 FAILED" << termcolor::reset;

      /* calulate how long the cast took */
      if(test_start_times.find(&test) != test_start_times.end()) {
        auto duration_s = std::chrono::duration_cast<std::chrono::milliseconds>(test_finished_ts - test_start_times[&test]);
        test_case_durations.push_back(duration_s);
        out << "  (" << duration_s.count() << "ms)";
      }

      out << "\n"
          << SEPARATOR_THIN
          << termcolor::bright_grey
          << "Unhandled exception:\n"
          << termcolor::red
          << what << "\n"
          << termcolor::reset
          << SEPARATOR_THIN;

      Listener::error(test, what);
    }
  };

}