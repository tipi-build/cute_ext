#pragma once

#include <sstream>
#include <string>
#include <vector>

#include <cute/cute_listener.h>
#include <cute/cute_counting_listener.h>
#include <cute/ostream_listener.h>
#include <cute/ide_listener.h>
#include <cute/tap_listener.h>
#include <cute/xml_listener.h>

#if defined(_WIN32)
#include <winsock.h>

// and vt100 support enable api on windows
#include <windows.h>
#include <VersionHelpers.h>
#endif

namespace tipi::cute_ext::util
{

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
        std::cerr << "[win - VT100] detected MSYS console. VT100 sequences are supported" << std::endl;
        // nothing to do, most of the fancy stuff will just work!
      }
      else {

        auto enable_vt = [&]() -> DWORD {
          
          HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
          if (hOut == INVALID_HANDLE_VALUE)
          {
            std::cerr << "[win - VT100] Failed to acquire STD_OUTPUT_HANDLE" << std::endl;
            return GetLastError();
          }

          DWORD dwMode = 0;
          if (!GetConsoleMode(hOut, &dwMode))
          {
            std::cerr << "[win - VT100] Failed to get console mode" << std::endl;
            return GetLastError();
          }

          dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
          if (!SetConsoleMode(hOut, dwMode))
          {
            std::cerr << "[win - VT100] Failed to enable VT100 sequence processing" << std::endl;
            return GetLastError();
          }

          // success
          return 0;
        };

        int ret = enable_vt();
      
        if(ret != 0) {
          std::cerr << "[win - VT100] -> return code: " << ret << std::endl;
          std::cerr << "[win - VT100] not using VT100 escape sequences" << std::endl;
        }

        // using the utf8 code page
        if(!SetConsoleOutputCP(65001)) {
          std::cerr << "[win - VT100] switch to CP65001 failed with return code: " << GetLastError() << std::endl;
        }
      }
    #else
      /* does noting on unixes */
    #endif
  }
}