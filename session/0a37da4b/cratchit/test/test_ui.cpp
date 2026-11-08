#include "ui/root_menu.hpp"
#include "ui/test_menu.hpp"

#include <gtest/gtest.h>

using pyroceph::tui::Navigator;
using pyroceph::tui::OptionsList;

TEST(RootMenu, IsCratchitWithTheTestMenuAsNine) {
  auto const root = cratchit::ui::make_root_menu();
  EXPECT_EQ(root->name(), "Cratchit");
  EXPECT_EQ(root->options(), (OptionsList{{U'9', "Testmeny"}}));
}

TEST(RootMenu, NineLeadsToTheTestMenu) {
  Navigator navigator(cratchit::ui::make_root_menu());
  EXPECT_TRUE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name(), "Testmeny");
}

TEST(RootMenu, OtherKeysAreNotMapped) {
  auto root = cratchit::ui::make_root_menu();
  EXPECT_EQ(root->on_key(U'1'), nullptr);
}

TEST(TestMenu, KeepsItsStates) {
  Navigator navigator(cratchit::ui::make_test_menu());
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Projekt"}, {U'2', "Bokslut"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Projekt");
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Öppna projekt");
  navigator.back();
  navigator.back();
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(TestMenu, BokslutTakesAYearOfFourDigits) {
  Navigator navigator(cratchit::ui::make_test_menu());
  navigator.press(U'2');
  auto const accepted = navigator.current().on_input(U"2025");
  ASSERT_TRUE(accepted);
  EXPECT_EQ(*accepted, "Bokslut för 2025 noterat.");
  EXPECT_FALSE(navigator.current().on_input(U"25"));
}
