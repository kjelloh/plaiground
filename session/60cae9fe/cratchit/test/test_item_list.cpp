#include "core/item_list.hpp"

#include <gtest/gtest.h>

#include <format>
#include <print>
#include <sstream>
#include <string>

using cratchit::ItemList;
using Kind = ItemList::Pick::Kind;

namespace {

std::string shown(ItemList const& list, std::size_t count) {
  std::ostringstream out;
  list.show(count, out, [&](std::size_t index, std::size_t key) { std::println(out, "  {}  item {}", key, index); });
  return out.str();
}

} // namespace

TEST(ItemList, NineOrFewerAreKeysOneToNine) {
  ItemList list;
  EXPECT_EQ(shown(list, 2), "  1  item 0\n  2  item 1\n");
  auto const pick = list.on_key(U'2', 2);
  EXPECT_EQ(pick.kind, Kind::Item);
  EXPECT_EQ(pick.index, 1u);
  EXPECT_EQ(list.on_key(U'3', 2).kind, Kind::None);
  EXPECT_EQ(list.on_key(U'0', 2).kind, Kind::None);
  EXPECT_EQ(list.on_key(U'9', 9).index, 8u);
  EXPECT_EQ(list.on_key(U'a', 9).kind, Kind::None);
}

TEST(ItemList, MoreThanNineScrollWithZeroAndNine) {
  ItemList list;
  std::string first_page;
  for (auto i = 0; i < 8; ++i) {
    first_page += std::format("  {}  item {}\n", i + 1, i);
  }
  EXPECT_EQ(shown(list, 10), first_page + "  9  ...down\n");
  EXPECT_EQ(list.on_key(U'0', 10).kind, Kind::None);
  EXPECT_EQ(list.on_key(U'9', 10).kind, Kind::Scrolled);
  EXPECT_EQ(shown(list, 10), "  0  ...up\n  1  item 8\n  2  item 9\n");
  EXPECT_EQ(list.on_key(U'9', 10).kind, Kind::None);
  EXPECT_EQ(list.on_key(U'3', 10).kind, Kind::None);
  EXPECT_EQ(list.on_key(U'2', 10).index, 9u);
  EXPECT_EQ(list.on_key(U'0', 10).kind, Kind::Scrolled);
  EXPECT_EQ(list.on_key(U'8', 10).index, 7u);
}

TEST(ItemList, KeepWithinAndReset) {
  ItemList list;
  list.on_key(U'9', 20);
  list.on_key(U'9', 20);
  EXPECT_EQ(list.on_key(U'1', 20).index, 16u);
  list.keep_within(12);
  EXPECT_EQ(list.on_key(U'1', 12).index, 8u);
  list.keep_within(0);
  EXPECT_EQ(list.on_key(U'1', 12).index, 8u);
  list.reset();
  EXPECT_EQ(list.on_key(U'1', 12).index, 0u);
}

TEST(ItemList, StartingAtZeroHasTenKeys) {
  ItemList list(ItemList::Start::Zero);
  EXPECT_EQ(shown(list, 2), "  0  item 0\n  1  item 1\n");
  EXPECT_EQ(list.on_key(U'0', 10).index, 0u);
  EXPECT_EQ(list.on_key(U'9', 10).index, 9u);
  EXPECT_EQ(list.on_key(U'2', 2).kind, Kind::None);
}

TEST(ItemList, StartingAtZeroPagesWithNineDownAndZeroUp) {
  ItemList list(ItemList::Start::Zero);
  std::string first_page;
  for (auto i = 0; i < 9; ++i) {
    first_page += std::format("  {}  item {}\n", i, i);
  }
  EXPECT_EQ(shown(list, 20), first_page + "  9  ...down\n");
  EXPECT_EQ(list.on_key(U'8', 20).index, 8u);
  EXPECT_EQ(list.on_key(U'9', 20).kind, Kind::Scrolled);
  EXPECT_EQ(list.first(20), 9u);
  EXPECT_EQ(list.on_key(U'1', 20).index, 9u);
  EXPECT_EQ(list.on_key(U'8', 20).index, 16u);
  EXPECT_EQ(list.on_key(U'9', 20).kind, Kind::Scrolled);
  EXPECT_EQ(shown(list, 20), "  0  ...up\n  1  item 17\n  2  item 18\n  3  item 19\n");
  EXPECT_EQ(list.on_key(U'4', 20).kind, Kind::None);
  EXPECT_EQ(list.on_key(U'0', 20).kind, Kind::Scrolled);
  EXPECT_EQ(list.on_key(U'0', 20).kind, Kind::Scrolled);
  EXPECT_EQ(list.first(20), 0u);
  EXPECT_EQ(list.on_key(U'0', 20).index, 0u);
}
