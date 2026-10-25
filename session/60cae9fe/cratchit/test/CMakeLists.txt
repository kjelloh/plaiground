find_package(GTest REQUIRED)

add_executable(cratchit_tests
  test_amount.cpp
  test_books.cpp
  test_columns.cpp
  test_cp437.cpp
  test_csv.cpp
  test_data_folder.cpp
  test_date.cpp
  test_entry.cpp
  test_help.cpp
  test_import.cpp
  test_key_input.cpp
  test_log.cpp
  test_log_view.cpp
  test_manual.cpp
  test_pager.cpp
  test_path_editor.cpp
  test_path_value.cpp
  test_project.cpp
  test_repl.cpp
  test_report.cpp
  test_statements.cpp
  test_sie_check.cpp
  test_sie_document.cpp
  test_sie_file.cpp
  test_sie_lexer.cpp
  test_statement.cpp
  test_terminal_encoding.cpp
  test_utf8.cpp
  test_version.cpp
  test_workspace.cpp
)
target_link_libraries(cratchit_tests PRIVATE cratchit_core cratchit_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cratchit_tests)
