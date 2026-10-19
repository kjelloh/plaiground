cmake_minimum_required(VERSION 3.25)
project(cratchit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CRATCHIT_WARNINGS -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_library(cratchit_core
  src/app/settings.cpp
  src/app/spec_check.cpp
  src/core/account.cpp
  src/core/account_types.cpp
  src/core/amount.cpp
  src/core/books.cpp
  src/core/date.cpp
  src/core/financial_event.cpp
  src/core/text.cpp
  src/cspec/spec_tree.cpp
  src/legal/compliance.cpp
  src/reports/planned_reports.cpp
  src/reports/provenance.cpp
  src/reports/vat_return_report.cpp
  src/sie/cp437.cpp
  src/sie/sie_reader.cpp
  src/sie/sie_to_journal.cpp
  src/statements/csv_statement_reader.cpp
  src/vat/eskd_reader.cpp
  src/vat/eskd_writer.cpp
  src/vat/vat_calculation.cpp
  src/vat/vat_spec.cpp
)
target_include_directories(cratchit_core PUBLIC src)
target_compile_options(cratchit_core PRIVATE ${CRATCHIT_WARNINGS})

add_executable(cratchit src/main.cpp)
target_link_libraries(cratchit PRIVATE cratchit_core)
target_compile_definitions(cratchit PRIVATE CRATCHIT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cratchit PRIVATE ${CRATCHIT_WARNINGS})

enable_testing()
find_package(GTest REQUIRED)
add_executable(cratchit_tests
  tests/amount_date_test.cpp
  tests/cspec_test.cpp
  tests/real_files_test.cpp
  tests/sie_test.cpp
  tests/specs_test.cpp
  tests/statement_test.cpp
  tests/vat_test.cpp
)
target_link_libraries(cratchit_tests PRIVATE cratchit_core GTest::gtest_main)
target_compile_definitions(cratchit_tests PRIVATE CRATCHIT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(cratchit_tests PRIVATE ${CRATCHIT_WARNINGS})
include(GoogleTest)
gtest_discover_tests(cratchit_tests)
