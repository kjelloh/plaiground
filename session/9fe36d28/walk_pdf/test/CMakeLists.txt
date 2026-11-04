find_package(GTest REQUIRED)

add_executable(walk_pdf_tests
  test_api.cpp
  test_cp437.cpp
  test_crypto.cpp
  test_display.cpp
  test_document.cpp
  test_filters.cpp
  test_fonts.cpp
  test_path.cpp
  test_peek.cpp
  test_plain_text.cpp
  test_references.cpp
  test_security.cpp
  test_single_byte.cpp
  test_syntax.cpp
  test_text_string.cpp
  test_utf16.cpp
  test_utf8.cpp
)
target_link_libraries(walk_pdf_tests PRIVATE walk_pdf_core walk_pdf_warnings GTest::gtest_main ZLIB::ZLIB)

include(GoogleTest)
gtest_discover_tests(walk_pdf_tests)
