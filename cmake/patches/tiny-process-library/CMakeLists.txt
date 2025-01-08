cmake_minimum_required(VERSION 3.1)

project(tiny-process-library)

if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  set(project_standalone ON)
else()
  set(project_standalone OFF)
endif()

option(BUILD_TESTING "set ON to build library tests" ${project_standalone})
option(TINY_PROCESS_LIBRARY_INSTALL "Add install targets" ${project_standalone})

add_library(tiny-process-library process.cpp)
add_library(tiny-process-library::tiny-process-library ALIAS tiny-process-library)

if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  target_sources(tiny-process-library PRIVATE process_win.cpp)
  #If compiled using MSYS2, use sh to run commands
  if(MSYS)
    target_compile_definitions(tiny-process-library PUBLIC MSYS_PROCESS_USE_SH)
  endif()
else()
  target_sources(tiny-process-library PRIVATE process_unix.cpp)
endif()

find_package(Threads REQUIRED)

target_link_libraries(tiny-process-library ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(tiny-process-library PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                                                       $<INSTALL_INTERFACE:include>)

# if tiny-process-library is not a sub-project:
if(TINY_PROCESS_LIBRARY_INSTALL)
  add_executable(examples examples.cpp)
  target_link_libraries(examples tiny-process-library)

  install(TARGETS tiny-process-library
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

  install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
  )

  install(FILES process.hpp DESTINATION include)
endif()

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()
