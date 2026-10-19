// Acceptance tests on real, local-only data (never committed).
// Uses take 1's folders: ../62c6984e/_sie_in (Fortnox SIE exports) and ../62c6984e/_eskd_irl (filed VAT returns).
// Skips when the folders are absent.

#include "cspec/spec_tree.hpp"
#include "sie/sie_reader.hpp"
#include "sie/sie_to_journal.hpp"
#include "vat/eskd_reader.hpp"
#include "vat/eskd_writer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace cratchit;

namespace {

std::filesystem::path const root{CRATCHIT_SOURCE_DIR};
std::filesystem::path const real_data = root / ".." / "62c6984e";

std::vector<std::filesystem::path> files_in(std::filesystem::path const& dir, std::string const& extension) {
  std::vector<std::filesystem::path> files;
  if (!std::filesystem::is_directory(dir)) return files;
  for (auto const& entry : std::filesystem::directory_iterator(dir)) {
    if (entry.path().extension() == extension) files.push_back(entry.path());
  }
  std::ranges::sort(files);
  return files;
}

std::string read(std::filesystem::path const& path) {
  std::ifstream file{path, std::ios::binary};
  std::ostringstream content;
  content << file.rdbuf();
  return content.str();
}

std::vector<Books> all_real_books() {
  auto const tree = cspec::SpecTree::load(root / "specs" / "sie" / "sie4.cspec");
  auto const spec = sie::SieSpec::from(*tree);
  std::vector<Books> books;
  for (auto const& path : files_in(real_data / "_sie_in", ".se")) {
    auto const records = sie::read_file(path, *spec);
    EXPECT_TRUE(records) << records.error();
    if (!records) continue;
    auto year = sie::to_books(*records);
    EXPECT_TRUE(year) << path << ": " << year.error();
    if (year) books.push_back(std::move(*year));
  }
  return books;
}

} // namespace

TEST(RealFiles, AllSieFilesFollowTheSpec) {
  if (files_in(real_data / "_sie_in", ".se").empty()) GTEST_SKIP() << "no local SIE files";
  EXPECT_FALSE(all_real_books().empty());
}

TEST(RealFiles, ReproducesFiledVatReturns) {
  auto const filed = files_in(real_data / "_eskd_irl", ".eskd");
  if (filed.empty() || files_in(real_data / "_sie_in", ".se").empty()) GTEST_SKIP() << "no local filed returns or SIE files";
  auto const books = all_real_books();
  auto const vat = vat::load_vat_spec(root / "specs");
  ASSERT_TRUE(vat) << vat.error();
  auto const eskd = vat::load_eskd_spec(root / "specs");
  ASSERT_TRUE(eskd) << eskd.error();

  int byte_equal = 0;
  for (auto const& path : filed) {
    auto const content = read(path);
    auto const expected = vat::read_eskd(content, *vat);
    ASSERT_TRUE(expected) << path << ": " << expected.error();
    auto const period = vat::VatPeriod::parse(expected->m_period, "quarter");
    ASSERT_TRUE(period) << period.error();
    auto const vat_return = vat::compute_vat_return(books, *period, "M", *vat);
    ASSERT_TRUE(vat_return) << path << ": " << vat_return.error();
    EXPECT_TRUE(vat::same_values(vat_return->m_boxes, expected->m_boxes)) << path;
    auto const written = vat::to_eskd(*vat_return, *eskd, *vat);
    ASSERT_TRUE(written) << written.error();
    if (*written == content) ++byte_equal;
    std::cout << "  " << path.filename().string() << ": " << (vat::same_values(vat_return->m_boxes, expected->m_boxes) ? "same values" : "DIFFERENT")
              << (*written == content ? ", byte for byte" : "") << "\n";
  }
  std::cout << "  " << byte_equal << " of " << filed.size() << " filed files reproduced byte for byte\n";
}
