# Advanced topics
## Controlling auto-parallel child process parameters and environment

**cute_ext** launches one process per test unit in `--parallel` mode. In order to enable customizations to the start parameters and environment variables
presented to each of these child processes you can register a callback that enables modifying startup parameters:

```cpp
runner.set_on_before_autoparallel_child_process([](const auto& suite_name, const auto& unit_name, auto& args, auto& env) -> void {
  /**
   * suite_name and unit_name are the std::string values used during the registration of the suites/units
   */

  /**
   * args is the pre-populated command + arguments vector that is used to start the child process
   * 
   * /!\ each parameter should be inserted into its own entry
   */

  /**
   * env in an std::unsorted_map<std::string, std::string> that contains a copy of the environment that 
   * was presented to the currently running executable
   */

  /**
   * EXAMPLES:
   */
  
  // add CUSTOM_VARIABLE=42 to env
  env.emplace("CUSTOM_VARIABLE", "42");
  
  // add --custom-option VALUE to command line arguments
  args.push_back("--custom-option");
  args.push_back("VALUE");
});
```

As a concrete example, to enable generating a per-process `gcov` report:

```cpp

#include <filesystem>

#if defined(_WIN32)

#include <windows.h>
#include <processthreadsapi.h>

#else

#include <sys/types.h>
#include <unistd.h>

#endif


auto get_pid = []() -> size_t {
  #if defined(_WIN32)

  return ::GetCurrentProcessId();

  #else

  return getpid();

  #endif
};

auto clean_path = [](std::string path) {
  static std::string forbidden_path_chars( "!$%&()[]{}§*+#:?\"<>|" );
  std::transform(
    path.begin(), path.end(), path.begin(), 
    [&](char c) { return forbidden_path_chars.find(c) != std::string::npos ? '_' : c; }
  );

  return path;
};

auto current_pid = get_pid();

std::stringstream pathbase_ss{};

#if defined(_WIN32)
pathbase_ss << "C:/temp/"; 
#else
pathbase_ss << "/tmp/"; 
#endif

pathbase_ss << "cute_ext_cov_" << current_pid << "/";
std::string pathbase = pathbase_ss.str();
std::filesystem::create_directories(pathbase);

std::cout << "\n---\n" << "ℹ️ All coverage analysis files will be written to: " << pathbase << "\n---\n" << std::endl;

runner.set_on_before_autoparallel_child_process([&, pathbase](const auto& suite_name, const auto& unit_name, auto& args, auto& env) -> void {
    // place all the .gcda files as: /tmp/cute_ext_cov_{current_pid}/{suite_name}-{unit_name}/<gcov-generate-filename.gcda>
    std::stringstream path_ss{};
    path_ss << pathbase << clean_path(suite_name) << "-" << clean_path(unit_name); 

    std::string path = path_ss.str();
    std::filesystem::create_directories(path);           

    env.emplace("GCOV_PREFIX", path);  
    env.emplace("GCOV_PREFIX_STRIP", "10000");  // clear all gcov generated folders
});

```


## Other helpers

```cpp
if(!runner.is_autoparallel_child()) { /* .... */ }
```