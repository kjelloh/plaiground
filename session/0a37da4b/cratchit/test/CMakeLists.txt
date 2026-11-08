find_package(GTest REQUIRED)

add_executable(cratchit_tests
  test_cp437.cpp
  test_decoder.cpp
  test_design_insufficiency.cpp
  test_detect.cpp
  test_error_state.cpp
  test_fiscal_year.cpp
  test_dot_folder.cpp
  test_input_buffer.cpp
  test_key_parser.cpp
  test_key_reader.cpp
  test_log.cpp
  test_manual.cpp
  test_path_completion.cpp
  test_path_value.cpp
  test_project.cpp
  test_prompt_loop.cpp
  test_state.cpp
  test_terminal_encoding.cpp
  test_transcoder.cpp
  test_ui.cpp
  test_utf16.cpp
  test_utf8.cpp
)
# The manual in the source tree, so that the tests check the one that is edited
target_compile_definitions(cratchit_tests PRIVATE CRATCHIT_MANUAL_FILE="${PROJECT_SOURCE_DIR}/docs/anvandarmanual.md")
target_link_libraries(cratchit_tests PRIVATE cratchit_core cratchit_ui pyroceph cratchit_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cratchit_tests)
