#include "core/version.hpp"

#include <gtest/gtest.h>

TEST(Version, IsNotEmpty) {
  EXPECT_FALSE(cratchit::version().empty());
}
