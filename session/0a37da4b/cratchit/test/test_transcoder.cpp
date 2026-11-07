#include "charset/transcoder.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

using charset::BasicTranscoder;
using charset::Bytes;
using charset::Encoding;
using charset::make_transcoder;

namespace {

using ByteVector = std::vector<std::uint8_t>;

ByteVector to_vector(Bytes const& bytes) { return ByteVector(bytes.begin(), bytes.end()); }

// Pushes all bytes, then finishes, and collects everything given back
template <class T>
ByteVector transcode(T& transcoder, ByteVector const& input) {
  ByteVector output;
  for (auto byte : input) {
    if (auto bytes = transcoder.push(byte)) output.insert(output.end(), bytes->begin(), bytes->end());
  }
  if (auto bytes = transcoder.finish()) output.insert(output.end(), bytes->begin(), bytes->end());
  return output;
}

} // namespace

TEST(BasicTranscoder, Cp437ToUtf8GivesBytesOnEachPush) {
  BasicTranscoder<charset::cp437::Decoder, charset::utf8::Encoder> transcoder;
  auto bytes = transcoder.push(0x86); // å
  ASSERT_TRUE(bytes);
  EXPECT_EQ(to_vector(*bytes), (ByteVector{0xC3, 0xA5}));
  EXPECT_FALSE(transcoder.finish());
}

TEST(BasicTranscoder, Utf8ToCp437WaitsForWholeCharacter) {
  BasicTranscoder<charset::utf8::Decoder, charset::cp437::Encoder> transcoder;
  EXPECT_FALSE(transcoder.push(0xC3));
  auto bytes = transcoder.push(0xA5); // å
  ASSERT_TRUE(bytes);
  EXPECT_EQ(to_vector(*bytes), (ByteVector{0x86}));
}

TEST(BasicTranscoder, CharacterMissingInTargetGivesNothing) {
  BasicTranscoder<charset::utf8::Decoder, charset::cp437::Encoder> transcoder;
  EXPECT_FALSE(transcoder.push(0xE2)); // € is E2 82 AC, not in CP437
  EXPECT_FALSE(transcoder.push(0x82));
  EXPECT_FALSE(transcoder.push(0xAC));
  EXPECT_FALSE(transcoder.finish());
}

TEST(BasicTranscoder, FinishGivesReplacementForUnfinishedInput) {
  BasicTranscoder<charset::utf8::Decoder, charset::utf8::Encoder> transcoder;
  EXPECT_FALSE(transcoder.push(0xC3));
  auto bytes = transcoder.finish();
  ASSERT_TRUE(bytes);
  EXPECT_EQ(to_vector(*bytes), (ByteVector{0xEF, 0xBF, 0xBD})); // U+FFFD
}

TEST(Transcoder, Cp437ToUtf8) {
  auto transcoder = make_transcoder(Encoding::cp437, Encoding::utf8);
  EXPECT_EQ(transcode(transcoder, {'K', 0x94, 'p'}), (ByteVector{'K', 0xC3, 0xB6, 'p'})); // Köp
}

TEST(Transcoder, Utf8ToUtf16le) {
  auto transcoder = make_transcoder(Encoding::utf8, Encoding::utf16le);
  EXPECT_EQ(transcode(transcoder, {'A', 0xC3, 0xA4}), (ByteVector{'A', 0x00, 0xE4, 0x00})); // Aä
}

TEST(Transcoder, Utf16beSurrogatePairToUtf8) {
  auto transcoder = make_transcoder(Encoding::utf16be, Encoding::utf8);
  // U+1F600 is D83D DE00 in UTF-16 and F0 9F 98 80 in UTF-8
  EXPECT_EQ(transcode(transcoder, {0xD8, 0x3D, 0xDE, 0x00}), (ByteVector{0xF0, 0x9F, 0x98, 0x80}));
}

TEST(Transcoder, Utf16leToCp437) {
  auto transcoder = make_transcoder(Encoding::utf16le, Encoding::cp437);
  EXPECT_EQ(transcode(transcoder, {0xD6, 0x00, 'l'}), (ByteVector{0x99})); // Ö, then an odd byte
}

TEST(Transcoder, EveryPairCanBeMade) {
  for (auto from : {Encoding::utf8, Encoding::utf16be, Encoding::utf16le, Encoding::cp437}) {
    for (auto to : {Encoding::utf8, Encoding::utf16be, Encoding::utf16le, Encoding::cp437}) {
      auto transcoder = make_transcoder(from, to);
      EXPECT_FALSE(transcoder.finish());
    }
  }
}
