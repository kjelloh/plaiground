#include "peek.hpp"

#include "bytes.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <string_view>
#include <vector>

namespace walk_pdf {

namespace {

bool is_printable(char c) { return c >= ' ' && c <= '~'; }

// Text is printable ASCII and whitespace. Allow some other bytes, as PDF text
// may hold e.g. Latin-1 letters, but call it binary if more than a quarter are.
bool is_binary(std::string_view bytes) {
  auto const other = std::ranges::count_if(
      bytes, [](char c) { return !is_printable(c) && !is_whitespace(c); });
  return static_cast<std::size_t>(other) * 4 > bytes.size();
}

// Whitespace runs as one space and other bytes outside printable ASCII as '.',
// trimmed. Stops a little after limit characters, as the rest is cut anyway.
std::string squeeze(std::string_view bytes, std::size_t limit) {
  std::string result;
  bool space = false;
  for (auto const c : bytes) {
    if (result.size() > limit) {
      break;
    }
    if (is_whitespace(c)) {
      space = !result.empty();
      continue;
    }
    if (space) {
      result += ' ';
      space = false;
    }
    result += is_printable(c) ? c : '.';
  }
  return result;
}

// Text cut to width, ending with "..." when cut
std::string fit(std::string text, std::size_t width) {
  if (text.size() <= width) {
    return text;
  }
  width = std::max<std::size_t>(width, 3);
  text.resize(width - 3);
  return text + "...";
}

// Does it start like PDF syntax: a dictionary, array, string or name?
// Then it is shown as text even if its strings hold binary (e.g. encrypted) bytes.
bool starts_as_syntax(std::string_view bytes) {
  Bytes const all{bytes};
  auto const c = all.at(all.skip_whitespace(0));
  return c == '<' || c == '[' || c == '(' || c == '/';
}

std::string text_or_binary(std::string_view bytes, std::size_t width) {
  if (!starts_as_syntax(bytes) && is_binary(bytes)) {
    return std::format("<{} bytes, binary>", bytes.size());
  }
  return fit(squeeze(bytes, width), width);
}

std::string count(std::size_t n, std::string_view singular, std::string_view plural) {
  return std::format("{} {}", n, n == 1 ? singular : plural);
}

std::string join(std::vector<std::string> const& parts, std::string_view separator) {
  std::string result;
  for (auto const& part : parts) {
    if (!result.empty()) {
      result += separator;
    }
    result += part;
  }
  return result;
}

// The bytes after a leading keyword such as "trailer"
std::string_view after(std::string_view bytes, std::string_view keyword) {
  return bytes.starts_with(keyword) ? bytes.substr(keyword.size()) : bytes;
}

// The names in a dictionary's /Filter entry: "/Filter /FlateDecode" or
// "/Filter [ /ASCII85Decode /FlateDecode ]". Found by a search for "/Filter";
// tokenizing objects will make this exact.
std::string filter_names(std::string_view dictionary) {
  auto pos = dictionary.find("/Filter");
  while (pos != std::string_view::npos && pos + 7 < dictionary.size() &&
         is_regular(dictionary[pos + 7])) {
    pos = dictionary.find("/Filter", pos + 1); // e.g. "/FilterX" is another key
  }
  if (pos == std::string_view::npos) {
    return {};
  }
  Bytes const bytes{dictionary};
  pos = bytes.skip_whitespace(pos + 7);
  auto const in_array = bytes.at(pos) == '[';
  if (in_array) {
    pos = bytes.skip_whitespace(pos + 1);
  }
  std::vector<std::string> names;
  while (bytes.at(pos) == '/') {
    auto const begin = pos + 1;
    auto end = begin;
    while (end < bytes.size() && is_regular(bytes.at(end))) {
      ++end;
    }
    names.emplace_back(dictionary.substr(begin, end - begin));
    if (!in_array) {
      break;
    }
    pos = bytes.skip_whitespace(end);
  }
  return join(names, " ");
}

std::string peek_header(std::string_view bytes) {
  return "version " + squeeze(after(bytes, "%PDF-"), 20);
}

std::string peek_comment(std::string_view bytes, std::size_t width) {
  auto const text = after(bytes, "%");
  // A comment of only non-ASCII bytes near the start says the file holds binary data
  if (!text.empty() && std::ranges::all_of(text, [](char c) { return (c & 0x80) != 0; })) {
    return std::format("<binary marker, {} bytes>", text.size());
  }
  return text_or_binary(text, width);
}

// An object's body, after "n g obj" and before "endobj". If it holds a stream,
// its dictionary is shown and the stream data described.
std::string peek_object(std::string_view bytes, std::size_t width) {
  Bytes const all{bytes};
  auto const begin = all.find_keyword(0, "obj").value_or(0) + 3;
  auto end = bytes.size();
  if (auto const endobj = bytes.rfind("endobj");
      endobj != std::string_view::npos && endobj >= begin && all.keyword_at(endobj, "endobj")) {
    end = endobj;
  }
  auto const stream = all.find_keyword(begin, "stream", end);
  if (!stream) {
    return text_or_binary(bytes.substr(begin, end - begin), width);
  }

  // The data starts after the EOL that follows "stream", and ends before the
  // EOL that precedes "endstream"
  auto const data_begin = all.end_of_line(*stream + 6);
  auto data_end = all.find_keyword(data_begin, "endstream", end).value_or(end);
  if (data_end > data_begin && bytes[data_end - 1] == '\n') {
    --data_end;
  }
  if (data_end > data_begin && bytes[data_end - 1] == '\r') {
    --data_end;
  }
  auto const dictionary = bytes.substr(begin, *stream - begin);
  auto const filters = filter_names(dictionary);
  auto const size = data_end - data_begin;
  auto const description = filters.empty() ? std::format(" + stream <{} bytes>", size)
                                           : std::format(" + stream <{} bytes, {}>", size, filters);
  // Always show the description, and the dictionary in whatever width is left
  auto const dictionary_width =
      std::max<std::size_t>(width > description.size() ? width - description.size() : 0, 20);
  return fit(squeeze(dictionary, dictionary_width), dictionary_width) + description;
}

// Entries and object numbers from the subsection lines "first count"
std::string peek_xref(std::string_view bytes) {
  Bytes const all{bytes};
  auto pos = all.end_of_line(0); // past "xref"
  std::size_t subsections = 0;
  std::uint64_t entries = 0;
  std::uint64_t first_object = 0;
  while (pos < all.size()) {
    pos = all.skip_whitespace(pos);
    auto const first = all.integer_at<std::uint64_t>(pos);
    if (!first) {
      break;
    }
    auto const n = all.integer_at<std::uint64_t>(all.skip_whitespace(first->second));
    if (!n) {
      break;
    }
    if (subsections == 0) {
      first_object = first->first;
    }
    ++subsections;
    entries += n->first;
    pos = all.end_of_line(n->second);
    for (std::uint64_t i = 0; i < n->first && pos < all.size(); ++i) {
      pos = all.end_of_line(pos); // one line per entry
    }
  }
  if (subsections == 1 && entries > 0) {
    return std::format("{}, objects {}-{}", count(entries, "entry", "entries"), first_object,
                       first_object + entries - 1);
  }
  return std::format("{} in {}", count(entries, "entry", "entries"),
                     count(subsections, "subsection", "subsections"));
}

// How many children of each kind
std::array<std::size_t, 16> count_children(Content const& content, std::size_t& total) {
  std::array<std::size_t, 16> counts{};
  total = 0;
  for (auto const& [path, child] : content) {
    ++counts[static_cast<std::size_t>(child.kind())];
    ++total;
  }
  return counts;
}

std::string peek_revision(Content const& content) {
  std::size_t total = 0;
  auto const counts = count_children(content, total);
  auto const of = [&](Kind kind) { return counts[static_cast<std::size_t>(kind)]; };

  std::vector<std::string> parts;
  parts.push_back(count(of(Kind::object), "object", "objects"));
  for (auto const kind : {Kind::xref, Kind::trailer}) {
    if (of(kind) == 1) {
      parts.emplace_back(to_string(kind));
    } else if (of(kind) > 1) {
      parts.push_back(std::format("{} {}", of(kind), to_string(kind)));
    }
  }
  if (of(Kind::unknown) > 0) {
    parts.push_back(std::format("{} unknown", of(Kind::unknown)));
  }
  return std::format("{}: {}", count(total, "element", "elements"), join(parts, ", "));
}

} // namespace

std::string peek(Content const& content, std::size_t width) {
  auto const bytes = content.bytes();
  switch (content.kind()) {
  case Kind::document: {
    std::size_t total = 0;
    count_children(content, total);
    return count(total, "revision", "revisions");
  }
  case Kind::revision: return peek_revision(content);
  case Kind::header: return peek_header(bytes);
  case Kind::comment: return peek_comment(bytes, width);
  case Kind::object: return peek_object(bytes, width);
  case Kind::xref: return peek_xref(bytes);
  case Kind::trailer: return text_or_binary(after(bytes, "trailer"), width);
  case Kind::startxref: return squeeze(after(bytes, "startxref"), width);
  case Kind::eof: return {};
  case Kind::unknown: return text_or_binary(bytes, width);
  }
  return {};
}

} // namespace walk_pdf
