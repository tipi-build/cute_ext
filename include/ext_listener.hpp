#pragma once

#include <cute/cute_listener.h>
#include "util.hpp"

namespace tipi::cute_ext
{
  /// @brief virtual base 
  struct ext_listener {

    ext_listener() { }
    virtual ~ext_listener() { };

    /// @brief Set the listener render options
    /// @param render_listener_info set to true if listener should output listener infos ("preamble" and "footer" typically)
    /// @param render_suite_info set to true if listener should output the suite header / footer
    /// @param render_test_info set to true if listener should render individual tests
    /// @param immediate_mode set to true to set the listener to immediate mode (outputs without buffering by testcase / suite etc)
    virtual void set_render_options(bool render_listener_info, bool render_suite_info, bool render_test_info, bool immediate_mode) = 0;

    /// @brief called at the start of a suite
    /// @param suite 
    /// @param info 
    /// @param n_of_tests 
    virtual void suite_begin(cute::suite const &suite, char const *info, size_t n_of_tests) = 0;

    /// @brief called at the end of a suite
    /// @param suite 
    /// @param info 
    virtual void suite_end(cute::suite const &suite, char const *info) = 0;
    
    /// @brief called at the start of a test case / unit
    /// @param test 
    /// @param suite 
    virtual void test_start(cute::test const &test, const cute::suite& suite) = 0;
    
    /// @brief Mark a test/unit as PASS
    /// @param test 
    /// @param msg 
    virtual void test_success(cute::test const &test, char const *msg) = 0;
    
    /// @brief Mark a test/unit as FAILED
    /// @param test 
    /// @param e 
    virtual void test_failure(cute::test const &test, cute::test_failure const &e) = 0;
    
    /// @brief Mark a test/unit as ERROR
    /// @param test 
    /// @param what 
    virtual void test_error(cute::test const &test, char const *what) = 0;

    /// @brief Render the preamble (everything before the first suite gets rendered)
    virtual void render_preamble() = 0;

    /// @brief Render the test summary / footer to the run
    virtual void render_end() = 0;
  };
}