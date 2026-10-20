#include "core/repl.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

using cratchit::Repl;
using cratchit::Section;
using cratchit::Step;

namespace {

// Feeds the keys of a string one at a time, then reports end of input
cratchit::KeySource keys_from(std::string keys) {
  return [keys = std::move(keys), pos = std::size_t{0}]() mutable -> std::optional<char> {
    if (pos == keys.size()) {
      return std::nullopt;
    }
    return keys[pos++];
  };
}

Section const manual{"Manual", "", {{"A", "Text A\n", {}}}};

} // namespace

TEST(Repl, HOpensHelp) {
  Repl repl(manual, 0);
  std::ostringstream out;
  EXPECT_EQ(repl.on_key('h', out), Step::Continue);
  EXPECT_NE(out.str().find("  1  A"), std::string::npos);
  EXPECT_EQ(repl.prompt(), "help> ");
}

TEST(Repl, QQuits) {
  Repl repl(manual, 0);
  std::ostringstream out;
  EXPECT_EQ(repl.on_key('q', out), Step::Quit);
}

TEST(Repl, CtrlDQuits) {
  Repl repl(manual, 0);
  std::ostringstream out;
  EXPECT_EQ(repl.on_key('\x04', out), Step::Quit);
}

TEST(Repl, CtrlDQuitsFromHelp) {
  Repl repl(manual, 0);
  std::ostringstream out;
  repl.on_key('h', out);
  EXPECT_EQ(repl.on_key('\x04', out), Step::Quit);
}

TEST(Repl, QInHelpLeavesHelpOnly) {
  Repl repl(manual, 0);
  std::ostringstream out;
  repl.on_key('h', out);
  EXPECT_EQ(repl.on_key('q', out), Step::Continue);
  EXPECT_EQ(repl.prompt(), "cratchit> ");
}

TEST(Repl, UnknownKeyPointsToHelp) {
  Repl repl(manual, 0);
  std::ostringstream out;
  EXPECT_EQ(repl.on_key('x', out), Step::Continue);
  EXPECT_NE(out.str().find("Press h for help"), std::string::npos);
}

TEST(Repl, EscapeDoesNothing) {
  Repl repl(manual, 0);
  EXPECT_TRUE(repl.ignores('\x1b'));
  std::ostringstream out;
  repl.on_key('h', out);
  EXPECT_TRUE(repl.ignores('\x1b'));
}

TEST(Repl, SpaceTurnsThePageOnlyWhilePaging) {
  Section long_manual{"Manual", "1\n2\n3\n4\n", {}};
  Repl repl(long_manual, 2);
  EXPECT_TRUE(repl.ignores(' '));
  std::ostringstream out;
  repl.on_key('h', out);
  EXPECT_FALSE(repl.ignores(' '));
  EXPECT_FALSE(repl.ignores('\n'));
}

TEST(RunRepl, BrowsesHelpAndQuits) {
  std::ostringstream out;
  cratchit::run_repl(keys_from("h1--q"), out, manual, 0);
  EXPECT_EQ(out.str(), "cratchit> h\n"
                       "Manual\n"
                       "\n"
                       "  1  A\n"
                       "\n"
                       "Press a number to open a section, - to go back, q to leave help.\n"
                       "help> 1\n"
                       "A\n"
                       "\n"
                       "Text A\n"
                       "\n"
                       "Press - to go back, q to leave help.\n"
                       "help 1> -\n"
                       "Manual\n"
                       "\n"
                       "  1  A\n"
                       "\n"
                       "Press a number to open a section, - to go back, q to leave help.\n"
                       "help> -\n"
                       "cratchit> q\n");
}

TEST(RunRepl, StopsAtEndOfInput) {
  std::ostringstream out;
  cratchit::run_repl(keys_from(""), out, manual, 0);
  EXPECT_EQ(out.str(), "cratchit> \n");
}

TEST(RunRepl, IgnoresNewlines) {
  std::ostringstream out;
  cratchit::run_repl(keys_from("\nq\n"), out, manual, 0);
  EXPECT_EQ(out.str(), "cratchit> q\n");
}
