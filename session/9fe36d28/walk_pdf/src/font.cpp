#include "font.hpp"

#include "charset/adobe_standard.hpp"
#include "charset/macroman.hpp"
#include "charset/utf8.hpp"
#include "charset/winansi.hpp"
#include "document.hpp"
#include "glyph_names.hpp"
#include "syntax.hpp"

#include <charconv>

namespace walk_pdf {

namespace {

std::optional<std::string> name_at(Document const& document, Path const& path) {
  auto const content = document.find(path);
  if (!content || content->kind() != Kind::name) {
    return std::nullopt;
  }
  return decode_name(content->bytes());
}

// A stream object's decoded data: the object a path leads to, then its stream
std::optional<std::string> stream_at(Document const& document, Path const& path) {
  auto const object = document.follow(path);
  if (!object) {
    return std::nullopt;
  }
  auto const decoded = document.decode(object->child(Segment::stream()));
  if (!decoded) {
    return std::nullopt;
  }
  return *decoded->bytes;
}

// A base encoding's code -> code point
std::optional<std::array<charset::unicode::CodePoint, 256>> base_encoding(std::string_view name) {
  std::array<charset::unicode::CodePoint, 256> table{};
  auto const fill = [&](auto decoder) {
    for (std::size_t code = 0; code < table.size(); ++code) {
      table[code] = decoder.push(static_cast<std::uint8_t>(code)).value_or(charset::unicode::replacement_character);
    }
    return table;
  };
  if (name == "WinAnsiEncoding") {
    return fill(charset::winansi::Decoder{});
  }
  if (name == "MacRomanEncoding") {
    return fill(charset::macroman::Decoder{});
  }
  if (name == "StandardEncoding") {
    return fill(charset::adobe_standard::Decoder{});
  }
  return std::nullopt; // e.g. MacExpertEncoding: not supported
}

void append_utf8(std::string& text, charset::unicode::CodePoint code_point) {
  for (auto const byte : charset::utf8::Encoder{}.push(code_point)) {
    text += static_cast<char>(byte);
  }
}

} // namespace

Font Font::load(Document const& document, Path const& font) {
  Font result;
  auto const entry = [&](std::string key) { return font.child(Segment::entry(std::move(key))); };
  auto const subtype = name_at(document, entry("Subtype")).value_or("");

  if (auto const data = stream_at(document, entry("ToUnicode"))) {
    result.to_unicode_ = CMap::parse(*data);
  }

  if (subtype == "Type0") {
    // Composite font: codes by its CMap. Identity-H/V and most predefined
    // CMaps use two bytes; an embedded CMap says itself.
    result.code_bytes_ = 2;
    if (auto const data = stream_at(document, entry("Encoding"))) {
      if (auto cmap = CMap::parse(*data); cmap.has_code_space()) {
        result.encoding_ = std::move(cmap);
      }
    }
    return result;
  }

  // Simple font: one byte per code, mapped by a base encoding and /Differences
  auto base = subtype == "TrueType" ? std::string{"WinAnsiEncoding"} : std::string{"StandardEncoding"};
  std::optional<Path> differences;
  if (auto const name = name_at(document, entry("Encoding"))) {
    base = *name;
  } else if (auto const encoding = document.follow(entry("Encoding"))) {
    if (auto const named = name_at(document, encoding->child(Segment::entry("BaseEncoding")))) {
      base = *named;
    }
    differences = encoding->child(Segment::entry("Differences"));
  }
  result.simple_ = base_encoding(base);
  if (!result.simple_) {
    result.simple_.emplace();
    result.simple_->fill(charset::unicode::replacement_character);
  }
  if (differences) {
    // [code /name /name ... code /name ...]: names for consecutive codes
    long long code = 0;
    for (auto const& item : document.children(*differences)) {
      auto const bytes = item.content.bytes();
      if (item.content.kind() == Kind::integer) {
        std::from_chars(bytes.data(), bytes.data() + bytes.size(), code);
      } else if (item.content.kind() == Kind::name) {
        if (code >= 0 && code < 256) {
          (*result.simple_)[static_cast<std::size_t>(code)] =
              glyph_unicode(decode_name(bytes)).value_or(charset::unicode::replacement_character);
        }
        ++code;
      }
    }
  }
  return result;
}

std::string Font::text(std::string_view codes) const {
  std::string result;
  for (std::size_t at = 0; at < codes.size();) {
    auto length = code_bytes_;
    if (encoding_) {
      length = encoding_->code_length(codes.substr(at)).value_or(code_bytes_);
    }
    length = std::min(length, codes.size() - at);
    auto const code = codes.substr(at, length);
    at += length;

    if (to_unicode_) {
      if (auto const text = to_unicode_->utf8(code)) {
        result += *text;
        continue;
      }
    }
    if (simple_ && length == 1) {
      append_utf8(result, (*simple_)[static_cast<unsigned char>(code[0])]);
      continue;
    }
    append_utf8(result, charset::unicode::replacement_character);
  }
  return result;
}

} // namespace walk_pdf
