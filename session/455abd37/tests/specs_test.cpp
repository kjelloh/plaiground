// The spec files and the code must agree. See src/app/spec_check.hpp for what is checked.

#include "app/settings.hpp"
#include "app/spec_check.hpp"
#include "core/account_types.hpp"
#include "cspec/spec_tree.hpp"

#include <gtest/gtest.h>

using namespace cratchit;

namespace {
std::filesystem::path const root{CRATCHIT_SOURCE_DIR};
}

TEST(Specs, SpecFilesAndCodeAgree) {
  auto const problems = check_specs(root);
  std::string all;
  for (auto const& problem : problems) all += problem + "\n";
  EXPECT_TRUE(problems.empty()) << all;
}

TEST(Specs, SettingsDefaultsAndUserOverrides) {
  auto const defaults = Settings::load(root / "specs");
  ASSERT_TRUE(defaults) << defaults.error();
  EXPECT_EQ(defaults->get("vat.settlement_series"), "M");
  EXPECT_EQ(defaults->get("vat.period_length"), "quarter");
  EXPECT_EQ(defaults->get("reporting.responsible_name"), "");
}

TEST(Specs, AccountTypesFollowLookupOrder) {
  auto const tree = cspec::SpecTree::load(root / "specs" / "bas" / "account_types.cspec");
  ASSERT_TRUE(tree) << tree.error();
  auto const types = AccountTypes::from(*tree);
  ASSERT_TRUE(types) << types.error();
  EXPECT_EQ(types->type_of(1920), 'T');
  EXPECT_EQ(types->type_of(2641), 'S');
  EXPECT_EQ(types->type_of(3001), 'I');
  EXPECT_EQ(types->type_of(6540), 'K');
  EXPECT_EQ(types->type_of(8314), 'I'); // group 83
  EXPECT_EQ(types->type_of(8910), 'K'); // group 89
  EXPECT_EQ(types->type_of(8510), std::nullopt); // class 8 is mixed, group 85 unknown
  EXPECT_EQ(types->type_of(8510, 'K'), 'K');     // #KTYP from the SIE file wins
}
