#include "core/charset/cp437.hpp"

#include <gtest/gtest.h>

#include <set>

using charset::cp437::CodePoint;
using charset::cp437::ToUnicodeBuffer;
using charset::cp437::to_unicode;

TEST(Cp437, AsciiIncludingControlCharactersIsUnchanged) {
  ToUnicodeBuffer buffer;
  for (unsigned byte = 0x00; byte < 0x80; ++byte) {
    EXPECT_EQ(buffer.push(static_cast<CodePoint>(byte)), char32_t(byte)) << "byte " << byte;
  }
}

TEST(Cp437, SwedishLetters) {
  ToUnicodeBuffer buffer;
  EXPECT_EQ(buffer.push(0x86), U'å');
  EXPECT_EQ(buffer.push(0x84), U'ä');
  EXPECT_EQ(buffer.push(0x94), U'ö');
  EXPECT_EQ(buffer.push(0x8F), U'Å');
  EXPECT_EQ(buffer.push(0x8E), U'Ä');
  EXPECT_EQ(buffer.push(0x99), U'Ö');
  EXPECT_EQ(buffer.push(0x82), U'é');
}

TEST(Cp437, SymbolsAndBoxDrawing) {
  ToUnicodeBuffer buffer;
  EXPECT_EQ(buffer.push(0x9E), U'₧'); // ₧
  EXPECT_EQ(buffer.push(0xB0), U'░'); // ░
  EXPECT_EQ(buffer.push(0xC9), U'╔'); // ╔
  EXPECT_EQ(buffer.push(0xDB), U'█'); // █
  EXPECT_EQ(buffer.push(0xE1), U'ß'); // ß
  EXPECT_EQ(buffer.push(0xFF), U' '); // no-break space
}

TEST(Cp437, EveryByteGivesADistinctCodePoint) {
  ToUnicodeBuffer buffer;
  std::set<char32_t> seen;
  for (unsigned byte = 0x00; byte <= 0xFF; ++byte) {
    auto const code_point = buffer.push(static_cast<CodePoint>(byte));
    ASSERT_TRUE(code_point.has_value()) << "byte " << byte;
    EXPECT_TRUE(seen.insert(*code_point).second) << "byte " << byte;
  }
  EXPECT_EQ(seen.size(), 256u);
}

TEST(Cp437, ToUnicodeDecodesAText) {
  EXPECT_EQ(to_unicode("R\x84kning\r\n\x8Fr"), U"Räkning\r\nÅr");
  EXPECT_EQ(to_unicode(""), U"");
}
