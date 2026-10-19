// Reads the real (local only, git-ignored) SIE files in _sie_in/.
// Skipped when there are none.

#include "sie_reader.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

using cratchit::Amount;
using cratchit::read_sie_file;

namespace {

std::vector<std::filesystem::path> real_sie_files() {
  std::vector<std::filesystem::path> files;
  std::filesystem::path const dir = std::filesystem::path{CRATCHIT_SOURCE_DIR} / "_sie_in";
  if (!std::filesystem::is_directory(dir)) return files;
  for (auto const& entry : std::filesystem::directory_iterator{dir}) {
    if (entry.path().extension() == ".se") files.push_back(entry.path());
  }
  std::ranges::sort(files);
  return files;
}

} // namespace

TEST(RealSieFiles, AllParseAndVouchersBalance) {
  auto const files = real_sie_files();
  if (files.empty()) GTEST_SKIP() << "No SIE files in _sie_in/";

  for (auto const& path : files) {
    SCOPED_TRACE(path.filename().string());
    auto const doc = read_sie_file(path);
    ASSERT_TRUE(doc.has_value()) << doc.error();

    EXPECT_FALSE(doc->m_org_nr.empty());
    EXPECT_FALSE(doc->m_accounts.empty());

    auto const current_year =
        std::ranges::find(doc->m_financial_years, 0, &cratchit::FinancialYear::m_index);
    ASSERT_NE(current_year, doc->m_financial_years.end()) << "no #RAR 0";

    for (auto const& voucher : doc->m_vouchers) {
      Amount sum;
      for (auto const& posting : voucher.m_postings) sum += posting.m_amount;
      EXPECT_EQ(sum, Amount{}) << "unbalanced voucher " << voucher.m_series << voucher.m_number;
      EXPECT_TRUE(voucher.m_date >= current_year->m_start && voucher.m_date <= current_year->m_end)
          << "voucher " << voucher.m_series << voucher.m_number << " outside #RAR 0";
    }
  }
}
