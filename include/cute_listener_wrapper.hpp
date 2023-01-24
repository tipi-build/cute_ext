#pragma once

#include <cute/cute_listener.h>
#include <cute/ide_listener.h>
#include "util.hpp"

namespace tipi::cute_ext
{
  using namespace std::string_literals;

  /// @brief Just a wrapper to decorate original cute::*_listener-s in a way all functions we expect are present
  /// @tparam Listener 
  template <typename Listener=cute::null_listener>
  struct cute_listener_wrapper : public cute::null_listener
  {
    
  private:
    Listener* wrapped_;

  public:

    //cute_listener_wrapper

    cute_listener_wrapper(Listener *wrapped) : wrapped_(wrapped) {
    }

    ~cute_listener_wrapper() {
    }

    void set_render_options(bool render_listener_info, bool render_suite_info, bool render_test_info, bool immediate_mode) {
      // does nothing ...
    }

    void begin(cute::suite const &suite, char const *info, size_t n_of_tests)
    {
      wrapped_->begin(suite, info, n_of_tests);
    }

    void end(cute::suite const &suite, char const *info)
    {
      wrapped_->end(suite, info);
    }    

    void start(cute::test const &test) {
      wrapped_->start(test);
    }

    void start(cute::test const &test, const cute::suite& suite)
    {
      wrapped_->start(test);
    }

    void success(cute::test const &test, char const *msg)
    {
      wrapped_->success(test, msg);
    }

    void failure(cute::test const &test, cute::test_failure const &e)
    {      
      wrapped_->failure(test, e);
    }

    void failure(cute::test const &test, char const *what)
    {
      cute::test_failure e(what, "", 0);
      wrapped_->failure(test, e);
    }

    void error(cute::test const &test, char const *what)
    {
      wrapped_->error(test, what);
    }

    void render_preamble() {}
    void render_end() { }
  };

}