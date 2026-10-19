#include "cspec/spec_tree.hpp"

#include <gtest/gtest.h>

using cratchit::cspec::SpecTree;

TEST(Cspec, ParsesPathsValuesAndComments) {
  auto const tree = SpecTree::parse("# comment\n\n  box.05.element = ForsMomsEjAnnan \r\nbox.05.title = a # not a comment\n", "t");
  ASSERT_TRUE(tree) << tree.error();
  EXPECT_EQ(tree->value("box.05.element"), "ForsMomsEjAnnan");
  EXPECT_EQ(tree->value("box.05.title"), "a # not a comment");
  EXPECT_EQ(tree->entries().size(), 2u);
  EXPECT_EQ(tree->where("box.05.title"), "t:4");
}

TEST(Cspec, SplitsAtFirstEquals) {
  auto const tree = SpecTree::parse("formula = a = b\n", "t");
  ASSERT_TRUE(tree);
  EXPECT_EQ(tree->value("formula"), "a = b");
}

TEST(Cspec, QuotedValuesKeepBlanksAndUnescape) {
  auto const tree = SpecTree::parse("sep = \" \"\nempty = \"\"\nxml = \"<a b=\\\"1\\\">\"\n", "t");
  ASSERT_TRUE(tree) << tree.error();
  EXPECT_EQ(tree->value("sep"), " ");
  EXPECT_EQ(tree->value("empty"), "");
  EXPECT_EQ(tree->value("xml"), "<a b=\"1\">");
}

TEST(Cspec, RejectsDuplicatesBadPathsAndMissingEquals) {
  EXPECT_FALSE(SpecTree::parse("a = 1\na = 2\n", "t"));
  EXPECT_FALSE(SpecTree::parse("a..b = 1\n", "t"));
  EXPECT_FALSE(SpecTree::parse("a b = 1\n", "t"));
  EXPECT_FALSE(SpecTree::parse("just text\n", "t"));
  EXPECT_FALSE(SpecTree::parse("a = \"bad \\q\"\n", "t"));
}

TEST(Cspec, ChildrenKeepFirstAppearanceOrder) {
  auto const tree = SpecTree::parse("order.10 = x\norder.02 = y\nbox.1.a = 1\nbox.1.b = 2\nbox.3.a = 3\n", "t");
  ASSERT_TRUE(tree);
  EXPECT_EQ(tree->children("order"), (std::vector<std::string>{"10", "02"}));
  EXPECT_EQ(tree->children("box"), (std::vector<std::string>{"1", "3"}));
  EXPECT_EQ(tree->children(""), (std::vector<std::string>{"order", "box"}));
}

TEST(Cspec, PathCanHaveValueAndChildren) {
  auto const tree = SpecTree::parse("rounding = truncate\nrounding.note = why\n", "t");
  ASSERT_TRUE(tree);
  EXPECT_EQ(tree->value("rounding"), "truncate");
  EXPECT_EQ(tree->children("rounding"), (std::vector<std::string>{"note"}));
  EXPECT_TRUE(tree->contains("rounding"));
  EXPECT_FALSE(tree->contains("round"));
}
