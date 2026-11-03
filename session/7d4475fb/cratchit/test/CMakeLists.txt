find_package(GTest REQUIRED)

add_executable(cratchit_tests
  test_amount.cpp
  test_closing.cpp
  test_cp437.cpp
  test_csv.cpp
  test_date.cpp
  test_ink2_accounts.cpp
  test_ink2_arguments.cpp
  test_ink2_filed.cpp
  test_ink2_form.cpp
  test_ink2_mapping.cpp
  test_ink2_output.cpp
  test_ink2_previous.cpp
  test_ink2_skv.cpp
  test_ink2_sru_output.cpp
  test_ink2_tax.cpp
  test_log.cpp
  test_manual.cpp
  test_sie_check.cpp
  test_sie_document.cpp
  test_sie_file.cpp
  test_sie_lexer.cpp
  test_sru.cpp
  test_terminal_encoding.cpp
  test_utf8.cpp
)
target_link_libraries(cratchit_tests PRIVATE cratchit_core cratchit_warnings GTest::gtest_main)
# The session's SIE files and filed INK2 as SRU files (git ignored); tests using them skip when missing
target_compile_definitions(cratchit_tests PRIVATE
  CRATCHIT_INK2_SIE_FOLDER="${PROJECT_SOURCE_DIR}/../_Resources/_sie"
  CRATCHIT_INK2_SRU_FOLDER="${PROJECT_SOURCE_DIR}/../_Resources/_ink2")

include(GoogleTest)
gtest_discover_tests(cratchit_tests)
