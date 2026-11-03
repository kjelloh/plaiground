#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ink2 {

// What the command line asks for
struct Arguments {
  bool help = false;          // --help or -h, alone
  std::filesystem::path file; // the SIE file
};

// cratchit_ink2 <file.se>, or cratchit_ink2 --help (-h). Error text for an unknown
// option, or a missing or extra file.
std::expected<Arguments, std::string> parse_arguments(std::vector<std::string_view> const& arguments);

// The usage line
constexpr std::string_view usage = "Usage: cratchit_ink2 <file.se>";

} // namespace cratchit::ink2
