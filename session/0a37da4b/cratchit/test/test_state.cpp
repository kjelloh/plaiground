#include "pyroceph/tui/state.hpp"

#include <gtest/gtest.h>

using pyroceph::tui::Navigator;
using pyroceph::tui::State;

namespace {

State tree() {
  return State{"Rot",
               {
                   {U'1', "Ett", State{"Ett", {{U'1', "Djup", State{"Djup", {}}}}}},
                   {U'2', "Två", State{"Två", {}}},
                   {U'å', "Å", State{"Å", {}}},
                   {U'-', "Nås aldrig", State{"Nås aldrig", {}}},
               }};
}

} // namespace

TEST(Navigator, StartsAtRoot) {
  auto const root = tree();
  Navigator navigator(root);
  EXPECT_EQ(navigator.current().name, "Rot");
  EXPECT_FALSE(navigator.can_go_back());
}

TEST(Navigator, ChoiceMovesToItsState) {
  auto const root = tree();
  Navigator navigator(root);
  EXPECT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name, "Två");
  EXPECT_TRUE(navigator.can_go_back());
}

TEST(Navigator, AnyCodePointCanBeAChoice) {
  auto const root = tree();
  Navigator navigator(root);
  EXPECT_TRUE(navigator.press(U'å'));
  EXPECT_EQ(navigator.current().name, "Å");
}

TEST(Navigator, KeyThatIsNoChoiceChangesNothing) {
  auto const root = tree();
  Navigator navigator(root);
  EXPECT_FALSE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name, "Rot");
}

TEST(Navigator, BackGoesToTheStateBefore) {
  auto const root = tree();
  Navigator navigator(root);
  navigator.press(U'1');
  navigator.press(U'1');
  EXPECT_EQ(navigator.current().name, "Djup");
  EXPECT_TRUE(navigator.press(U'-'));
  EXPECT_EQ(navigator.current().name, "Ett");
  EXPECT_TRUE(navigator.press(U'-'));
  EXPECT_EQ(navigator.current().name, "Rot");
}

TEST(Navigator, BackAtRootChangesNothing) {
  auto const root = tree();
  Navigator navigator(root);
  EXPECT_FALSE(navigator.press(U'-'));
  EXPECT_EQ(navigator.current().name, "Rot");
}

TEST(Navigator, ChoiceWithBackKeyCannotBeReached) {
  auto const root = tree();
  Navigator navigator(root);
  navigator.press(U'-');
  EXPECT_EQ(navigator.current().name, "Rot");
}
