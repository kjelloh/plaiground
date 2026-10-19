#include "core/text.hpp"

namespace cratchit::text {

std::string_view trim(std::string_view s) {
  auto const is_blank = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
  while (!s.empty() && is_blank(s.front())) s.remove_prefix(1);
  while (!s.empty() && is_blank(s.back())) s.remove_suffix(1);
  return s;
}

std::vector<std::string_view> split_lines(std::string_view s) {
  std::vector<std::string_view> lines;
  while (!s.empty()) {
    auto const end = s.find('\n');
    auto line = s.substr(0, end);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    lines.push_back(line);
    if (end == std::string_view::npos) break;
    s.remove_prefix(end + 1);
  }
  return lines;
}

std::vector<std::string> split_list(std::string_view s, char separator) {
  std::vector<std::string> parts;
  if (trim(s).empty()) return parts;
  while (true) {
    auto const end = s.find(separator);
    parts.emplace_back(trim(s.substr(0, end)));
    if (end == std::string_view::npos) break;
    s.remove_prefix(end + 1);
  }
  return parts;
}

std::string latin1_to_utf8(std::string_view latin1) {
  std::string out;
  for (char const byte : latin1) {
    auto const c = static_cast<unsigned char>(byte);
    if (c < 0x80) {
      out += static_cast<char>(c);
    } else {
      out += static_cast<char>(0xC0 | (c >> 6));
      out += static_cast<char>(0x80 | (c & 0x3F));
    }
  }
  return out;
}

std::expected<std::string, std::string> utf8_to_latin1(std::string_view utf8) {
  std::string out;
  for (std::size_t i = 0; i < utf8.size(); ++i) {
    auto const c = static_cast<unsigned char>(utf8[i]);
    if (c < 0x80) {
      out += static_cast<char>(c);
      continue;
    }
    if ((c == 0xC2 || c == 0xC3) && i + 1 < utf8.size()) {
      auto const next = static_cast<unsigned char>(utf8[++i]);
      out += static_cast<char>(((c & 0x03) << 6) | (next & 0x3F));
      continue;
    }
    return std::unexpected("character outside ISO-8859-1 at byte " + std::to_string(i));
  }
  return out;
}

std::string_view strip_bom(std::string_view s) {
  if (s.starts_with("\xEF\xBB\xBF")) s.remove_prefix(3);
  return s;
}

} // namespace cratchit::text
