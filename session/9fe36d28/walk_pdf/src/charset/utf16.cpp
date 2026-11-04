#include "charset/utf16.hpp"

namespace charset::utf16 {

namespace {

bool is_high_surrogate(unicode::CodePoint value) { return value >= 0xD800 && value <= 0xDBFF; }
bool is_low_surrogate(unicode::CodePoint value) { return value >= 0xDC00 && value <= 0xDFFF; }

void append(Bytes& bytes, CodeUnit unit, ByteOrder order) {
  auto const high = static_cast<Byte>(unit >> 8);
  auto const low = static_cast<Byte>(unit & 0xFF);
  bytes.bytes[bytes.size++] = order == ByteOrder::big_endian ? high : low;
  bytes.bytes[bytes.size++] = order == ByteOrder::big_endian ? low : high;
}

void append(CodePoints& code_points, unicode::CodePoint code_point) {
  code_points.code_points[code_points.size++] = code_point;
}

} // namespace

Bytes FromUnicodeBuffer::push(unicode::CodePoint code_point) {
  Bytes result;
  if ((code_point >= 0xD800 && code_point <= 0xDFFF) || code_point > 0x10FFFF) {
    code_point = unicode::replacement_character;
  }
  if (code_point < 0x10000) {
    append(result, static_cast<CodeUnit>(code_point), order_);
    return result;
  }
  auto const value = code_point - 0x10000;
  append(result, static_cast<CodeUnit>(0xD800 + (value >> 10)), order_);
  append(result, static_cast<CodeUnit>(0xDC00 + (value & 0x3FF)), order_);
  return result;
}

CodePoints ToUnicodeBuffer::push(Byte byte) {
  if (!first_byte_) {
    first_byte_ = byte;
    return {};
  }
  auto const first = static_cast<CodeUnit>(*first_byte_);
  first_byte_.reset();
  auto const unit = order_ == ByteOrder::big_endian ? static_cast<CodeUnit>((first << 8) | byte)
                                                    : static_cast<CodeUnit>((byte << 8) | first);
  return take(unit);
}

CodePoints ToUnicodeBuffer::take(CodeUnit unit) {
  CodePoints result;
  if (high_) {
    if (is_low_surrogate(unit)) {
      append(result, 0x10000 + ((static_cast<unicode::CodePoint>(*high_) - 0xD800) << 10) +
                         (static_cast<unicode::CodePoint>(unit) - 0xDC00));
      high_.reset();
      return result;
    }
    // The high surrogate is alone; this unit is still taken below
    append(result, unicode::replacement_character);
    ++failure_count_;
    high_.reset();
  }
  if (is_high_surrogate(unit)) {
    high_ = unit;
  } else if (is_low_surrogate(unit)) {
    append(result, unicode::replacement_character);
    ++failure_count_;
  } else {
    append(result, unit);
  }
  return result;
}

std::optional<unicode::CodePoint> ToUnicodeBuffer::finish() {
  // An odd last byte and a waiting high surrogate are one failure together
  auto const failed = first_byte_.has_value() || high_.has_value();
  first_byte_.reset();
  high_.reset();
  if (!failed) {
    return std::nullopt;
  }
  ++failure_count_;
  return unicode::replacement_character;
}

} // namespace charset::utf16
