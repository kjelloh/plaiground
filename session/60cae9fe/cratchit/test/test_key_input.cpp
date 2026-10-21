#include "core/key_input.hpp"

#include <gtest/gtest.h>

#include <string>

using cratchit::Charset;
using cratchit::KeyReader;

namespace {

// Reads all keys of bytes in charset
std::u32string keys_of(Charset charset, std::string bytes) {
  KeyReader reader(charset, [bytes = std::move(bytes), pos = std::size_t{0}]() mutable -> std::optional<char> {
    if (pos == bytes.size()) {
      return std::nullopt;
    }
    return bytes[pos++];
  });
  std::u32string result;
  while (auto const key = reader.next()) {
    result += *key;
  }
  return result;
}

} // namespace

TEST(KeyReader, ReadsAsciiAsIs) {
  EXPECT_EQ(keys_of(Charset::Utf8, "hq\x04"), U"hq\x04");
  EXPECT_EQ(keys_of(Charset::Cp437, "hq\x04"), U"hq\x04");
}

TEST(KeyReader, DecodesUtf8) {
  EXPECT_EQ(keys_of(Charset::Utf8, "R\xc3\xa4kenskaper"), U"Räkenskaper");
}

TEST(KeyReader, DecodesCp437) {
  EXPECT_EQ(keys_of(Charset::Cp437, "R\x84kenskaper"), U"Räkenskaper");
}

TEST(KeyReader, ReadsAnEncodingItCannotReadAsUtf8) {
  EXPECT_EQ(keys_of(Charset::Cp850, "\xc3\xa5"), U"å");
}

TEST(KeyReader, BytesThatAreNotUtf8GiveTheReplacementCharacter) {
  EXPECT_EQ(keys_of(Charset::Utf8, "a\x84" "b"), U"a\xFFFD" "b");
}

TEST(KeyReader, AnUnfinishedSequenceAtEndOfInputGivesTheReplacementCharacter) {
  EXPECT_EQ(keys_of(Charset::Utf8, "a\xc3"), U"a\xFFFD");
}

TEST(KeyReader, NothingAfterEndOfInput) {
  KeyReader reader(Charset::Utf8, [] { return std::optional<char>{}; });
  EXPECT_FALSE(reader.next());
  EXPECT_FALSE(reader.next());
}
