#include "core/repl.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

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

} // namespace

TEST(OnKey, HPrintsHelp) {
  std::ostringstream out;
  EXPECT_EQ(cratchit::on_key('h', out), cratchit::Step::Continue);
  EXPECT_NE(out.str().find("h  show this help"), std::string::npos);
  EXPECT_NE(out.str().find("q  quit"), std::string::npos);
}

TEST(OnKey, QQuits) {
  std::ostringstream out;
  EXPECT_EQ(cratchit::on_key('q', out), cratchit::Step::Quit);
}

TEST(OnKey, CtrlDQuits) {
  std::ostringstream out;
  EXPECT_EQ(cratchit::on_key('\x04', out), cratchit::Step::Quit);
}

TEST(OnKey, UnknownKeyPointsToHelp) {
  std::ostringstream out;
  EXPECT_EQ(cratchit::on_key('x', out), cratchit::Step::Continue);
  EXPECT_NE(out.str().find("Press h for help"), std::string::npos);
}

TEST(RunRepl, StopsAtQAndIgnoresLaterKeys) {
  std::ostringstream out;
  cratchit::run_repl(keys_from("hqh"), out);
  EXPECT_EQ(out.str(), "cratchit> h\n"
                       "Keys:\n"
                       "  h  show this help\n"
                       "  q  quit\n"
                       "cratchit> q\n");
}

TEST(RunRepl, StopsAtEndOfInput) {
  std::ostringstream out;
  cratchit::run_repl(keys_from(""), out);
  EXPECT_EQ(out.str(), "cratchit> \n");
}

TEST(RunRepl, IgnoresNewlines) {
  std::ostringstream out;
  cratchit::run_repl(keys_from("\nq\n"), out);
  EXPECT_EQ(out.str(), "cratchit> q\n");
}
