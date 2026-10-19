// Milestone 1 acceptance test: cratchit calculates the same VAT returns as the ones that were
// filed with SKV (real, local only, git-ignored files in _sie_in/ and _eskd_irl/).
// Skipped when the files are not there.

#include "eskd_reader.hpp"
#include "sie_reader.hpp"
#include "vat_calculation.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <vector>

using cratchit::calculate_vat_return;
using cratchit::read_eskd_file;
using cratchit::read_sie_file;
using cratchit::Voucher;

namespace {

std::filesystem::path const source_dir{CRATCHIT_SOURCE_DIR};

std::vector<std::filesystem::path> files_with_extension(std::filesystem::path const& dir, std::string_view extension) {
  std::vector<std::filesystem::path> files;
  if (!std::filesystem::is_directory(dir)) return files;
  for (auto const& entry : std::filesystem::directory_iterator{dir}) {
    if (entry.path().extension() == extension) files.push_back(entry.path());
  }
  return files;
}

} // namespace

TEST(RealVatReturns, CalculatedEqualsFiled) {
  auto const sie_files = files_with_extension(source_dir / "_sie_in", ".se");
  auto const eskd_files = files_with_extension(source_dir / "_eskd_irl", ".eskd");
  if (sie_files.empty() || eskd_files.empty()) GTEST_SKIP() << "No real SIE or eskd files";

  // All financial years together, since a VAT quarter can span two of them.
  std::vector<Voucher> vouchers;
  std::string org_nr;
  for (auto const& path : sie_files) {
    auto document = read_sie_file(path);
    ASSERT_TRUE(document.has_value()) << document.error();
    org_nr = document->m_org_nr;
    std::ranges::move(document->m_vouchers, std::back_inserter(vouchers));
  }

  for (auto const& path : eskd_files) {
    SCOPED_TRACE(path.filename().string());
    auto const filed = read_eskd_file(path);
    ASSERT_TRUE(filed.has_value()) << filed.error();

    auto const calculated = calculate_vat_return(vouchers, org_nr, filed->m_period);
    ASSERT_TRUE(calculated.has_value()) << calculated.error();
    EXPECT_EQ(*calculated, *filed);
  }
}
