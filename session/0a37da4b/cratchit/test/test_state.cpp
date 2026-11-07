#include "menu_state.hpp"

#include <gtest/gtest.h>

using pyroceph::tui::Navigator;
using test::menu;

namespace {

std::unique_ptr<pyroceph::tui::State> tree(int* alive = nullptr) {
  return menu("Rot",
              {
                  {U'1', "Ett", [=] { return menu("Ett", {{U'1', "Djup", [=] { return menu("Djup", {}, {}, alive); }}}, {}, alive); }},
                  {U'2', "Två", [=] { return menu("Två", {}, {}, alive); }},
                  {U'å', "Å", [=] { return menu("Å", {}, {}, alive); }},
                  {U'-', "Minus", [=] { return menu("Minus", {}, {}, alive); }},
              },
              {}, alive);
}

} // namespace

TEST(Navigator, StartsAtRoot) {
  Navigator navigator(tree());
  EXPECT_EQ(navigator.current().name(), "Rot");
  EXPECT_FALSE(navigator.can_go_back());
}

TEST(Navigator, KeyTheStateMapsMovesToTheNewState) {
  Navigator navigator(tree());
  EXPECT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Två");
  EXPECT_TRUE(navigator.can_go_back());
}

TEST(Navigator, AnyCodePointCanBeMapped) {
  Navigator navigator(tree());
  EXPECT_TRUE(navigator.press(U'å'));
  EXPECT_EQ(navigator.current().name(), "Å");
}

TEST(Navigator, KeyTheStateDoesNotMapChangesNothing) {
  Navigator navigator(tree());
  EXPECT_FALSE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name(), "Rot");
}

TEST(Navigator, BackGoesToTheStateBefore) {
  Navigator navigator(tree());
  navigator.press(U'1');
  navigator.press(U'1');
  EXPECT_EQ(navigator.current().name(), "Djup");
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Ett");
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Rot");
}

TEST(Navigator, PathFollowsKeysAndBack) {
  Navigator navigator(tree());
  ASSERT_EQ(navigator.path().size(), 1U);
  EXPECT_EQ(navigator.path().front()->name(), "Rot");
  navigator.press(U'1');
  navigator.press(U'1');
  ASSERT_EQ(navigator.path().size(), 3U);
  EXPECT_EQ(navigator.path()[1]->name(), "Ett");
  EXPECT_EQ(navigator.path()[2]->name(), "Djup");
  navigator.back();
  EXPECT_EQ(navigator.path().size(), 2U);
}

TEST(Navigator, BackAtRootChangesNothing) {
  Navigator navigator(tree());
  EXPECT_FALSE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Rot");
}

TEST(Navigator, StateCanMapTheBackKey) {
  Navigator navigator(tree());
  EXPECT_TRUE(navigator.press(U'-'));
  EXPECT_EQ(navigator.current().name(), "Minus");
}

TEST(Navigator, StatesAreCreatedOnTheWayInAndDestroyedOnTheWayOut) {
  int alive = 0;
  {
    Navigator navigator(tree(&alive));
    EXPECT_EQ(alive, 1);
    navigator.press(U'1');
    navigator.press(U'1');
    EXPECT_EQ(alive, 3);
    navigator.back();
    EXPECT_EQ(alive, 2);
    navigator.press(U'1'); // a new Djup
    EXPECT_EQ(alive, 3);
  }
  EXPECT_EQ(alive, 0);
}

TEST(State, TakesNoInputByDefault) {
  Navigator navigator(tree());
  auto const result = navigator.current().on_input(U"2025");
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error(), pyroceph::tui::no_input_error);
}
