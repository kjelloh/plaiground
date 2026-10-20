#include "core/manual.hpp"

#include <gtest/gtest.h>

#include <functional>

using cratchit::parse_manual;
using cratchit::Section;
using cratchit::to_plain_text;

TEST(ParseManual, HeadingsFormATree) {
  auto root = parse_manual("# Top\n"
                           "Intro\n"
                           "## A\n"
                           "Text A\n"
                           "### A1\n"
                           "Text A1\n"
                           "## B\n");
  EXPECT_EQ(root.title, "Top");
  EXPECT_EQ(root.text, "Intro\n");
  ASSERT_EQ(root.children.size(), 2u);
  EXPECT_EQ(root.children[0].title, "A");
  EXPECT_EQ(root.children[0].text, "Text A\n");
  ASSERT_EQ(root.children[0].children.size(), 1u);
  EXPECT_EQ(root.children[0].children[0].title, "A1");
  EXPECT_EQ(root.children[0].children[0].text, "Text A1\n");
  EXPECT_EQ(root.children[1].title, "B");
  EXPECT_TRUE(root.children[1].text.empty());
}

TEST(ParseManual, SkippedLevelStillNests) {
  auto root = parse_manual("# Top\n## A\n#### Deep\n## B\n");
  ASSERT_EQ(root.children.size(), 2u);
  ASSERT_EQ(root.children[0].children.size(), 1u);
  EXPECT_EQ(root.children[0].children[0].title, "Deep");
}

TEST(ParseManual, SeveralTopHeadingsGetAnUntitledRoot) {
  auto root = parse_manual("# One\n# Two\n");
  EXPECT_TRUE(root.title.empty());
  ASSERT_EQ(root.children.size(), 2u);
}

TEST(ParseManual, HashInCodeBlockIsNotAHeading) {
  auto root = parse_manual("# Top\n```sh\n# a comment\n```\n");
  EXPECT_TRUE(root.children.empty());
  EXPECT_EQ(root.text, "    # a comment\n");
}

TEST(ParseManual, HashWithoutSpaceIsNotAHeading) {
  auto root = parse_manual("# Top\n#hashtag\n");
  EXPECT_TRUE(root.children.empty());
  EXPECT_EQ(root.text, "#hashtag\n");
}

TEST(ParseManual, TitlesArePlainText) {
  auto root = parse_manual("# The `q` key\n");
  EXPECT_EQ(root.title, "The q key");
}

TEST(ToPlainText, RemovesEmphasisCodeAndLinks) {
  EXPECT_EQ(to_plain_text("Press **h** or *q*, see `cratchit` and [the README](README.md).\n"),
            "Press h or q, see cratchit and the README.\n");
}

TEST(ToPlainText, KeepsMarkupInsideCode) {
  EXPECT_EQ(to_plain_text("Run `a *b* c`\n"), "Run a *b* c\n");
}

TEST(ToPlainText, KeepsLoneStars) {
  EXPECT_EQ(to_plain_text("2 * 3 = 6\n"), "2 * 3 = 6\n");
}

TEST(ToPlainText, IndentsCodeBlocksWithoutFences) {
  EXPECT_EQ(to_plain_text("Run:\n\n```sh\n./cratchit\n```\n"), "Run:\n\n    ./cratchit\n");
}

TEST(ToPlainText, LaysOutTablesAsColumns) {
  EXPECT_EQ(to_plain_text("| Key | What |\n"
                          "|-----|------|\n"
                          "| h   | Help |\n"
                          "| Ctrl-D | Quit |\n"),
            "  Key     What\n"
            "  h       Help\n"
            "  Ctrl-D  Quit\n");
}

TEST(ToPlainText, TableColumnsCountSwedishLettersOnce) {
  EXPECT_EQ(to_plain_text("| År | x |\n| ab | y |\n"), "  År  x\n  ab  y\n");
}

TEST(ToPlainText, ListsUseDashes) {
  EXPECT_EQ(to_plain_text("* one\n- two\n"), "- one\n- two\n");
}

TEST(ToPlainText, KeepsOneBlankLineAndTrimsEnds) {
  EXPECT_EQ(to_plain_text("\n\nA\n\n\n\nB\n\n"), "A\n\nB\n");
}

TEST(BuiltInManual, IsTheUserManual) {
  auto root = parse_manual(cratchit::manual_text());
  EXPECT_EQ(root.title, "cratchit user manual");
  EXPECT_FALSE(root.children.empty());
}

TEST(BuiltInManual, HasNoMarkdownLeft) {
  std::function<void(Section const&)> check = [&](Section const& section) {
    EXPECT_EQ(section.text.find("```"), std::string::npos) << section.title;
    EXPECT_EQ(section.text.find("**"), std::string::npos) << section.title;
    EXPECT_EQ(section.text.find("|--"), std::string::npos) << section.title;
    for (auto const& child : section.children) {
      check(child);
    }
  };
  check(parse_manual(cratchit::manual_text()));
}
