find_package(GTest REQUIRED)

add_executable(cratchit_tests
  test_amount.cpp
  test_books.cpp
  test_csv.cpp
  test_date.cpp
  test_import.cpp
  test_inventory.cpp
  test_sie_check.cpp
  test_sie_document.cpp
  test_sie_export.cpp
  test_sie_file.cpp
  test_sie_import.cpp
  test_sie_lexer.cpp
  test_sie_project.cpp
  test_sha256.cpp
  test_sie_summary.cpp
  test_statement.cpp
  test_statement_drafts.cpp
  test_event_draft.cpp
  test_pdf_documents.cpp
  test_cp437.cpp
  test_decoder.cpp
  test_design_insufficiency.cpp
  test_detect.cpp
  test_details_project.cpp
  test_error_state.cpp
  test_fiscal_year.cpp
  test_dot_folder.cpp
  test_input_buffer.cpp
  test_key_parser.cpp
  test_key_reader.cpp
  test_log.cpp
  test_manual.cpp
  test_path_completion.cpp
  test_pager.cpp
  test_path_value.cpp
  test_project.cpp
  test_prompt_loop.cpp
  test_state.cpp
  test_terminal_encoding.cpp
  test_today.cpp
  test_transcoder.cpp
  test_ui.cpp
  test_year_view.cpp
  test_utf16.cpp
  test_utf8.cpp
  test_version.cpp
)
# The manual in the source tree, so that the tests check the one that is edited
target_compile_definitions(cratchit_tests PRIVATE CRATCHIT_MANUAL_FILE="${PROJECT_SOURCE_DIR}/docs/anvandarmanual.md")
target_link_libraries(cratchit_tests PRIVATE cratchit_core cratchit_ui pyroceph cratchit_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cratchit_tests)

# walk_pdf's own tests (session 9fe36d28), those that need no PDF files of the user
add_executable(walk_pdf_tests
  walk_pdf/test_api.cpp
  walk_pdf/test_crypto.cpp
  walk_pdf/test_document.cpp
  walk_pdf/test_filters.cpp
  walk_pdf/test_fonts.cpp
  walk_pdf/test_path.cpp
  walk_pdf/test_plain_text.cpp
  walk_pdf/test_references.cpp
  walk_pdf/test_security.cpp
  walk_pdf/test_single_byte.cpp
  walk_pdf/test_syntax.cpp
  walk_pdf/test_text_string.cpp
)
target_include_directories(walk_pdf_tests PRIVATE ${PROJECT_SOURCE_DIR}/src/walk_pdf)
target_link_libraries(walk_pdf_tests PRIVATE walk_pdf cratchit_warnings GTest::gtest_main ZLIB::ZLIB)
gtest_discover_tests(walk_pdf_tests)
