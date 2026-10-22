#include "core/pager.hpp"

#include <gtest/gtest.h>

#include <sstream>

using cratchit::Pager;

TEST(Pager, ShortTextLeavesAtOnce) {
  Pager pager(5);
  std::ostringstream out;
  EXPECT_EQ(pager.open({"a", "b"}, out), Pager::Result::Leave);
  EXPECT_EQ(out.str(), "a\nb\n");
}

TEST(Pager, PagesWithSpaceAndStopsWithMinus) {
  Pager pager(2);
  std::ostringstream out;
  EXPECT_EQ(pager.open({"1", "2", "3", "4", "5"}, out), Pager::Result::Stay);
  EXPECT_EQ(pager.prompt(), "--More-- ");
  EXPECT_EQ(pager.on_key(U' ', out), Pager::Result::Stay);
  EXPECT_EQ(out.str(), "1\n2\n3\n4\n");
  EXPECT_EQ(pager.on_key(U'x', out), Pager::Result::Stay);
  EXPECT_EQ(pager.on_key(U'\n', out), Pager::Result::Leave);
  EXPECT_EQ(out.str(), "1\n2\n3\n4\nPress space for more, - or q to stop.\n5\n");
  EXPECT_EQ(pager.open({"1", "2", "3"}, out), Pager::Result::Stay);
  EXPECT_EQ(pager.on_key(U'-', out), Pager::Result::Leave);
}

TEST(Pager, ZeroShowsEverything) {
  Pager pager(0);
  std::ostringstream out;
  EXPECT_EQ(pager.open({"1", "2", "3"}, out), Pager::Result::Leave);
  EXPECT_EQ(out.str(), "1\n2\n3\n");
}
