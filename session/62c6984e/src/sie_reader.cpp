#include "sie_reader.hpp"

#include "cp437.hpp"
#include "sie_line.hpp"

#include <charconv>
#include <fstream>
#include <iterator>
#include <optional>
#include <vector>

namespace cratchit {

namespace {

std::string_view trim(std::string_view s) {
  auto const is_space = [](char c) { return c == ' ' || c == '\t' || c == '\r'; };
  while (!s.empty() && is_space(s.front())) s.remove_prefix(1);
  while (!s.empty() && is_space(s.back())) s.remove_suffix(1);
  return s;
}

std::expected<int, std::string> parse_int(std::string_view text) {
  int value = 0;
  auto const [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (text.empty() || ec != std::errc{} || end != text.data() + text.size()) {
    return std::unexpected("Invalid number: \"" + std::string{text} + "\"");
  }
  return value;
}

using Fields = std::vector<SieField>;

std::expected<void, std::string> require_fields(Fields const& fields, std::size_t count) {
  if (fields.size() < count) {
    return std::unexpected(fields.front().text + " needs at least " + std::to_string(count - 1) + " values");
  }
  return {};
}

std::expected<FinancialYear, std::string> parse_rar(Fields const& f) {
  if (auto ok = require_fields(f, 4); !ok) return std::unexpected(ok.error());
  auto const index = parse_int(f[1].text);
  if (!index) return std::unexpected(index.error());
  auto const start = parse_date(f[2].text);
  if (!start) return std::unexpected(start.error());
  auto const end = parse_date(f[3].text);
  if (!end) return std::unexpected(end.error());
  return FinancialYear{*index, *start, *end};
}

std::expected<Voucher, std::string> parse_ver(Fields const& f) {
  if (auto ok = require_fields(f, 4); !ok) return std::unexpected(ok.error());
  auto const date = parse_date(f[3].text);
  if (!date) return std::unexpected(date.error());
  Voucher voucher{.series = f[1].text, .number = f[2].text, .date = *date};
  if (f.size() > 4) voucher.text = f[4].text;
  return voucher;
}

std::expected<Posting, std::string> parse_trans(Fields const& f) {
  if (auto ok = require_fields(f, 4); !ok) return std::unexpected(ok.error());
  if (!f[2].is_object_list) return std::unexpected("#TRANS is missing its object list {}");
  auto const account = parse_int(f[1].text);
  if (!account) return std::unexpected(account.error());
  auto const amount = parse_amount(f[3].text);
  if (!amount) return std::unexpected(amount.error());

  Posting posting{.account = *account, .amount = *amount};
  if (f.size() > 4 && !f[4].text.empty()) {
    auto const date = parse_date(f[4].text);
    if (!date) return std::unexpected(date.error());
    posting.date = *date;
  }
  if (f.size() > 5) posting.text = f[5].text;
  return posting;
}

class Parser {
public:
  std::expected<SieDocument, std::string> parse(std::string_view text) {
    while (!text.empty()) {
      ++m_line_number;
      auto const newline = text.find('\n');
      auto const line = trim(text.substr(0, newline));
      text = newline == std::string_view::npos ? std::string_view{} : text.substr(newline + 1);

      if (auto ok = parse_line(line); !ok) {
        return std::unexpected("Line " + std::to_string(m_line_number) + ": " + ok.error());
      }
    }
    if (m_voucher) return std::unexpected("Unexpected end of file inside a #VER block");
    return std::move(m_document);
  }

private:
  enum class State { Records, ExpectBlockStart, InBlock };

  std::expected<void, std::string> parse_line(std::string_view line) {
    if (line.empty()) return {};

    if (m_state == State::ExpectBlockStart) {
      if (line != "{") return std::unexpected("Expected '{' after #VER");
      m_state = State::InBlock;
      return {};
    }
    if (line == "}") {
      if (m_state != State::InBlock) return std::unexpected("'}' without an open #VER block");
      m_document.vouchers.push_back(std::move(*m_voucher));
      m_voucher.reset();
      m_state = State::Records;
      return {};
    }

    auto const fields = split_sie_line(line);
    if (!fields) return std::unexpected(fields.error());
    if (fields->empty()) return {};
    auto const& label = fields->front().text;

    if (m_state == State::InBlock) {
      if (label == "#TRANS") {
        auto posting = parse_trans(*fields);
        if (!posting) return std::unexpected(posting.error());
        m_voucher->postings.push_back(std::move(*posting));
        return {};
      }
      if (label == "#BTRANS" || label == "#RTRANS") return {};
      return std::unexpected("Unexpected " + label + " inside a #VER block");
    }

    if (label == "#ORGNR") {
      if (auto ok = require_fields(*fields, 2); !ok) return ok;
      m_document.org_nr = (*fields)[1].text;
    } else if (label == "#RAR") {
      auto year = parse_rar(*fields);
      if (!year) return std::unexpected(year.error());
      m_document.financial_years.push_back(*year);
    } else if (label == "#KONTO") {
      if (auto ok = require_fields(*fields, 3); !ok) return ok;
      auto const account = parse_int((*fields)[1].text);
      if (!account) return std::unexpected(account.error());
      m_document.accounts[*account] = (*fields)[2].text;
    } else if (label == "#VER") {
      auto voucher = parse_ver(*fields);
      if (!voucher) return std::unexpected(voucher.error());
      m_voucher = std::move(*voucher);
      m_state = State::ExpectBlockStart;
    }
    return {};
  }

  SieDocument m_document;
  std::optional<Voucher> m_voucher;
  State m_state = State::Records;
  int m_line_number = 0;
};

} // namespace

std::expected<SieDocument, std::string> parse_sie(std::string_view text) { return Parser{}.parse(text); }

std::expected<SieDocument, std::string> read_sie_file(std::filesystem::path const& path) {
  std::ifstream file{path, std::ios::binary};
  if (!file) return std::unexpected("Cannot open " + path.string());
  std::string const bytes{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

  auto document = parse_sie(cp437_to_utf8(bytes));
  if (!document) return std::unexpected(path.filename().string() + ": " + document.error());
  return document;
}

} // namespace cratchit
