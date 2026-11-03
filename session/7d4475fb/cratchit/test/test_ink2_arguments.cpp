#include "core/ink2/arguments.hpp"

#include <gtest/gtest.h>

using cratchit::ink2::parse_arguments;

TEST(Ink2Arguments, File) {
  auto const arguments = parse_arguments({"books.se"});
  ASSERT_TRUE(arguments);
  EXPECT_FALSE(arguments->help);
  EXPECT_EQ(arguments->file, "books.se");
}

TEST(Ink2Arguments, Help) {
  EXPECT_TRUE(parse_arguments({"--help"})->help);
  EXPECT_TRUE(parse_arguments({"-h"})->help);
}

TEST(Ink2Arguments, Wrong) {
  EXPECT_EQ(parse_arguments({}).error(), "No SIE file given");
  EXPECT_EQ(parse_arguments({"a.se", "b.se"}).error(), "Only one SIE file, not also b.se");
  EXPECT_EQ(parse_arguments({"--trace", "a.se"}).error(), "Unknown option --trace");
  EXPECT_EQ(parse_arguments({"--help", "a.se"}).error(), "Unknown option --help");
}
