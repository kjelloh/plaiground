find_package(GTest REQUIRED)

add_executable(cratchit_tests
  test_amount.cpp
  test_ar_closing.cpp
  test_ar_equity.cpp
  test_ar_layout.cpp
  test_ar_statement.cpp
  test_cp437.cpp
  test_date.cpp
  test_log.cpp
  test_manual.cpp
  test_sie_check.cpp
  test_sie_document.cpp
  test_sie_file.cpp
  test_sie_lexer.cpp
  test_terminal_encoding.cpp
  test_utf8.cpp
)
target_link_libraries(cratchit_tests PRIVATE cratchit_core cratchit_warnings GTest::gtest_main)
# The filed årsredovisningar's SIE files (git ignored) and the amounts typed in from them
target_compile_definitions(cratchit_tests PRIVATE
  CRATCHIT_AR_SIE_FOLDER="${PROJECT_SOURCE_DIR}/../_Resources/_sie"
  CRATCHIT_AR_EXPECTED_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/expected_ar")

include(GoogleTest)
gtest_discover_tests(cratchit_tests)
