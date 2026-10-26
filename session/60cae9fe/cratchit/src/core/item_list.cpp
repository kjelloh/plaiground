#include "core/item_list.hpp"

#include <algorithm>
#include <print>

namespace cratchit {

namespace {

constexpr std::size_t range_size = 8;

} // namespace

ItemList::ItemList(Start start) : start_(start) {}

void ItemList::reset() {
  first_ = 0;
}

void ItemList::keep_within(std::size_t count) {
  while (count > 0 && first_ > 0 && first_ >= count) {
    first_ = previous_first();
  }
}

std::size_t ItemList::first(std::size_t count) const {
  return paged(count) ? first_ : 0;
}

ItemList::Pick ItemList::on_key(charset::unicode::CodePoint key, std::size_t count) {
  if (paged(count) && key == U'0' && first_ > 0) {
    first_ = previous_first();
    return {Pick::Kind::Scrolled};
  }
  if (paged(count) && key == U'9' && first_ + page_size() < count) {
    first_ += page_size();
    return {Pick::Kind::Scrolled};
  }
  auto const first_key_shown = paged(count) ? first_key() : (start_ == Start::Zero ? 0 : 1);
  auto const keys = paged(count) ? page_size() : count;
  if (key >= U'0' + first_key_shown && key <= U'9') {
    auto const offset = static_cast<std::size_t>(key - U'0') - first_key_shown;
    if (offset < keys && first(count) + offset < count) {
      return {Pick::Kind::Item, first(count) + offset};
    }
  }
  return {};
}

void ItemList::show(std::size_t count, std::ostream& out,
                    std::function<void(std::size_t index, std::size_t key)> const& item) const {
  auto const first = this->first(count);
  auto const last = paged(count) ? std::min(first + page_size(), count) : count;
  auto const key = paged(count) ? first_key() : (start_ == Start::Zero ? 0 : 1);
  if (first > 0) {
    std::println(out, "  0  ...up");
  }
  for (auto i = first; i < last; ++i) {
    item(i, i - first + key);
  }
  if (last < count) {
    std::println(out, "  9  ...down");
  }
}

bool ItemList::paged(std::size_t count) const {
  return count > (start_ == Start::Zero ? 10u : 9u);
}

std::size_t ItemList::page_size() const {
  return start_ == Start::Zero && first_ == 0 ? range_size + 1 : range_size;
}

std::size_t ItemList::first_key() const {
  return start_ == Start::Zero && first_ == 0 ? 0 : 1;
}

std::size_t ItemList::previous_first() const {
  if (start_ == Start::Zero) {
    return first_ <= range_size + 1 ? 0 : first_ - range_size;
  }
  return first_ <= range_size ? 0 : first_ - range_size;
}

} // namespace cratchit
