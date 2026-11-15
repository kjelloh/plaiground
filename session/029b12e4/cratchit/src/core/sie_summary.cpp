#include "core/sie_summary.hpp"

#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <format>
#include <fstream>
#include <iterator>
#include <map>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

// Voucher numbers compare as numbers when they are, else as text after them
bool number_less(std::string const& a, std::string const& b) {
  auto as_number = [](std::string const& text) -> std::optional<long long> {
    long long value = 0;
    auto const [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc() || end != text.data() + text.size()) return std::nullopt;
    return value;
  };
  auto const x = as_number(a);
  auto const y = as_number(b);
  if (x && y) return *x < *y;
  if (x != y) return x.has_value(); // numbers before text
  return a < b;
}

} // namespace

std::expected<SieSummary, std::string> summarize_sie_file(fs::path const& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) return std::unexpected(std::string("filen går inte att öppna"));
  std::vector<std::uint8_t> const bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
  auto const lexed = sie::lex_bytes(bytes);
  if (!lexed) return std::unexpected(lexed.error());
  auto const parsed = sie::parse_document(lexed->lines);
  auto const& document = parsed.document;
  if (document.sie_type.empty()) return std::unexpected(std::string("#SIETYP saknas"));
  SieSummary summary;
  if (auto const year = document.year(0)) summary.year = std::pair{year->start, year->end};
  summary.generated = document.generated;
  summary.vouchers = document.vouchers.size();
  std::map<std::string, std::string> highest;
  for (auto const& voucher : document.vouchers) {
    if (!summary.last_date || *summary.last_date < voucher.date) summary.last_date = voucher.date;
    auto [it, added] = highest.try_emplace(voucher.series, voucher.number);
    if (!added && number_less(it->second, voucher.number)) it->second = voucher.number;
  }
  summary.series.assign(highest.begin(), highest.end());
  return summary;
}

std::string summary_line(SieSummary const& summary) {
  std::vector<std::string> parts;
  if (summary.year) parts.push_back(std::format("{} – {}", format_iso_date(summary.year->first), format_iso_date(summary.year->second)));
  if (summary.generated) parts.push_back(std::format("skapad {}", format_iso_date(*summary.generated)));
  if (summary.last_date) {
    parts.push_back(std::format("{} ver. t.o.m. {}", summary.vouchers, format_iso_date(*summary.last_date)));
  } else {
    parts.push_back(std::format("{} ver.", summary.vouchers));
  }
  if (!summary.series.empty()) {
    std::string series;
    for (auto const& [name, number] : summary.series) {
      if (!series.empty()) series += " · ";
      series += std::format("{} {}", name, number);
    }
    parts.push_back(std::move(series));
  }
  std::string line;
  for (auto const& part : parts) {
    if (!line.empty()) line += "  ";
    line += part;
  }
  return line;
}

std::vector<std::string> describe_sie_file(fs::path const& file) {
  auto const summary = summarize_sie_file(file);
  if (!summary) return {std::format("(går inte att läsa: {})", summary.error())};
  return {summary_line(*summary)};
}

} // namespace cratchit
