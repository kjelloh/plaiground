#include "core/charset/cp437.hpp"
#include "core/charset/utf8.hpp"

#include <gtest/gtest.h>

#include <string>

using charset::utf8::FromUnicodeBuffer;
using charset::utf8::from_unicode;

namespace {

// The bytes of one push, as a string to compare against escapes
std::string encoded(char32_t code_point) {
  FromUnicodeBuffer buffer;
  std::string result;
  for (auto const byte : buffer.push(code_point)) {
    result.push_back(static_cast<char>(byte));
  }
  return result;
}

} // namespace

TEST(Utf8, AsciiIsOneByte) {
  EXPECT_EQ(encoded(0x00), std::string(1, '\0'));
  EXPECT_EQ(encoded(U'A'), "A");
  EXPECT_EQ(encoded(0x7F), "\x7F");
}

TEST(Utf8, TwoBytes) {
  EXPECT_EQ(encoded(0x80), "\xC2\x80");
  EXPECT_EQ(encoded(0xE4), "\xC3\xA4"); // ä
  EXPECT_EQ(encoded(0xC5), "\xC3\x85"); // Å
  EXPECT_EQ(encoded(0x7FF), "\xDF\xBF");
}

TEST(Utf8, ThreeBytes) {
  EXPECT_EQ(encoded(0x800), "\xE0\xA0\x80");
  EXPECT_EQ(encoded(0x20AC), "\xE2\x82\xAC"); // €
  EXPECT_EQ(encoded(0x2554), "\xE2\x95\x94"); // ╔
  EXPECT_EQ(encoded(0xFFFF), "\xEF\xBF\xBF");
}

TEST(Utf8, FourBytes) {
  EXPECT_EQ(encoded(0x10000), "\xF0\x90\x80\x80");
  EXPECT_EQ(encoded(0x1F600), "\xF0\x9F\x98\x80"); // 😀
  EXPECT_EQ(encoded(0x10FFFF), "\xF4\x8F\xBF\xBF");
}

TEST(Utf8, NonCharactersGiveReplacementCharacter) {
  EXPECT_EQ(encoded(0xD800), "\xEF\xBF\xBD");
  EXPECT_EQ(encoded(0xDFFF), "\xEF\xBF\xBD");
  EXPECT_EQ(encoded(0x110000), "\xEF\xBF\xBD");
}

TEST(Utf8, FromUnicodeEncodesAText) {
  EXPECT_EQ(from_unicode(U"Räkning €"), "R\xC3\xA4kning \xE2\x82\xAC");
  EXPECT_EQ(from_unicode(U""), "");
}

TEST(Utf8, TranscodesCp437ToUtf8) {
  auto const text = charset::cp437::to_unicode("\x8Fr \x84r \x94l\r\n");
  EXPECT_EQ(from_unicode(text), "\xC3\x85r \xC3\xA4r \xC3\xB6l\r\n"); // År är öl
}
