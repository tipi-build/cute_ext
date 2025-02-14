cmake_minimum_required(VERSION 3.27.6)

project(cute_ext 
  VERSION "1.1.0"
  LANGUAGES CXX
)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(NOT DEFINED FETCHCONTENT_QUIET) 
  set(FETCHCONTENT_QUIET OFF)
endif()

option(CUTE_EXT_FETCHCONTENT_DEPENDENCIES "set ON FetchContent Cute_ext's dependencies" OFF)

#
# Dependencies
#

include(FetchContent)

if(CUTE_EXT_FETCHCONTENT_DEPENDENCIES)
  # Disables the install rules that are added by including the subdirectory/fetchcontent installation script
  install(CODE "set(CMAKE_INSTALL_LOCAL_ONLY TRUE)" ALL_COMPONENTS)
  
  set(CXXOPTS_ENABLE_INSTALL ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG        eb787304d67ec22f7c3a184ee8b4c481d04357fd
    OVERRIDE_FIND_PACKAGE
  )

  set(TINY_PROCESS_LIBRARY_INSTALL ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    tiny-process-library
    GIT_REPOSITORY https://github.com/GerHobbelt/tiny-process-library.git
    GIT_TAG        2f7b3cf6d9972d096a77863e31c854f80194927d
    # note: the original project's CMakeLists do not create the install targets when the project is built as a dependency
    # this patched CMakelists adds a TINY_PROCESS_LIBRARY_INSTALL option
    PATCH_COMMAND  cmake -E copy ${CMAKE_CURRENT_LIST_DIR}/cmake/patches/tiny-process-library/CMakeLists.txt -t <SOURCE_DIR>
    OVERRIDE_FIND_PACKAGE
  )

  FetchContent_Declare(
    termcolor
    GIT_REPOSITORY https://github.com/ikalnytskyi/termcolor.git
    GIT_TAG        b3cb0f365f8435588df7a6b12a82b2ac5fc1fe95 # tag v2.1.0
    OVERRIDE_FIND_PACKAGE
  )

  set(BOOST_INCLUDE_LIBRARIES system filesystem CACHE STRING "" FORCE)
  FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG        32da69a36f84c5255af8a994951918c258bac601 # tag v1.80
    OVERRIDE_FIND_PACKAGE
  )

endif()

# note the use of OVERRIDE_FIND_PACKAGE args in the fetchContent_Declare()
# calls above. find_package will ensure that FetchContent_MakeAvailable is 
# called and use those if required (e.g. the dependency cannot be provided 
# by another mean)
find_package(cxxopts REQUIRED)
find_package(tiny-process-library REQUIRED)
find_package(termcolor REQUIRED)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

#
# Library & installation
#

add_library(cute_ext INTERFACE)
add_library(cute_ext::cute_ext ALIAS cute_ext)
target_link_libraries(cute_ext INTERFACE cxxopts::cxxopts tiny-process-library::tiny-process-library termcolor::termcolor)

set(CUTE_EXT_INCLUDE_INSTALL_DESTINATION_DIR "include/cute_ext")
set(CUTE_EXT_CMAKE_TARGETS_DESTINATION_DIR "lib/cmake/cute_ext")

target_include_directories(cute_ext INTERFACE 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/cute>             # for cute::cute emulation
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  $<INSTALL_INTERFACE:${CUTE_EXT_INCLUDE_INSTALL_DESTINATION_DIR}/cute> # for cute::cute emulation
  $<INSTALL_INTERFACE:${CUTE_EXT_INCLUDE_INSTALL_DESTINATION_DIR}>
)

install(TARGETS cute_ext
  EXPORT cute_extTargets
  LIBRARY DESTINATION  lib
  ARCHIVE DESTINATION  lib
  RUNTIME DESTINATION  bin
  INCLUDES DESTINATION include
)

install(EXPORT cute_extTargets
  FILE cute_extTargets.cmake
  NAMESPACE cute_ext::
  DESTINATION ${CUTE_EXT_CMAKE_TARGETS_DESTINATION_DIR}
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION ${CUTE_EXT_INCLUDE_INSTALL_DESTINATION_DIR})
#
# Testing
#

if(CUTE_EXT_BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
