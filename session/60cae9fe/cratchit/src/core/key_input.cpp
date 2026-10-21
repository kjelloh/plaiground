#include "core/key_input.hpp"

#include "core/log.hpp"

#include <cstddef>
#include <format>
#include <string>
#include <utility>

namespace cratchit {

namespace cp437 = charset::cp437;
namespace unicode = charset::unicode;
namespace utf8 = charset::utf8;

namespace {

// Hex of the failed bytes from index first on, e.g. "C3 28"
std::string hex_from(std::vector<utf8::CodePoint> const& bytes, std::size_t first) {
  std::string result;
  for (auto i = first; i < bytes.size(); ++i) {
    result += std::format("{}{:02X}", result.empty() ? "" : " ", bytes[i]);
  }
  return result;
}

// Runs one push or finish on a UTF-8 buffer and logs a sequence it failed
template <typename Step>
std::optional<unicode::CodePoint> logging_failure(utf8::ToUnicodeBuffer& buffer, Step step) {
  auto const failures_before = buffer.failure_count();
  auto const bytes_before = buffer.failed_bytes().size();
  auto const result = step(buffer);
  if (buffer.failure_count() > failures_before) {
    LOG_DESIGN_INSUFFICIENCY("Key input is not UTF-8: bytes {} read as U+FFFD",
                             hex_from(buffer.failed_bytes(), bytes_before));
  }
  return result;
}

} // namespace

KeyReader::KeyReader(Charset charset, ByteSource next_byte) : next_byte_(std::move(next_byte)) {
  if (charset == Charset::Cp437) {
    buffer_ = cp437::ToUnicodeBuffer{};
  }
  LOG_DEVELOPMENT_TRACE("Keys are read as {}", charset == Charset::Cp437 ? "CP437" : "UTF-8");
}

std::optional<unicode::CodePoint> KeyReader::next() {
  while (!finished_) {
    auto const byte = next_byte_();
    if (!byte) {
      finished_ = true;
      return finish();
    }
    if (auto const key = push(static_cast<std::uint8_t>(*byte))) {
      return key;
    }
  }
  return std::nullopt;
}

std::optional<unicode::CodePoint> KeyReader::push(std::uint8_t byte) {
  if (auto* buffer = std::get_if<utf8::ToUnicodeBuffer>(&buffer_)) {
    return logging_failure(*buffer, [byte](utf8::ToUnicodeBuffer& b) { return b.push(byte); });
  }
  return std::get<cp437::ToUnicodeBuffer>(buffer_).push(byte);
}

std::optional<unicode::CodePoint> KeyReader::finish() {
  if (auto* buffer = std::get_if<utf8::ToUnicodeBuffer>(&buffer_)) {
    return logging_failure(*buffer, [](utf8::ToUnicodeBuffer& b) { return b.finish(); });
  }
  return std::nullopt; // CP437 has no unfinished sequences
}

} // namespace cratchit
