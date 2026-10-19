#include "vat_box.hpp"

#include <gtest/gtest.h>

#include <set>
#include <string_view>

using cratchit::find_vat_box;
using cratchit::vat_boxes;

TEST(VatBox, NumbersAndElementsAreUnique) {
  std::set<int> numbers;
  std::set<std::string_view> elements;
  for (auto const& box : vat_boxes()) {
    EXPECT_TRUE(numbers.insert(box.m_number).second) << box.m_number;
    EXPECT_TRUE(elements.insert(box.m_element).second) << box.m_element;
  }
  EXPECT_EQ(numbers.size(), 29u);
}

TEST(VatBox, Lookup) {
  ASSERT_NE(find_vat_box(48), nullptr);
  EXPECT_EQ(find_vat_box(48)->m_element, "MomsIngAvdr");
  ASSERT_NE(find_vat_box("MomsBetala"), nullptr);
  EXPECT_EQ(find_vat_box("MomsBetala")->m_number, 49);
  EXPECT_EQ(find_vat_box(9), nullptr);
  EXPECT_EQ(find_vat_box("Period"), nullptr);
}
