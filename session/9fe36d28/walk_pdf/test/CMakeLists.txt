find_package(GTest REQUIRED)

add_executable(walk_pdf_tests
  test_cp437.cpp
  test_document.cpp
  test_path.cpp
  test_peek.cpp
  test_syntax.cpp
  test_text_string.cpp
  test_utf16.cpp
  test_utf8.cpp
)
target_link_libraries(walk_pdf_tests PRIVATE walk_pdf_core walk_pdf_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(walk_pdf_tests)
