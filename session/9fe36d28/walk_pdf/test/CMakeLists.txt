find_package(GTest REQUIRED)

add_executable(walk_pdf_tests
  test_document.cpp
  test_path.cpp
  test_peek.cpp
  test_syntax.cpp
)
target_link_libraries(walk_pdf_tests PRIVATE walk_pdf_core walk_pdf_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(walk_pdf_tests)
