#pragma once

#include <original/CUTE/cute/cute_listener.h>
#include "ext_listener.hpp"
#include "util.hpp"

namespace tipi
{
namespace cute_ext
{
  using namespace std::string_literals;

  /// @brief Just a wrapper to decorate original cute listeners that get
  /// passed to cute_ext to make them all look like an ext_listener
  /// @tparam Listener 
  template <typename t_wrapped=cute::null_listener>
  struct listener_wrapper : public ext_listener
  {    
  private:
    /// @brief holds the instance listener if it gets constructed in place
    std::shared_ptr<t_wrapped> held_listener_;

    /// @brief ref to the passed or instanciated listener
    t_wrapped& wrapped_;

  public:

    template <class... Param>
    listener_wrapper(Param &... args) 
      : held_listener_(std::make_shared<t_wrapped>(args...))
      , wrapped_(*held_listener_)
    {
    }

    template <class... Param>
    listener_wrapper(Param &&... args) 
      : held_listener_(std::make_shared<t_wrapped>(args...)) 
      , wrapped_(*held_listener_)
    {
    }

    listener_wrapper(t_wrapped &wrapped) : wrapped_(wrapped) 
    {
    }

    /**
     * set_render_options
     * 
     */
  public:
    void set_render_options(bool render_listener_info, bool render_suite_info, bool render_test_info, bool immediate_mode) override {
      call_set_render_options(wrapped_, render_listener_info, render_suite_info, render_test_info, immediate_mode);
    }

  protected:
    template <typename T_listener>
    typename std::enable_if<util::has_set_render_options_t<T_listener>::value, void>::type
    call_set_render_options(T_listener &listener, bool render_listener_info, bool render_suite_info, bool render_test_info, bool immediate_mode) { 
      listener.set_render_options(render_listener_info, render_suite_info, render_test_info, immediate_mode);
    }

    template <typename T_listener>
    typename std::enable_if<!util::has_set_render_options_t<T_listener>::value, void>::type
    call_set_render_options(T_listener &listener, bool render_listener_info, bool render_suite_info, bool render_test_info, bool immediate_mode) { 
      // function does not exit
    }

    /**
     * suite_begin
     * 
     */
  public:
    void suite_begin(cute::suite const &suite, char const *info, size_t n_of_tests) override {
      call_suite_begin(wrapped_, suite, info, n_of_tests);
    }

  protected:
    template <typename T_listener>
    typename std::enable_if<util::has_set_render_options_t<T_listener>::value, void>::type
    call_suite_begin(T_listener &listener, cute::suite const &suite, char const *info, size_t n_of_tests) { 
      listener.suite_begin(suite, info, n_of_tests);
    }

    template <typename T_listener>
    typename std::enable_if<!util::has_set_render_options_t<T_listener>::value, void>::type
    call_suite_begin(T_listener &listener, cute::suite const &suite, char const *info, size_t n_of_tests) { 
      listener.begin(suite, info, n_of_tests);
    }

    /**
     * suite_end
     * 
     */
  public:
    void suite_end(cute::suite const &suite, char const *info) override {
      call_suite_end(wrapped_, suite, info);
    }

  protected:
    template <typename T_listener>
    typename std::enable_if<util::has_set_render_options_t<T_listener>::value, void>::type
    call_suite_end(T_listener &listener, cute::suite const &suite, char const *info) { 
      listener.suite_end(suite, info);
    }

    template <typename T_listener>
    typename std::enable_if<!util::has_set_render_options_t<T_listener>::value, void>::type
    call_suite_end(T_listener &listener, cute::suite const &suite, char const *info) { 
      listener.end(suite, info);
    }

    /**
     * render_preamble
     * 
     */
  public:
    void test_start(cute::test const &test, const cute::suite& suite) override {
      call_test_start(wrapped_, test, suite);
    }

  protected:
    template <typename T_listener>
    typename std::enable_if<util::has_set_render_options_t<T_listener>::value, void>::type
    call_test_start(T_listener &listener, cute::test const &test, const cute::suite& suite) { 
      listener.test_start(test, suite);
    }

    template <typename T_listener>
    typename std::enable_if<!util::has_set_render_options_t<T_listener>::value, void>::type
    call_test_start(T_listener &listener, cute::test const &test, const cute::suite& suite) { 
      wrapped_.start(test);
    }

    /**
     * render_preamble
     * 
     */
  public:
    void test_success(cute::test const &test, char const *msg) override {
      call_test_success(wrapped_, test, msg);
    }

  protected:
    template <typename T_listener>
    typename std::enable_if<util::has_set_render_options_t<T_listener>::value, void>::type
    call_test_success(T_listener &listener, cute::test const &test, char const *msg) { 
      listener.test_success(test, msg);
    }

    template <typename T_listener>
    typename std::enable_if<!util::has_set_render_options_t<T_listener>::value, void>::type
    call_test_success(T_listener &listener, cute::test const &test, char const *msg) { 
      listener.success(test, msg);
    }

    /**
     * test_failure
     * 
     */
  public:
    void test_failure(cute::test const &test, cute::test_failure const &e) override {
      call_test_failure(wrapped_, test, e);
    }

  protected:
    template <typename T_listener>
    typename std::enable_if<util::has_set_render_options_t<T_listener>::value, void>::type
    call_test_failure(T_listener &listener, cute::test const &test, cute::test_failure const &e) { 
      listener.test_failure(test, e);
    }

    template <typename T_listener>
    typename std::enable_if<!util::has_set_render_options_t<T_listener>::value, void>::type
    call_test_failure(T_listener &listener, cute::test const &test, cute::test_failure const &e) { 
      listener.failure(test, e);
    }

    /**
     * render_preamble
     * 
     */
  public:
    void test_error(cute::test const &test, char const *what) override {
      call_test_error(wrapped_, test, what);
    }

  protected:
    template <typename T_listener>
    typename std::enable_if<util::has_set_render_options_t<T_listener>::value, void>::type
    call_test_error(T_listener &listener, cute::test const &test, char const *what) { 
      listener.test_error(test, what);
    }

    template <typename T_listener>
    typename std::enable_if<!util::has_set_render_options_t<T_listener>::value, void>::type
    call_test_error(T_listener &listener, cute::test const &test, char const *what) { 
      listener.error(test, what);
    }

    /**
     * render_preamble
     * 
     */
  public:
    void render_preamble() override {
      call_render_preamble(wrapped_);
    }

  protected:
    template <typename T_listener>
    typename std::enable_if<util::has_set_render_options_t<T_listener>::value, void>::type
    call_render_preamble(T_listener &listener) { 
      listener.render_preamble();
    }

    template <typename T_listener>
    typename std::enable_if<!util::has_set_render_options_t<T_listener>::value, void>::type
    call_render_preamble(T_listener &listener) { 
      // function does not exit
    }

    /**
     * render_end()
     */

  public:
    void render_end() override {
      call_render_end(wrapped_);
    }

  protected:
    template <typename T_listener>
    typename std::enable_if<util::has_set_render_options_t<T_listener>::value, void>::type
    call_render_end(T_listener &listener) { 
      listener.render_end();
    }

    template <typename T_listener>
    typename std::enable_if<!util::has_set_render_options_t<T_listener>::value, void>::type
    call_render_end(T_listener &listener) { 
      // function does not exit
    }
  };

}
}