#include "vat/eskd_reader.hpp"

namespace cratchit::vat {

namespace {

// The text between <name> and </name>, or nothing.
std::optional<std::string> element_text(std::string_view content, std::string_view name) {
  auto const open = "<" + std::string{name} + ">";
  auto const start = content.find(open);
  if (start == std::string_view::npos) return std::nullopt;
  auto const end = content.find("</" + std::string{name} + ">", start);
  if (end == std::string_view::npos) return std::nullopt;
  return std::string{content.substr(start + open.size(), end - start - open.size())};
}

std::map<int, Amount> without_zeros(std::map<int, Amount> boxes) {
  std::erase_if(boxes, [](auto const& box) { return box.second.is_zero() && box.first != 49; });
  return boxes;
}

} // namespace

std::expected<FiledVatReturn, std::string> read_eskd(std::string_view content, VatSpec const& spec) {
  FiledVatReturn filed;
  auto org_nr = element_text(content, "OrgNr");
  auto period = element_text(content, "Period");
  if (!org_nr || !period) return std::unexpected("eskd file has no OrgNr or Period");
  filed.m_org_nr = std::move(*org_nr);
  filed.m_period = std::move(*period);
  for (auto const& box : spec.m_boxes) {
    auto const text = element_text(content, box.m_element);
    if (!text) continue;
    auto const amount = Amount::parse(*text);
    if (!amount) return std::unexpected(box.m_element + ": " + amount.error());
    filed.m_boxes[box.m_number] = *amount;
  }
  return filed;
}

bool same_values(std::map<int, Amount> const& a, std::map<int, Amount> const& b) { return without_zeros(a) == without_zeros(b); }

} // namespace cratchit::vat
