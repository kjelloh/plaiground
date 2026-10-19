#include "vat/eskd_writer.hpp"

#include "core/text.hpp"

namespace cratchit::vat {

namespace {

bool is_valid_org_nr(std::string_view org_nr) {
  if (org_nr.size() != 11 || org_nr[6] != '-') return false;
  for (std::size_t i = 0; i < org_nr.size(); ++i) {
    if (i != 6 && (org_nr[i] < '0' || org_nr[i] > '9')) return false;
  }
  return true;
}

std::string escape_xml(std::string_view text) {
  std::string out;
  for (char c : text) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      default: out += c;
    }
  }
  return out;
}

std::string element(std::string_view name, std::string_view value) {
  return "<" + std::string{name} + ">" + std::string{value} + "</" + std::string{name} + ">";
}

} // namespace

std::expected<EskdSpec, std::string> load_eskd_spec(std::filesystem::path const& specs_dir) {
  auto const tree = cspec::SpecTree::load(specs_dir / "vat" / "eskd_6p0.cspec");
  if (!tree) return std::unexpected(tree.error());
  EskdSpec spec;
  spec.m_version_info = spec_version(*tree);
  for (auto [path, target] : {std::pair{"file.xml_declaration", &spec.m_xml_declaration}, std::pair{"file.doctype", &spec.m_doctype},
                              std::pair{"root.element", &spec.m_root_element}, std::pair{"root.attribute.Version", &spec.m_version},
                              std::pair{"org_nr.element", &spec.m_org_nr_element}, std::pair{"moms.element", &spec.m_moms_element},
                              std::pair{"period.element", &spec.m_period_element}, std::pair{"note.element", &spec.m_note_element}}) {
    auto value = tree->require(path);
    if (!value) return std::unexpected(value.error());
    *target = std::move(*value);
  }
  spec.m_include_doctype = tree->value_or("file.include_doctype", "no") == "yes";
  spec.m_body_on_one_line = tree->value_or("file.body_on_one_line", "no") == "yes";
  spec.m_final_newline = tree->value_or("file.final_newline", "no") == "yes";
  spec.m_note_max_length = static_cast<std::size_t>(std::stoul(tree->value_or("note.max_length", "0")));
  for (auto const& key : tree->children("order")) spec.m_order.push_back(tree->value_or("order." + key, ""));
  return spec;
}

std::expected<std::string, std::string> to_eskd(VatReturn const& vat_return, EskdSpec const& eskd, VatSpec const& vat) {
  if (!is_valid_org_nr(vat_return.m_org_nr)) {
    return std::unexpected("invalid org number (expected NNNNNN-NNNN): \"" + vat_return.m_org_nr + "\"");
  }
  if (!vat_return.m_boxes.contains(49)) return std::unexpected("box 49 is required");
  for (auto const& [number, amount] : vat_return.m_boxes) {
    if (amount.ore() % 100 != 0) return std::unexpected("box " + std::to_string(number) + " is not whole kronor");
  }
  auto const note = text::utf8_to_latin1(vat_return.m_note);
  if (!note) return std::unexpected("note: " + note.error());
  if (note->size() > eskd.m_note_max_length) return std::unexpected("note is longer than " + std::to_string(eskd.m_note_max_length) + " characters");

  std::vector<std::string> body{"<" + eskd.m_root_element + " Version=\"" + eskd.m_version + "\">",
                                element(eskd.m_org_nr_element, vat_return.m_org_nr), "<" + eskd.m_moms_element + ">",
                                element(eskd.m_period_element, vat_return.m_period.to_string())};
  std::size_t written = 0;
  for (auto const& item : eskd.m_order) {
    if (item == "note") {
      if (!note->empty()) body.push_back(element(eskd.m_note_element, escape_xml(*note)));
      continue;
    }
    auto const number = std::stoi(item);
    auto const it = vat_return.m_boxes.find(number);
    if (it == vat_return.m_boxes.end()) continue;
    auto const* box = vat.find_box(number);
    if (box == nullptr) return std::unexpected("eskd order lists unknown box " + item);
    body.push_back(element(box->m_element, std::to_string(it->second.whole_kronor())));
    ++written;
  }
  if (written != vat_return.m_boxes.size()) return std::unexpected("some boxes are missing from the eskd element order");
  body.push_back("</" + eskd.m_moms_element + ">");
  body.push_back("</" + eskd.m_root_element + ">");

  std::string out = eskd.m_xml_declaration + "\n";
  if (eskd.m_include_doctype) out += eskd.m_doctype + "\n";
  for (std::size_t i = 0; i < body.size(); ++i) {
    out += body[i];
    if (!eskd.m_body_on_one_line && i + 1 < body.size()) out += "\n";
  }
  if (eskd.m_final_newline) out += "\n";
  return out;
}

} // namespace cratchit::vat
