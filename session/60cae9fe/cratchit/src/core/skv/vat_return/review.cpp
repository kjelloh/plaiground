#include "core/skv/vat_return/review.hpp"

#include "core/skv/vat_return/box_texts.hpp"

#include <algorithm>
#include <format>
#include <set>

namespace cratchit::skv::vat_return {

namespace {

std::size_t characters(std::string_view text) {
  return static_cast<std::size_t>(
      std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

// The text of the voucher a posting is in, if the books have it
std::string heading_of(books::Books const& books, VoucherPosting const& posting) {
  auto const* year = books.year_starting(posting.fiscal_year);
  if (!year) {
    return {};
  }
  auto const it = std::ranges::find_if(year->vouchers, [&](books::Voucher const& voucher) {
    return voucher.series == posting.series && voucher.number == posting.number;
  });
  return it == year->vouchers.end() ? std::string{} : it->event.heading;
}

std::string account_name(books::Books const& books, std::string const& account) {
  auto const* known = books.account(account);
  return known ? known->name : std::string{};
}

} // namespace

std::vector<ReviewBox> review_boxes(Document const& document) {
  std::set<std::string> sections;
  for (auto const& box : document.boxes) {
    if (auto const text = text_of(box.box)) {
      sections.insert(text->section);
    }
  }
  std::vector<ReviewBox> boxes;
  for (auto const& text : built_in_box_texts()) {
    if (!sections.contains(text.section) || text.box == vat_to_pay_box ||
        std::ranges::find(boxes, text.box, &ReviewBox::box) != boxes.end()) {
      continue;
    }
    auto const posted = std::ranges::find(document.boxes, text.box, &BoxAmount::box);
    Amount amount = 0;
    std::size_t postings = 0;
    if (posted != document.boxes.end()) {
      amount = posted->amount;
      for (auto const& account : posted->accounts) {
        postings += account.postings.size();
      }
    }
    boxes.push_back({text.box, text_of(text.box, amount)->title, postings});
  }
  return boxes;
}

std::vector<std::string> review_of(books::Books const& books, Document const& document, Box const& box,
                                   std::size_t width) {
  auto const posted = std::ranges::find(document.boxes, box, &BoxAmount::box);
  auto const amount = posted == document.boxes.end() ? Amount{0} : posted->amount;
  auto const text = text_of(box, amount);
  std::vector<std::string> lines;
  lines.push_back(std::format("Box {}  {}", box, text ? text->title : std::string{}));
  if (text) {
    lines.push_back(std::format("Section: {}", text->section));
  }
  std::set<std::string> with_postings;
  if (posted == document.boxes.end()) {
    lines.push_back(std::format("No postings in {} - {}.", format_iso_date(document.period.first),
                                format_iso_date(document.period.last)));
  } else {
    lines.push_back(std::format("On the form: {}", format_amount(posted->amount)));
    lines.push_back("Accounts and vouchers as in the books (debit positive, credit negative):");
    for (auto const& account : posted->accounts) {
      with_postings.insert(account.account);
      lines.push_back(std::format("  {:<6}{:>15}  {}", account.account, format_amount(account.amount),
                                  account_name(books, account.account)));
      for (auto const& posting : account.postings) {
        lines.push_back(std::format("        {:<8}{}{:>15}  {}", std::format("{} {}", posting.series, posting.number),
                                    format_iso_date(posting.date), format_amount(posting.amount),
                                    heading_of(books, posting)));
      }
    }
  }
  std::vector<std::string> others;
  for (auto const& account : accounts_of(box)) {
    if (!with_postings.contains(account)) {
      auto const name = account_name(books, account);
      others.push_back(std::format("  {:<6}{}", account, name.empty() ? "(not in the chart of accounts)" : name));
    }
  }
  if (!others.empty()) {
    lines.push_back(with_postings.empty() ? std::format("Accounts that report in box {}:", box)
                                          : std::format("Other accounts that report in box {}, without postings:", box));
    lines.insert(lines.end(), others.begin(), others.end());
  }
  lines.push_back("");
  lines.push_back(std::format("SKV's description of box {}:", box));
  if (text) {
    for (auto& line : wrap_text(text->description, width)) {
      lines.push_back(std::move(line));
    }
  }
  lines.push_back("");
  return lines;
}

std::string cut_text(std::string_view text, std::size_t max) {
  if (characters(text) <= max) {
    return std::string(text);
  }
  std::size_t count = 0;
  std::size_t end = 0;
  for (; end < text.size(); ++end) {
    if ((static_cast<unsigned char>(text[end]) & 0xC0) != 0x80 && count++ == max - 3) {
      break;
    }
  }
  return std::string(text.substr(0, end)) + "...";
}

std::vector<std::string> wrap_text(std::string_view text, std::size_t width) {
  std::vector<std::string> lines;
  for (std::size_t start = 0; start <= text.size();) {
    auto const end = std::min(text.find('\n', start), text.size());
    auto const paragraph = text.substr(start, end - start);
    std::string line;
    std::size_t length = 0;
    bool words = false;
    for (std::size_t i = 0; i < paragraph.size();) {
      auto const space = std::min(paragraph.find(' ', i), paragraph.size());
      auto const word = paragraph.substr(i, space - i);
      i = space + 1;
      if (word.empty()) {
        continue;
      }
      words = true;
      auto const size = characters(word);
      if (length > 0 && length + 1 + size > width) {
        lines.push_back(std::move(line));
        line.clear();
        length = 0;
      }
      if (length > 0) {
        line += ' ';
        ++length;
      }
      line += word;
      length += size;
    }
    if (words) {
      lines.push_back(std::move(line));
    } else if (end < text.size()) {
      lines.emplace_back(); // an empty line stays
    }
    start = end + 1;
  }
  return lines;
}

} // namespace cratchit::skv::vat_return
