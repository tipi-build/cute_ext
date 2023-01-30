#pragma once

#include <sstream>
#include <string>
#include <vector>

#include <cute/cute_listener.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <winsock.h>

// and vt100 support enable api on windows
#include <windows.h>
#include <VersionHelpers.h>

#else
// access POSIX environment
extern char **environ;
#endif


namespace tipi::cute_ext::util
{
  using namespace std::string_literals;

  namespace symbols {
    #ifdef CUTEEXT_SAFE_SYMBOLS
      
      const auto run_icon   = "▶";
      const auto suite_icon = "▶";
      const auto test_icon  = "●";      

      const auto test_pass  = "✔ "; // trailing space on purpose
      const auto test_fail  = "❌";
      const auto test_error = "❌";
      
      const auto suite_pass = "✔ ";
      const auto suite_fail = "❌"; // trailing space on purpose

    #else
    
      const auto run_icon   = "🏃";
      const auto suite_icon = "🧫";
      const auto test_icon  = "🧪";      

      const auto test_pass  = "🟢";
      const auto test_fail  = "🟥";
      const auto test_error = "❌";
      
      const auto suite_pass = "🟢";
      const auto suite_fail = "🟥";

    #endif
  }


  std::vector<std::string> split(const std::string &s, char delim)
  {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delim))
    {
      result.push_back(item);
    }

    return result;
  }

  std::string padRight(std::string str, const size_t num, const char paddingChar = ' ') 
  {
      if(num > str.size()) { str.append(num - str.size(), paddingChar); }
      return str;
  }

  template <typename TestClass, typename MemFun>
	cute::test makeSimpleMemberFunctionTest(MemFun fun,char const *name, char const *ctx){
		return cute::test(cute::incarnate_for_member_function<TestClass,MemFun>(fun), cute::demangle(typeid(TestClass).name())+"::"+name+"::"+ctx);
	}

  #define TIPI_CUTE_SMEMFUN(TestClass,MemberFunctionName,ctxname) \
	tipi::cute_ext::util::makeSimpleMemberFunctionTest<TestClass>(\
		&TestClass::MemberFunctionName,\
		#MemberFunctionName,\
    ctxname)

  inline bool tty_is_vscode_term() {
    char* tmp = std::getenv("TERM_PROGRAM");

    if(tmp == NULL) {
      return false;
    }
    else {
      std::string term_prog(tmp);
      return term_prog == "vscode";
    }
  }

  inline bool tty_is_windows_terminal() {
    return std::getenv("WT_SESSION") != NULL;
  }

  inline void enable_vt100_support_windows10() {
    #if defined(_WIN32)

      if(std::getenv("MSYSTEM") != NULL) {
        //std::cerr << "[win - VT100] detected MSYS console. VT100 sequences are supported" << std::endl;
        // nothing to do, most of the fancy stuff will just work!
      }
      else {

        auto enable_vt = [&]() -> DWORD {
          
          HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
          if (hOut == INVALID_HANDLE_VALUE)
          {
            //std::cerr << "[win - VT100] Failed to acquire STD_OUTPUT_HANDLE" << std::endl;
            return GetLastError();
          }

          DWORD dwMode = 0;
          if (!GetConsoleMode(hOut, &dwMode))
          {
            //std::cerr << "[win - VT100] Failed to get console mode" << std::endl;
            return GetLastError();
          }

          dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
          if (!SetConsoleMode(hOut, dwMode))
          {
            //std::cerr << "[win - VT100] Failed to enable VT100 sequence processing" << std::endl;
            return GetLastError();
          }

          // success
          return 0;
        };

        int ret = enable_vt();
      
        if(ret != 0) {
          //std::cerr << "[win - VT100] -> return code: " << ret << std::endl;
          //std::cerr << "[win - VT100] not using VT100 escape sequences" << std::endl;
        }

        // using the utf8 code page
        if(!SetConsoleOutputCP(65001)) {
          //std::cerr << "[win - VT100] switch to CP65001 failed with return code: " << GetLastError() << std::endl;
        }
      }
    #else
      /* does noting on unixes */
    #endif
  }

  // SFINAE tester
  template <typename Listener>
  class has_set_render_options_t
  {
  private:
      typedef char YesType[1];
      typedef char NoType[2];
      template <typename T> static YesType& test( decltype(&T::set_render_options) );
      template <typename T> static NoType& test(...);
  public:
      enum { value = sizeof(test<Listener>(0)) == sizeof(YesType) };
  };

  /// @brief the name says it all - a copy/move-able std::atomic<T>
  /// @tparam cnt_T 
  template<class cnt_T>
  class copyable_atomic : public std::atomic<cnt_T>
  {
  public:
    copyable_atomic() = default;
    constexpr copyable_atomic(cnt_T desired) : std::atomic<cnt_T>(desired) { /**/ }
    constexpr copyable_atomic(const copyable_atomic<cnt_T>& other) :
        copyable_atomic(other.load(std::memory_order_relaxed))
    { /**/ }

    copyable_atomic& operator=(const copyable_atomic<cnt_T>& other) {
      // avoid  unexpected reorderings
      this->store(
        other.load(std::memory_order_acquire),
        std::memory_order_release);
      return *this;
    }
  };

  /// @brief Get the ENV variable map of THIS process
  /// @return 
  inline std::unordered_map<std::string, std::string> get_current_ENVIRONMENT() {
    std::unordered_map<std::string, std::string> result{};
    
    #if defined(_WIN32)

    auto free = [](char* p) { FreeEnvironmentStrings(p); };
    auto env_block = std::unique_ptr<char, decltype(free)>{ GetEnvironmentStrings(), free};
    char* env = env_block.get();

    if (env != 0)
    {
      // GetEnvironmentStrings returns a block of \0 terminated string, with each string
      // being formated like:
      // VARIABLE=value\0
      // 
      // the last entry is terminated by two \0 (one for the variable, one for the entire block)
      //
      // this means that to parse through this we need to look out for two separators
      //
      // '=' separating the variable name from the value
      // '\0' terminating each entry
      //
      // We need to keep track of these special things however (which we should skip btw...)
      // 
      // =C:=C:\.tipi\v7.w\40999a5-cute_ext.b\63dea0c\bin\test
      // =D:=D:/
      //
      // these are "special" environment variables that cmd uses to keep track of a bunch of "special things"
      // like per disk CWDs or expected exit codes and that are basically MSDOS compat remainders:
      // interesting read: https://devblogs.microsoft.com/oldnewthing/20100506-00/?p=14133

      int entry_start = 0;          // position of the start of variable name
      int entry_separator = 0;      // position of the '=' separator 
      bool search_value = false;    // toggle between looking up a VAR or VALUE
      bool entry_line_start = true;
      bool entry_is_msdos_special = false;

      for(size_t ix = 0; ; ix++) {    

        /** handle the "MSDOS special" entries that have entries starting with = */

        if(entry_line_start) {
          if(env[ix] == '=') {
            entry_is_msdos_special = true;
          }

          entry_line_start = false;
        }

        if(entry_is_msdos_special) {
          if(env[ix] == '\0') {
            entry_line_start = true;
            entry_is_msdos_special = false;
            entry_start = ix;

            // the unlikely case that we have only special entries...
            //
            // break on double \0 -- this is safe only because windows
            // guarantees us that there *will be* two \0 at the end of the 
            // env var block
            if (env[ix + 1] == '\0') {
              break;
            }
          }
        }
        else {

          /** The "normal entries... "*/
          if(!search_value && env[ix] == '=') {
            search_value = true;
            entry_separator = ix;
          }
          else if(env[ix] == '\0') {

            std::string env_key(env + entry_start + 1, entry_separator - entry_start - 1);
            std::string env_val(env + entry_separator + 1, ix - entry_separator - 1);

            result.emplace(env_key, env_val); 
            
            // remember for the next round
            search_value = false;
            entry_start = ix;

            // break on double \0 -- this is safe only because windows
            // guarantees us that there *will be* two \0 at the end of the 
            // env var block
            if (env[ix + 1] == '\0') {
              break;
            }
          }
        }
      }
    }
    else
    {
      size_t err = GetLastError();
      throw std::runtime_error("Getting environment variables failed with return code: "s + std::to_string(err));
    }

    #else
    char ** env;    
    env = environ;
    
    for (; *env; ++env) {
      std::string env_raw = std::string(*env);
      auto pos_eq = env_raw.find('=');
      result.emplace(env_raw.substr(0, pos_eq), env_raw.substr(pos_eq + 1, env_raw.size()));
    }
            
    #endif

    return result;
  }

}