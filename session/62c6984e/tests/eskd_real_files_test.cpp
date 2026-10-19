// Reads the real (local only, git-ignored) eskd files in _eskd_irl/ that were sent to SKV,
// writes them back and checks the result is byte-identical. Skipped when there are none.

#include "eskd_reader.hpp"
#include "eskd_writer.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <iterator>

using cratchit::read_eskd_file;
using cratchit::to_eskd;

TEST(RealEskdFiles, WriterReproducesFiledFiles) {
  std::filesystem::path const dir = std::filesystem::path{CRATCHIT_SOURCE_DIR} / "_eskd_irl";
  int count = 0;
  if (std::filesystem::is_directory(dir)) {
    for (auto const& entry : std::filesystem::directory_iterator{dir}) {
      if (entry.path().extension() != ".eskd") continue;
      ++count;
      SCOPED_TRACE(entry.path().filename().string());

      std::ifstream file{entry.path(), std::ios::binary};
      std::string const original{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

      auto const vat_return = read_eskd_file(entry.path());
      ASSERT_TRUE(vat_return.has_value()) << vat_return.error();
      auto const written = to_eskd(*vat_return);
      ASSERT_TRUE(written.has_value()) << written.error();
      EXPECT_EQ(*written, original);
    }
  }
  if (count == 0) GTEST_SKIP() << "No eskd files in _eskd_irl/";
}
