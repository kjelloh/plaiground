cmake_minimum_required(VERSION 3.25)
project(cratchit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cratchit_core
  src/amount.cpp
  src/cp437.cpp
  src/date.cpp
  src/eskd_reader.cpp
  src/eskd_writer.cpp
  src/latin1.cpp
  src/sie_line.cpp
  src/sie_reader.cpp
  src/vat_account_mapping.cpp
  src/vat_box.cpp
  src/vat_calculation.cpp
  src/vat_period.cpp
)
target_include_directories(cratchit_core PUBLIC src)
target_compile_options(cratchit_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(cratchit src/main.cpp)
target_link_libraries(cratchit PRIVATE cratchit_core)

enable_testing()
find_package(GTest REQUIRED)
add_executable(cratchit_tests
  tests/amount_test.cpp
  tests/cp437_test.cpp
  tests/date_test.cpp
  tests/eskd_real_files_test.cpp
  tests/eskd_test.cpp
  tests/latin1_test.cpp
  tests/sie_line_test.cpp
  tests/sie_reader_test.cpp
  tests/sie_real_files_test.cpp
  tests/vat_box_test.cpp
  tests/vat_calculation_test.cpp
  tests/vat_period_test.cpp
  tests/vat_real_files_test.cpp
)
target_link_libraries(cratchit_tests PRIVATE cratchit_core GTest::gtest_main)
target_compile_definitions(cratchit_tests PRIVATE CRATCHIT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cratchit_tests PRIVATE -Wall -Wextra -Wpedantic)
include(GoogleTest)
gtest_discover_tests(cratchit_tests)
