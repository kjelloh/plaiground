#include "pyroceph/tui/prompt_loop.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using charset::Encoding;
using pyroceph::tui::Navigator;
using pyroceph::tui::State;
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
std::string run(KeyReader& keys, Navigator& navigator) {
  auto* out = std::tmpfile();
  pyroceph::tui::run_prompt_loop(keys, navigator, out);
  std::rewind(out);
  std::string written;
  for (int c; (c = std::fgetc(out)) != EOF;) written.push_back(static_cast<char>(c));
  std::fclose(out);
  return written;
}

State tree() {
  return State{"Huvudmeny", {{U'1', "Bokslut", State{"Bokslut", {{U'\u00E5', "År", State{"År", {}}}}}}}};
}

} // namespace

TEST(PromptLoop, ShowsTheFirstStateAtOnce) {
  auto const root = tree();
  Navigator navigator(root);
  auto keys = keys_of({});
  EXPECT_EQ(run(keys, navigator), "\nHuvudmeny\n  1 Bokslut\n");
}

TEST(PromptLoop, ShowsEachNewStateWithBack) {
  auto const root = tree();
  Navigator navigator(root);
  auto keys = keys_of({'1', 'x', '-'}); // x is no choice and is ignored
  EXPECT_EQ(run(keys, navigator),
            "\nHuvudmeny\n  1 Bokslut\n"
            "\nBokslut\n  \u00E5 År\n  - Tillbaka\n"
            "\nHuvudmeny\n  1 Bokslut\n");
}

TEST(PromptLoop, NonAsciiKeyIsAChoice) {
  auto const root = tree();
  Navigator navigator(root);
  auto keys = keys_of({'1', 0xC3, 0xA5});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name, "År");
}

TEST(PromptLoop, QuitKeysEndTheLoop) {
  for (std::uint8_t quit : {0x03, 0x04, 0x1A}) {
    auto const root = tree();
    Navigator navigator(root);
    auto keys = keys_of({quit, '1'});
    run(keys, navigator);
    EXPECT_EQ(navigator.current().name, "Huvudmeny") << int{quit};
  }
}
