#include "core/manual.hpp"

#include "ui/help_topics.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

using cratchit::Manual;
using cratchit::plain_text_of;

TEST(Manual, HelpIsTheParagraphAfterItsMarker) {
  Manual manual("# Rubrik\n\n<!-- hjälp: a -->\nFörsta raden.\nAndra raden.\n\nInte med.\n");
  EXPECT_EQ(manual.help("a"), (std::vector<std::string>{"Första raden.", "Andra raden."}));
}

TEST(Manual, HelpEndsAtAHeadingOrAnotherMarker) {
  Manual manual("<!-- hjälp: a -->\nA.\n## Nästa\n<!-- hjälp: b -->\nB.\n<!-- hjälp: c -->\nC.");
  EXPECT_EQ(manual.help("a"), (std::vector<std::string>{"A."}));
  EXPECT_EQ(manual.help("b"), (std::vector<std::string>{"B."}));
  EXPECT_EQ(manual.help("c"), (std::vector<std::string>{"C."}));
}

TEST(Manual, BlankLinesAfterTheMarkerAreSkipped) {
  Manual manual("<!-- hjälp: a -->\n\nA.\n");
  EXPECT_EQ(manual.help("a"), (std::vector<std::string>{"A."}));
}

TEST(Manual, UnknownTopicHasNoHelp) {
  Manual manual("<!-- hjälp: a -->\nA.\n");
  EXPECT_TRUE(manual.help("b").empty());
}

TEST(Manual, ListsItsTopics) {
  Manual manual("<!-- hjälp: a -->\nA.\n<!--hjälp:b-->\nB.\n<!-- annat -->\n");
  EXPECT_EQ(manual.topics(), (std::vector<std::string>{"a", "b"}));
}

TEST(Manual, PlainTextDropsMarkdown) {
  EXPECT_EQ(plain_text_of("Tryck `-` för **Tillbaka** och *Inte än*."), "Tryck - för Tillbaka och Inte än.");
  EXPECT_EQ(plain_text_of("Se [Skriva en sökväg](#skriva-en-sökväg) nedan."), "Se Skriva en sökväg nedan.");
  EXPECT_EQ(plain_text_of("* Punkt"), "- Punkt");
  EXPECT_EQ(plain_text_of("[ej länk"), "[ej länk");
}

TEST(Manual, MissingFileIsAnError) {
  EXPECT_FALSE(cratchit::read_manual("/no/such/folder/anvandarmanual.md"));
}

// The manual that comes with cratchit has a help paragraph for every state's topic
TEST(Manual, HasHelpForEveryTopic) {
  auto const manual = cratchit::read_manual(CRATCHIT_MANUAL_FILE);
  ASSERT_TRUE(manual) << manual.error();
  for (auto const topic : cratchit::ui::all_help_topics) {
    EXPECT_FALSE(manual->help(topic).empty()) << topic;
  }
}

// ... and no help paragraph that no state uses
TEST(Manual, HasNoHelpForUnknownTopics) {
  auto const manual = cratchit::read_manual(CRATCHIT_MANUAL_FILE);
  ASSERT_TRUE(manual) << manual.error();
  for (auto const& topic : manual->topics()) {
    EXPECT_NE(std::ranges::find(cratchit::ui::all_help_topics, topic), cratchit::ui::all_help_topics.end()) << topic;
  }
}
