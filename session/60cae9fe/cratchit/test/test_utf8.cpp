#include "core/charset/cp437.hpp"
#include "core/charset/utf8.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

using charset::utf8::FromUnicodeBuffer;

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

// Each code point goes straight from one buffer into the other
TEST(Utf8, TranscodesCp437ToUtf8) {
  charset::cp437::ToUnicodeBuffer to_unicode;
  FromUnicodeBuffer from_unicode;
  std::string result;
  for (char const c : std::string_view{"\x8Fr \x84r \x94l\r\n"}) {
    if (auto const code_point = to_unicode.push(static_cast<charset::cp437::CodePoint>(c))) {
      for (auto const byte : from_unicode.push(*code_point)) {
        result.push_back(static_cast<char>(byte));
      }
    }
  }
  EXPECT_EQ(result, "\xC3\x85r \xC3\xA4r \xC3\xB6l\r\n"); // År är öl
}
