find_package(GTest REQUIRED)

add_executable(cratchit_tests
  test_help.cpp
  test_manual.cpp
  test_repl.cpp
  test_version.cpp
)
target_link_libraries(cratchit_tests PRIVATE cratchit_core cratchit_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cratchit_tests)
