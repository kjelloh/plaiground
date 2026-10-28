#include "core/skv/vat_return/eskd.hpp"

#include "core/log.hpp"
#include "core/skv/vat_return/box_tags.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <format>
#include <set>
#include <utility>

namespace cratchit::skv::vat_return {

std::expected<std::vector<BoxTag>, std::string> box_tags(std::string_view text) {
  if (text.starts_with("\xEF\xBB\xBF")) {
    text.remove_prefix(3);
  }
  std::vector<BoxTag> tags;
  std::set<std::string_view> seen_tags;
  std::size_t number = 0;
  while (!text.empty()) {
    ++number;
    auto const end = text.find('\n');
    auto line = text.substr(0, end);
    text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
    if (line.ends_with('\r')) {
      line.remove_suffix(1);
    }
    if (line.empty() || (number == 1 && line == "box;tag")) {
      continue;
    }
    auto const separator = line.find(';');
    auto const box = line.substr(0, separator);
    auto const tag = separator == std::string_view::npos ? std::string_view{} : line.substr(separator + 1);
    if (box.size() != 2 || !std::ranges::all_of(box, [](char c) { return c >= '0' && c <= '9'; })) {
      return std::unexpected(std::format("Line {}: the box is not two digits: {}", number, line));
    }
    if (tag.empty() || !std::ranges::all_of(tag, [](char c) {
          return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        })) {
      return std::unexpected(std::format("Line {}: the tag is not ASCII letters and digits: {}", number, line));
    }
    if (std::ranges::find(tags, box, &BoxTag::box) != tags.end()) {
      return std::unexpected(std::format("Line {}: box {} is listed before", number, box));
    }
    if (!seen_tags.insert(tag).second) {
      return std::unexpected(std::format("Line {}: tag {} is listed before", number, tag));
    }
    tags.push_back({Box(box), std::string(tag)});
  }
  return tags;
}

namespace eskd {

namespace {

std::vector<BoxTag> const& built_in_tags() {
  static auto const tags = [] {
    auto parsed = box_tags(box_tags_text());
    if (!parsed) {
      LOG_DESIGN_INSUFFICIENCY("Built-in eSKD tags of the VAT return boxes not read: {}", parsed.error());
      return std::vector<BoxTag>{};
    }
    return *std::move(parsed);
  }();
  return tags;
}

} // namespace

Document document_of(vat_return::Document const& form, std::string_view orgnr) {
  auto const& tags = built_in_tags();
  Document document{std::string(orgnr),
                    std::format("{:04}{:02}", static_cast<int>(form.period.last.year()),
                                static_cast<unsigned>(form.period.last.month())),
                    {}};
  for (auto const& box : form.boxes) {
    if (std::ranges::find(tags, box.box, &BoxTag::box) == tags.end()) {
      LOG_DESIGN_INSUFFICIENCY("VAT return box {} has no eSKD tag, so it is left out of the file", box.box);
    }
  }
  for (auto const& [box, tag] : tags) {
    if (box == vat_to_pay_box) {
      document.fields.push_back({tag, vat_to_pay(form)});
    } else if (auto const found = std::ranges::find(form.boxes, box, &BoxAmount::box); found != form.boxes.end()) {
      document.fields.push_back({tag, found->amount});
    }
  }
  return document;
}

} // namespace eskd

} // namespace cratchit::skv::vat_return
