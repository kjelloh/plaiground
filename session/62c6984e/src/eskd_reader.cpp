#include "eskd_reader.hpp"

#include "latin1.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <optional>

namespace cratchit {

namespace {

struct Element {
  std::string_view m_name;
  std::string_view m_value;
};

// Reads the next <Name>value</Name> at or after pos. Returns nullopt at a closing tag or end of text.
std::expected<std::optional<Element>, std::string> next_element(std::string_view text, std::size_t& pos) {
  auto const open = text.find('<', pos);
  if (open == std::string_view::npos || text.substr(open, 2) == "</") return std::nullopt;
  auto const open_end = text.find('>', open);
  if (open_end == std::string_view::npos) return std::unexpected("Unterminated tag");

  auto const name = text.substr(open + 1, open_end - open - 1);
  auto const closing = "</" + std::string{name} + ">";
  auto const close = text.find(closing, open_end);
  if (close == std::string_view::npos) return std::unexpected("Missing " + closing);

  pos = close + closing.size();
  return Element{name, text.substr(open_end + 1, close - open_end - 1)};
}

std::string unescape_xml(std::string_view text) {
  constexpr std::pair<std::string_view, char> entities[]{
      {"&amp;", '&'}, {"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'}, {"&apos;", '\''}};

  std::string out;
  while (!text.empty()) {
    auto const entity = std::ranges::find_if(entities, [&](auto const& e) { return text.starts_with(e.first); });
    if (entity != std::ranges::end(entities)) {
      out += entity->second;
      text.remove_prefix(entity->first.size());
    } else {
      out += text.front();
      text.remove_prefix(1);
    }
  }
  return out;
}

std::expected<Amount, std::string> parse_kronor(std::string_view text) {
  auto const amount = parse_amount(text);
  if (!amount || text.find('.') != std::string_view::npos) {
    return std::unexpected("Invalid whole kronor amount: \"" + std::string{text} + "\"");
  }
  return *amount;
}

} // namespace

std::expected<VatReturn, std::string> parse_eskd(std::string_view text) {
  auto const root_tag = std::string_view{"<eSKDUpload Version=\"6.0\">"};
  auto const root = text.find(root_tag);
  if (root == std::string_view::npos) return std::unexpected("Not an eSKDUpload 6.0 file");
  std::size_t pos = root + root_tag.size();

  VatReturn result;
  auto const org_nr = next_element(text, pos);
  if (!org_nr) return std::unexpected(org_nr.error());
  if (!*org_nr || (*org_nr)->m_name != "OrgNr") return std::unexpected("Expected <OrgNr>");
  result.m_org_nr = std::string{(*org_nr)->m_value};

  auto const moms = text.find("<Moms>", pos);
  if (moms == std::string_view::npos) return std::unexpected("Expected <Moms>");
  pos = moms + std::string_view{"<Moms>"}.size();

  bool has_period = false;
  while (true) {
    auto const element = next_element(text, pos);
    if (!element) return std::unexpected(element.error());
    if (!*element) break;
    auto const [name, value] = **element;

    if (name == "Period") {
      auto const period = parse_vat_period(value);
      if (!period) return std::unexpected(period.error());
      result.m_period = *period;
      has_period = true;
    } else if (name == "TextUpplysningMoms") {
      result.m_note = latin1_to_utf8(unescape_xml(value));
    } else if (auto const box = find_vat_box(name)) {
      auto const amount = parse_kronor(value);
      if (!amount) return std::unexpected(amount.error());
      result.m_boxes[box->m_number] = *amount;
    } else {
      return std::unexpected("Unknown element <" + std::string{name} + ">");
    }
  }
  if (!has_period) return std::unexpected("Expected <Period>");
  return result;
}

std::expected<VatReturn, std::string> read_eskd_file(std::filesystem::path const& path) {
  std::ifstream file{path, std::ios::binary};
  if (!file) return std::unexpected("Cannot open " + path.string());
  std::string const bytes{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

  auto result = parse_eskd(bytes);
  if (!result) return std::unexpected(path.filename().string() + ": " + result.error());
  return result;
}

} // namespace cratchit
