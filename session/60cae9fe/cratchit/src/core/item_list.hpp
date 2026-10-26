#pragma once

#include "core/charset/unicode.hpp"

#include <cstddef>
#include <functional>
#include <ostream>

namespace cratchit {

// The page of a list whose items are picked by number. Keys 1-9 pick an item; with more items
// than that, keys 1-8 show a page of eight, 0 scrolls up and 9 down, as in help.
// A list that starts at key 0 has keys 0-9 for up to ten items; with more, the first page has
// keys 0-8 and 9 down, the others 0 up, 1-8 and 9 down.
// The list itself stays with its owner, which passes its size on each call.
class ItemList {
public:
  enum class Start { One, Zero };

  struct Pick {
    enum class Kind { None, Scrolled, Item };
    Kind kind = Kind::None;
    std::size_t index = 0; // the item picked, an index into the list (Item only)
  };

  explicit ItemList(Start start = Start::One);

  // Back to the first page
  void reset();

  // After the list shrank: the last page that still has items, if the one shown is gone
  void keep_within(std::size_t count);

  // The first item on the page shown
  std::size_t first(std::size_t count) const;

  // A number picks an item, 0 or 9 scrolls (the page has changed, show it again), else nothing
  Pick on_key(charset::unicode::CodePoint key, std::size_t count);

  // The page: "0  ...up" when there are items above, each item, "9  ...down" when there are items below.
  // item writes its own line(s), given its index into the list and its key.
  void show(std::size_t count, std::ostream& out,
            std::function<void(std::size_t index, std::size_t key)> const& item) const;

private:
  Start start_;
  std::size_t first_ = 0;

  bool paged(std::size_t count) const;
  std::size_t page_size() const;      // the items on the page shown, when paged
  std::size_t first_key() const;      // the key of the page's first item
  std::size_t previous_first() const; // where the page before the one shown starts
};

} // namespace cratchit
