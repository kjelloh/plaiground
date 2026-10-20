find_package(GTest REQUIRED)

add_executable(cratchit_tests
  test_data_folder.cpp
  test_help.cpp
  test_manual.cpp
  test_path_editor.cpp
  test_path_value.cpp
  test_repl.cpp
  test_version.cpp
  test_workspace.cpp
)
target_link_libraries(cratchit_tests PRIVATE cratchit_core cratchit_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cratchit_tests)
