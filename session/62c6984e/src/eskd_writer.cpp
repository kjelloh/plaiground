#include "eskd_writer.hpp"

#include "latin1.hpp"

#include <fstream>

namespace cratchit {

namespace {

constexpr std::size_t max_note_length = 300;

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

std::expected<std::string, std::string> to_eskd(VatReturn const& vat_return) {
  if (!is_valid_org_nr(vat_return.m_org_nr)) {
    return std::unexpected("Invalid org number (expected NNNNNN-NNNN): \"" + vat_return.m_org_nr + "\"");
  }
  if (!vat_return.m_period.ok()) return std::unexpected("Invalid VAT period");
  if (!vat_return.m_boxes.contains(49)) return std::unexpected("Box 49 (MomsBetala) is required");
  for (auto const& [number, amount] : vat_return.m_boxes) {
    if (find_vat_box(number) == nullptr) return std::unexpected("Unknown VAT box " + std::to_string(number));
    if (amount.cents() % 100 != 0) return std::unexpected("Box " + std::to_string(number) + " is not whole kronor");
  }

  auto const note = utf8_to_latin1(vat_return.m_note);
  if (!note) return std::unexpected(note.error());
  if (note->size() > max_note_length) return std::unexpected("Note is longer than 300 characters");

  std::string body = "<eSKDUpload Version=\"6.0\">" + element("OrgNr", vat_return.m_org_nr) + "<Moms>" +
                     element("Period", to_string(vat_return.m_period));
  for (auto const& box : vat_boxes()) {
    if (auto const it = vat_return.m_boxes.find(box.m_number); it != vat_return.m_boxes.end()) {
      body += element(box.m_element, std::to_string(it->second.cents() / 100));
    }
  }
  if (!note->empty()) body += element("TextUpplysningMoms", escape_xml(*note));
  body += "</Moms></eSKDUpload>";

  return "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
         "<!DOCTYPE eSKDUpload PUBLIC \"-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV\" "
         "\"https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd\">\n" +
         body + "\n";
}

std::expected<void, std::string> write_eskd_file(VatReturn const& vat_return, std::filesystem::path const& path) {
  auto const content = to_eskd(vat_return);
  if (!content) return std::unexpected(content.error());

  std::ofstream file{path, std::ios::binary};
  if (!file) return std::unexpected("Cannot create " + path.string());
  file << *content;
  if (!file) return std::unexpected("Failed writing " + path.string());
  return {};
}

} // namespace cratchit
