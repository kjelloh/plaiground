#include "vat/vat_spec.hpp"

namespace cratchit::vat {

namespace {

std::expected<int, std::string> box_number(cspec::SpecTree const& tree, std::string const& key) {
  if (key.size() != 2 || key[0] < '0' || key[0] > '9' || key[1] < '0' || key[1] > '9') {
    return std::unexpected(tree.source() + ": box key \"" + key + "\" is not two digits");
  }
  return std::stoi(key);
}

std::expected<AccountList, std::string> account_list(cspec::SpecTree const& tree, std::string const& path) {
  auto const text = tree.require(path);
  if (!text) return std::unexpected(text.error());
  auto list = AccountList::parse(*text);
  if (!list) return std::unexpected(tree.where(path) + ": " + list.error());
  return list;
}

} // namespace

BoxDef const* VatSpec::find_box(int number) const {
  for (auto const& box : m_boxes) {
    if (box.m_number == number) return &box;
  }
  return nullptr;
}

SpecVersion spec_version(cspec::SpecTree const& tree) {
  return {tree.value_or("spec.id", "?"), tree.value_or("spec.version", "?"), tree.value_or("spec.status", "?")};
}

std::expected<VatSpec, std::string> load_vat_spec(std::filesystem::path const& specs_dir) {
  auto const boxes = cspec::SpecTree::load(specs_dir / "vat" / "vat_boxes.cspec");
  if (!boxes) return std::unexpected(boxes.error());
  auto const mapping = cspec::SpecTree::load(specs_dir / "vat" / "bas_to_vat_box.cspec");
  if (!mapping) return std::unexpected(mapping.error());
  auto const rules = cspec::SpecTree::load(specs_dir / "vat" / "vat_rules.cspec");
  if (!rules) return std::unexpected(rules.error());

  VatSpec spec;
  spec.m_used_specs = {spec_version(*boxes), spec_version(*mapping), spec_version(*rules)};

  for (auto const& key : boxes->children("box")) {
    auto const number = box_number(*boxes, key);
    if (!number) return std::unexpected(number.error());
    auto const base = "box." + key + ".";
    auto const element = boxes->require(base + "element");
    if (!element) return std::unexpected(element.error());
    auto const kind = boxes->require(base + "kind");
    if (!kind) return std::unexpected(kind.error());
    if (*kind != "base" && *kind != "output_vat" && *kind != "input_vat" && *kind != "result") {
      return std::unexpected(boxes->where(base + "kind") + ": unknown kind \"" + *kind + "\"");
    }
    spec.m_boxes.push_back({*number, *element, boxes->value_or(base + "title", ""), *kind});
  }

  for (auto const& key : mapping->children("box")) {
    auto const number = box_number(*mapping, key);
    if (!number) return std::unexpected(number.error());
    if (spec.find_box(*number) == nullptr) return std::unexpected(mapping->source() + ": box " + key + " is not in vat_boxes.cspec");
    auto const base = "box." + key + ".";
    auto accounts = account_list(*mapping, base + "accounts");
    if (!accounts) return std::unexpected(accounts.error());
    auto const sign = mapping->value_or(base + "sign", "");
    if (sign != "credit" && sign != "debit") return std::unexpected(mapping->where(base + "sign") + ": sign must be credit or debit");
    spec.m_mappings.push_back({*number, std::move(*accounts), sign == "credit"});
  }
  auto vat_accounts = account_list(*mapping, "vat_accounts");
  if (!vat_accounts) return std::unexpected(vat_accounts.error());
  spec.m_vat_accounts = std::move(*vat_accounts);
  auto unmapped = account_list(*mapping, "unmapped_allowed");
  if (!unmapped) return std::unexpected(unmapped.error());
  spec.m_unmapped_allowed = std::move(*unmapped);

  auto const rounding = rules->value_or("rounding.box", "");
  if (rounding != "truncate") return std::unexpected(rules->where("rounding.box") + ": only 'truncate' is implemented");
  spec.m_truncate = true;
  spec.m_exclude_settlement = rules->value_or("settlement.exclude", "no") == "yes";
  if (rules->value_or("period.selection", "") != "voucher_date") {
    return std::unexpected(rules->where("period.selection") + ": only 'voucher_date' is implemented");
  }
  return spec;
}

} // namespace cratchit::vat
