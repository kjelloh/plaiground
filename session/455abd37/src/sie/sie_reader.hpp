#pragma once

// Reads SIE 4 files into generic records. Which fields each #LABEL has, and their
// types, come from specs/sie/sie4.cspec, not from this code.

#include "cspec/spec_tree.hpp"

#include <expected>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::sie {

struct FieldSpec {
  std::string m_name;
  std::string m_type;
  bool m_required{};
};

struct RecordSpec {
  std::string m_label;
  std::vector<FieldSpec> m_fields;
  std::string m_use; // read | ignore | read-write
  bool m_block{};
  std::string m_in_block;
};

class SieSpec {
public:
  static std::expected<SieSpec, std::string> from(cspec::SpecTree const& spec);

  RecordSpec const* find(std::string_view label) const;
  std::map<std::string, RecordSpec, std::less<>> const& records() const { return m_records; }
  std::string const& encoding() const { return m_encoding; }

private:
  std::map<std::string, RecordSpec, std::less<>> m_records;
  std::string m_encoding;
};

struct Record {
  std::string m_label;                        // without '#'
  std::map<std::string, std::string> m_fields; // field name -> text (unquoted). Unknown labels: "1", "2", …
  bool m_known{};
  int m_line{};
  std::vector<Record> m_block; // sub-records between { and }
};

// 'utf8_text' is the file content already decoded to UTF-8.
std::expected<std::vector<Record>, std::string> read_records(std::string_view utf8_text, SieSpec const& spec,
                                                             std::string const& source_name = "sie");

// Reads and decodes the file (the encoding comes from the spec: cp437).
std::expected<std::vector<Record>, std::string> read_file(std::filesystem::path const& path, SieSpec const& spec);

} // namespace cratchit::sie
