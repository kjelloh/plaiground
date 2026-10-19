#include "sie/sie_reader.hpp"

#include "core/account.hpp"
#include "core/amount.hpp"
#include "core/date.hpp"
#include "core/text.hpp"
#include "sie/cp437.hpp"

#include <fstream>
#include <set>
#include <sstream>

namespace cratchit::sie {

namespace {

const std::set<std::string, std::less<>> known_types{"string", "word", "int", "year_index", "account",
                                                     "amount", "decimal", "date", "period", "object_list"};

bool is_integer(std::string_view s) {
  if (!s.empty() && s.front() == '-') s.remove_prefix(1);
  if (s.empty()) return false;
  for (char c : s) {
    if (c < '0' || c > '9') return false;
  }
  return true;
}

bool is_decimal(std::string_view s) {
  if (!s.empty() && s.front() == '-') s.remove_prefix(1);
  if (s.empty()) return false;
  int points = 0;
  for (char c : s) {
    if (c == '.') ++points;
    else if (c < '0' || c > '9') return false;
  }
  return points <= 1;
}

std::expected<void, std::string> check_type(std::string_view type, std::string_view value) {
  bool ok = true;
  if (type == "word") ok = !value.empty();
  else if (type == "int" || type == "year_index") ok = is_integer(value);
  else if (type == "account") ok = parse_account_no(value).has_value();
  else if (type == "amount") ok = Amount::parse(value).has_value();
  else if (type == "decimal") ok = is_decimal(value);
  else if (type == "date") ok = parse_date(value).has_value();
  else if (type == "period") ok = value.size() == 6 && is_integer(value);
  else if (type == "object_list") ok = value.starts_with('{');
  if (!ok) return std::unexpected("\"" + std::string{value} + "\" is not a valid " + std::string{type});
  return {};
}

struct Token {
  std::string m_text;
  bool m_quoted{};
};

// Splits the part of a line after the label into tokens: "quoted \" strings", {object lists} and bare words.
std::expected<std::vector<Token>, std::string> tokenize(std::string_view s) {
  std::vector<Token> tokens;
  std::size_t i = 0;
  while (true) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    if (i == s.size()) break;
    if (s[i] == '"') {
      std::string text;
      ++i;
      while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < s.size()) ++i;
        text += s[i++];
      }
      if (i == s.size()) return std::unexpected("missing closing '\"'");
      ++i;
      tokens.push_back({text, true});
    } else if (s[i] == '{') {
      auto const start = i;
      bool in_quotes = false;
      while (i < s.size() && (in_quotes || s[i] != '}')) {
        if (s[i] == '\\' && in_quotes) ++i;
        else if (s[i] == '"') in_quotes = !in_quotes;
        ++i;
      }
      if (i == s.size()) return std::unexpected("missing closing '}'");
      ++i;
      tokens.push_back({std::string{s.substr(start, i - start)}, false});
    } else {
      auto const start = i;
      while (i < s.size() && s[i] != ' ' && s[i] != '\t') ++i;
      tokens.push_back({std::string{s.substr(start, i - start)}, false});
    }
  }
  return tokens;
}

} // namespace

std::expected<SieSpec, std::string> SieSpec::from(cspec::SpecTree const& spec) {
  SieSpec sie;
  sie.m_encoding = spec.value_or("file.encoding", "cp437");
  for (auto const& label : spec.children("record")) {
    auto const base = "record." + label + ".";
    RecordSpec record{label, {}, spec.value_or(base + "use", "ignore"), spec.value_or(base + "block", "no") == "yes",
                      spec.value_or(base + "in_block", "")};
    for (auto const& key : spec.children(base + "field")) {
      auto const path = base + "field." + key;
      auto const parts = text::split_list(spec.value_or(path, ""), ' ');
      if (parts.size() != 3 || !known_types.contains(parts[1]) || (parts[2] != "required" && parts[2] != "optional")) {
        return std::unexpected(spec.where(path) + ": expected '<name> <type> <required|optional>'");
      }
      record.m_fields.push_back({parts[0], parts[1], parts[2] == "required"});
    }
    sie.m_records.emplace(label, std::move(record));
  }
  return sie;
}

RecordSpec const* SieSpec::find(std::string_view label) const {
  auto const it = m_records.find(label);
  return it == m_records.end() ? nullptr : &it->second;
}

std::expected<std::vector<Record>, std::string> read_records(std::string_view utf8_text, SieSpec const& spec,
                                                             std::string const& source_name) {
  std::vector<Record> records;
  Record* open_block = nullptr; // the record whose { … } we are inside
  bool expecting_block = false;
  int line_number = 0;
  for (auto line : text::split_lines(utf8_text)) {
    ++line_number;
    auto const fail = [&](std::string const& message) {
      return std::unexpected(source_name + ":" + std::to_string(line_number) + ": " + message);
    };
    auto const trimmed = text::trim(line);
    if (trimmed.empty()) continue;
    if (trimmed == "{") {
      if (!expecting_block || records.empty()) return fail("unexpected '{'");
      open_block = &records.back();
      expecting_block = false;
      continue;
    }
    if (trimmed == "}") {
      if (open_block == nullptr) return fail("unexpected '}'");
      open_block = nullptr;
      continue;
    }
    if (trimmed.front() != '#') return fail("expected a #LABEL line");

    auto const label_end = trimmed.find_first_of(" \t");
    Record record{std::string{trimmed.substr(1, label_end == std::string_view::npos ? trimmed.npos : label_end - 1)}, {}, false, line_number, {}};
    auto tokens = tokenize(label_end == std::string_view::npos ? std::string_view{} : trimmed.substr(label_end));
    if (!tokens) return fail(tokens.error());

    auto const* record_spec = spec.find(record.m_label);
    if (record_spec == nullptr) {
      for (std::size_t i = 0; i < tokens->size(); ++i) record.m_fields[std::to_string(i + 1)] = (*tokens)[i].m_text;
    } else {
      record.m_known = true;
      if (tokens->size() > record_spec->m_fields.size()) {
        return fail("#" + record.m_label + " has " + std::to_string(tokens->size()) + " fields, the spec allows " +
                    std::to_string(record_spec->m_fields.size()));
      }
      for (std::size_t i = 0; i < record_spec->m_fields.size(); ++i) {
        auto const& field = record_spec->m_fields[i];
        if (i >= tokens->size()) {
          if (field.m_required) return fail("#" + record.m_label + " is missing field '" + field.m_name + "'");
          continue;
        }
        auto const& token = (*tokens)[i];
        bool const empty_optional = token.m_text.empty() && !field.m_required;
        if (!empty_optional && field.m_type != "string") {
          if (auto ok = check_type(field.m_type, token.m_text); !ok) {
            return fail("#" + record.m_label + " field '" + field.m_name + "': " + ok.error());
          }
        }
        record.m_fields[field.m_name] = token.m_text;
      }
      if (!record_spec->m_in_block.empty() && open_block == nullptr) {
        return fail("#" + record.m_label + " must be inside a #" + record_spec->m_in_block + " block");
      }
    }

    if (open_block != nullptr) {
      open_block->m_block.push_back(std::move(record));
    } else {
      expecting_block = record_spec != nullptr && record_spec->m_block;
      records.push_back(std::move(record));
    }
  }
  if (open_block != nullptr) return std::unexpected(source_name + ": missing '}' at end of file");
  return records;
}

std::expected<std::vector<Record>, std::string> read_file(std::filesystem::path const& path, SieSpec const& spec) {
  std::ifstream file{path, std::ios::binary};
  if (!file) return std::unexpected("cannot open " + path.string());
  std::ostringstream content;
  content << file.rdbuf();
  if (spec.encoding() != "cp437") return std::unexpected("unsupported SIE encoding in spec: " + spec.encoding());
  return read_records(cp437_to_utf8(content.str()), spec, path.string());
}

} // namespace cratchit::sie
