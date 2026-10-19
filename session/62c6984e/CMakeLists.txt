cmake_minimum_required(VERSION 3.25)
project(cratchit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cratchit_core
  src/amount.cpp
)
target_include_directories(cratchit_core PUBLIC src)
target_compile_options(cratchit_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(cratchit src/main.cpp)
target_link_libraries(cratchit PRIVATE cratchit_core)

enable_testing()
find_package(GTest REQUIRED)
add_executable(cratchit_tests
  tests/amount_test.cpp
)
target_link_libraries(cratchit_tests PRIVATE cratchit_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(cratchit_tests)
