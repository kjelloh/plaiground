#include "pyroceph/tui/prompt_loop.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using charset::Encoding;
using pyroceph::tui::Choice;
using pyroceph::tui::CodePoint;
using pyroceph::tui::KeyReader;

namespace {

KeyReader keys_of(std::vector<std::uint8_t> bytes) {
  auto shared = std::make_shared<std::vector<std::uint8_t>>(std::move(bytes));
  auto next = std::make_shared<std::size_t>(0);
  return KeyReader(
      [shared, next]() -> std::optional<std::uint8_t> {
        if (*next == shared->size()) return std::nullopt;
        return (*shared)[(*next)++];
      },
      Encoding::utf8);
}

// Runs the loop and gives back what it wrote
std::string run(KeyReader& keys, pyroceph::tui::Lookup const& lookup) {
  auto* out = std::tmpfile();
  pyroceph::tui::run_prompt_loop(keys, lookup, out);
  std::rewind(out);
  std::string written;
  for (int c; (c = std::fgetc(out)) != EOF;) written.push_back(static_cast<char>(c));
  std::fclose(out);
  return written;
}

std::optional<Choice> nothing(CodePoint) { return std::nullopt; }

} // namespace

TEST(PromptLoop, OneLinePerKeyUntilInputEnds) {
  auto keys = keys_of({'a', 0xC3, 0xA5});
  EXPECT_EQ(run(keys, nothing), "U+0061 (inget val)\nU+00E5 (inget val)\n");
}

TEST(PromptLoop, LooksUpEachCodePoint) {
  auto keys = keys_of({'a', 'b'});
  std::vector<CodePoint> looked_up;
  auto written = run(keys, [&](CodePoint code_point) -> std::optional<Choice> {
    looked_up.push_back(code_point);
    if (code_point == U'b') return Choice{};
    return std::nullopt;
  });
  EXPECT_EQ(looked_up, (std::vector<CodePoint>{U'a', U'b'}));
  EXPECT_EQ(written, "U+0061 (inget val)\nU+0062 (val)\n");
}

TEST(PromptLoop, QuitKeysEndTheLoopWithoutLookup) {
  for (std::uint8_t quit : {0x03, 0x04, 0x1A}) {
    auto keys = keys_of({'a', quit, 'b'});
    EXPECT_EQ(run(keys, nothing), "U+0061 (inget val)\n") << int{quit};
  }
}

TEST(PromptLoop, EndedInputWritesNothing) {
  auto keys = keys_of({});
  EXPECT_EQ(run(keys, nothing), "");
}
