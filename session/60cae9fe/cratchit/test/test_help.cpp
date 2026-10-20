#include "core/help.hpp"

#include <gtest/gtest.h>

#include <format>
#include <sstream>
#include <string>

using cratchit::HelpBrowser;
using cratchit::Section;

namespace {

Section small_manual() {
  return {"Manual",
          "Intro\n",
          {{"A", "Text A\n", {{"A1", "Text A1\n", {}}}},
           {"B", "Text B\n", {}}}};
}

// A manual with the given number of empty sections named S1, S2, ...
Section manual_with(std::size_t sections) {
  Section root{"Manual", "", {}};
  for (std::size_t i = 1; i <= sections; ++i) {
    root.children.push_back({std::format("S{}", i), "", {}});
  }
  return root;
}

bool contains(std::string const& text, std::string const& part) {
  return text.find(part) != std::string::npos;
}

} // namespace

TEST(HelpBrowser, OpenShowsTitleTextAndNumberedSections) {
  auto manual = small_manual();
  HelpBrowser help(manual, 0);
  std::ostringstream out;
  help.open(out);
  EXPECT_EQ(out.str(), "Manual\n"
                       "\n"
                       "Intro\n"
                       "\n"
                       "  1  A\n"
                       "  2  B\n"
                       "\n"
                       "Press a number to open a section, - to go back, q to leave help.\n");
  EXPECT_EQ(help.prompt(), "help> ");
}

TEST(HelpBrowser, NumbersDrillDownToText) {
  auto manual = small_manual();
  HelpBrowser help(manual, 0);
  std::ostringstream out;
  help.open(out);
  help.on_key('1', out);
  EXPECT_EQ(help.prompt(), "help 1> ");
  out.str("");
  help.on_key('1', out);
  EXPECT_EQ(help.prompt(), "help 1.1> ");
  EXPECT_EQ(out.str(), "A1\n"
                       "\n"
                       "Text A1\n"
                       "\n"
                       "Press - to go back, q to leave help.\n");
}

TEST(HelpBrowser, MinusGoesBackThenLeaves) {
  auto manual = small_manual();
  HelpBrowser help(manual, 0);
  std::ostringstream out;
  help.open(out);
  help.on_key('2', out);
  out.str("");
  EXPECT_EQ(help.on_key('-', out), HelpBrowser::Result::Stay);
  EXPECT_EQ(help.prompt(), "help> ");
  // Going back shows the list again, not the text
  EXPECT_FALSE(contains(out.str(), "Intro"));
  EXPECT_TRUE(contains(out.str(), "  2  B"));
  EXPECT_EQ(help.on_key('-', out), HelpBrowser::Result::Leave);
}

TEST(HelpBrowser, QLeavesFromAnyDepth) {
  auto manual = small_manual();
  HelpBrowser help(manual, 0);
  std::ostringstream out;
  help.open(out);
  help.on_key('1', out);
  help.on_key('1', out);
  EXPECT_EQ(help.on_key('q', out), HelpBrowser::Result::Leave);
}

TEST(HelpBrowser, MissingNumberSaysSo) {
  auto manual = small_manual();
  HelpBrowser help(manual, 0);
  std::ostringstream out;
  help.open(out);
  out.str("");
  EXPECT_EQ(help.on_key('3', out), HelpBrowser::Result::Stay);
  EXPECT_TRUE(contains(out.str(), "No section 3 here."));
  EXPECT_EQ(help.prompt(), "help> ");
}

TEST(HelpBrowser, NineSectionsNeedNoScrolling) {
  auto manual = manual_with(9);
  HelpBrowser help(manual, 0);
  std::ostringstream out;
  help.open(out);
  EXPECT_TRUE(contains(out.str(), "  9  S9\n"));
  EXPECT_FALSE(contains(out.str(), "...down"));
  help.on_key('9', out);
  EXPECT_EQ(help.prompt(), "help 9> ");
}

TEST(HelpBrowser, TenSectionsScrollWithNineAndZero) {
  auto manual = manual_with(20);
  HelpBrowser help(manual, 0);
  std::ostringstream out;
  help.open(out);
  EXPECT_TRUE(contains(out.str(), "  8  S8\n  9  ...down\n"));
  EXPECT_FALSE(contains(out.str(), "...up"));

  out.str("");
  help.on_key('9', out);
  EXPECT_TRUE(contains(out.str(), "  0  ...up\n  1  S9\n"));
  EXPECT_TRUE(contains(out.str(), "  8  S16\n  9  ...down\n"));

  out.str("");
  help.on_key('9', out);
  EXPECT_TRUE(contains(out.str(), "  0  ...up\n  1  S17\n"));
  EXPECT_TRUE(contains(out.str(), "  4  S20\n\n"));
  EXPECT_FALSE(contains(out.str(), "...down"));
  out.str("");
  EXPECT_EQ(help.on_key('9', out), HelpBrowser::Result::Stay);
  EXPECT_TRUE(contains(out.str(), "No section 9 here."));

  out.str("");
  help.on_key('0', out);
  EXPECT_TRUE(contains(out.str(), "  1  S9\n"));

  help.on_key('2', out);
  EXPECT_EQ(help.prompt(), "help 10> ");
}

TEST(HelpBrowser, BackKeepsTheScrolledRange) {
  auto manual = manual_with(20);
  HelpBrowser help(manual, 0);
  std::ostringstream out;
  help.open(out);
  help.on_key('9', out);
  help.on_key('1', out);
  out.str("");
  help.on_key('-', out);
  EXPECT_TRUE(contains(out.str(), "  1  S9\n"));
}

TEST(HelpBrowser, LongTextIsPaged) {
  Section manual{"Manual", "1\n2\n3\n4\n5\n", {}};
  HelpBrowser help(manual, 3);
  std::ostringstream out;
  help.open(out);
  EXPECT_EQ(out.str(), "Manual\n\n1\n");
  EXPECT_TRUE(help.is_paging());
  EXPECT_EQ(help.prompt(), "--More-- ");

  out.str("");
  help.on_key(' ', out);
  EXPECT_EQ(out.str(), "2\n3\n4\n");

  out.str("");
  help.on_key('\n', out);
  EXPECT_EQ(out.str(), "5\n\nPress - to go back, q to leave help.\n");
  EXPECT_FALSE(help.is_paging());
  EXPECT_EQ(help.prompt(), "help> ");
}

TEST(HelpBrowser, OtherKeysWhilePagingExplainThePage) {
  Section manual{"Manual", "1\n2\n3\n4\n5\n", {}};
  HelpBrowser help(manual, 3);
  std::ostringstream out;
  help.open(out);
  out.str("");
  help.on_key('1', out);
  EXPECT_EQ(out.str(), "Press space for more, - to go back, q to leave help.\n");
  EXPECT_TRUE(help.is_paging());
}

TEST(HelpBrowser, MinusWhilePagingGoesBack) {
  Section manual{"Manual", "", {{"A", "1\n2\n3\n4\n5\n", {}}}};
  // Section A needs two pages; the list it goes back to fits on one
  HelpBrowser help(manual, 5);
  std::ostringstream out;
  help.open(out);
  help.on_key('1', out);
  EXPECT_TRUE(help.is_paging());
  EXPECT_EQ(help.on_key('-', out), HelpBrowser::Result::Stay);
  EXPECT_FALSE(help.is_paging());
  EXPECT_EQ(help.prompt(), "help> ");
}
