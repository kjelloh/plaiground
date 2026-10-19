#include "core/financial_event.hpp"

namespace cratchit {

namespace {

std::string event_key(std::size_t index) {
  auto number = std::to_string(index + 1);
  while (number.size() < 4) number.insert(0, "0");
  return "event." + number;
}

} // namespace

std::string to_cspec(std::vector<FinancialEvent> const& events) {
  std::string out = "# Financial event listing, schema: specs/events/financial_event.cspec\n";
  for (std::size_t i = 0; i < events.size(); ++i) {
    auto const& event = events[i];
    auto const key = event_key(i);
    out += "\n";
    out += key + ".date = " + to_yyyymmdd(event.m_date) + "\n";
    out += key + ".amount = " + to_string(event.m_amount) + "\n";
    out += key + ".heading = " + cspec::quote(event.m_heading) + "\n";
    if (event.m_account) out += key + ".account = " + std::to_string(*event.m_account) + "\n";
    out += key + ".source.interface = " + event.m_source_interface + "\n";
    if (!event.m_source_file.empty()) out += key + ".source.file = " + cspec::quote(event.m_source_file) + "\n";
    if (event.m_source_row) out += key + ".source.row = " + std::to_string(*event.m_source_row) + "\n";
  }
  return out;
}

std::expected<std::vector<FinancialEvent>, std::string> events_from_cspec(cspec::SpecTree const& tree) {
  std::vector<FinancialEvent> events;
  for (auto const& id : tree.children("event")) {
    auto const base = "event." + id + ".";
    auto const date_text = tree.require(base + "date");
    if (!date_text) return std::unexpected(date_text.error());
    auto const date = parse_date(*date_text);
    if (!date) return std::unexpected(tree.where(base + "date") + ": " + date.error());
    auto const amount_text = tree.require(base + "amount");
    if (!amount_text) return std::unexpected(amount_text.error());
    auto const amount = Amount::parse(*amount_text);
    if (!amount) return std::unexpected(tree.where(base + "amount") + ": " + amount.error());
    auto const heading = tree.require(base + "heading");
    if (!heading) return std::unexpected(heading.error());
    auto const interface = tree.require(base + "source.interface");
    if (!interface) return std::unexpected(interface.error());

    FinancialEvent event{*date, *amount, *heading, std::nullopt, *interface, tree.value_or(base + "source.file", ""), std::nullopt};
    if (auto const account = tree.value(base + "account")) {
      auto const parsed = parse_account_no(*account);
      if (!parsed) return std::unexpected(tree.where(base + "account") + ": " + parsed.error());
      event.m_account = *parsed;
    }
    if (auto const row = tree.value(base + "source.row")) event.m_source_row = std::stoi(*row);
    events.push_back(std::move(event));
  }
  return events;
}

} // namespace cratchit
