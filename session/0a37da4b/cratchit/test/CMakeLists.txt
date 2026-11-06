find_package(GTest REQUIRED)

add_executable(cratchit_tests
  test_cp437.cpp
  test_detect.cpp
  test_dot_folder.cpp
  test_log.cpp
  test_path_value.cpp
  test_terminal_encoding.cpp
  test_utf16.cpp
  test_utf8.cpp
)
target_link_libraries(cratchit_tests PRIVATE charset cratchit_core cratchit_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cratchit_tests)
