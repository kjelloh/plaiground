#include "core/operations.hpp"

#include <gtest/gtest.h>

#include <sstream>

using cratchit::Operations;

namespace {

enum class Op { Save, Flip, Close, Open };

} // namespace

TEST(Operations, NumberPicksTheOperationOffered) {
  Operations<Op> operations;
  operations.add(1, "Save", Op::Save).add(2, "Flip", Op::Flip);
  EXPECT_EQ(operations.on_key(U'1'), Op::Save);
  EXPECT_EQ(operations.on_key(U'2'), Op::Flip);
  EXPECT_FALSE(operations.on_key(U'3').has_value());
  EXPECT_FALSE(operations.on_key(U's').has_value());
  std::ostringstream out;
  operations.show(out);
  EXPECT_EQ(out.str(), "  1  Save\n  2  Flip\n");
}

TEST(Operations, OneNotOfferedLeavesAGap) {
  Operations<Op> operations;
  operations.add(1, "Save", Op::Save, false).add(2, "Flip", Op::Flip);
  EXPECT_FALSE(operations.on_key(U'1').has_value());
  EXPECT_EQ(operations.on_key(U'2'), Op::Flip);
  std::ostringstream out;
  operations.show(out);
  EXPECT_EQ(out.str(), "  2  Flip\n");
}

TEST(Operations, TwoMayShareANumberWhenOneIsOffered) {
  for (auto const closed : {false, true}) {
    Operations<Op> operations;
    operations.add(8, "Close", Op::Close, !closed).add(8, "Open", Op::Open, closed);
    EXPECT_EQ(operations.on_key(U'8'), closed ? Op::Open : Op::Close);
  }
}

TEST(Operations, OtherKeysAreNamedInOneLineOnly) {
  Operations<Op> operations;
  operations.add(1, "Save", Op::Save).other("d", "delete").other("-", "back").other("q", "quit");
  std::ostringstream out;
  operations.show(out);
  EXPECT_EQ(out.str(), "  1  Save\n");
  EXPECT_EQ(operations.one_liner(), "Other keys: d delete, - back, q quit.");
  EXPECT_FALSE(operations.on_key(U'd').has_value());
}
