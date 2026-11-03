#include "core/ink2/arguments.hpp"

#include <format>

namespace cratchit::ink2 {

std::expected<Arguments, std::string> parse_arguments(std::vector<std::string_view> const& arguments) {
  if (arguments.size() == 1 && (arguments.front() == "--help" || arguments.front() == "-h")) {
    return Arguments{true, {}};
  }
  Arguments parsed;
  bool has_file = false;
  for (auto const argument : arguments) {
    if (argument.starts_with('-')) {
      return std::unexpected(std::format("Unknown option {}", argument));
    }
    if (has_file) {
      return std::unexpected(std::format("Only one SIE file, not also {}", argument));
    }
    parsed.file = argument;
    has_file = true;
  }
  if (!has_file) {
    return std::unexpected("No SIE file given");
  }
  return parsed;
}

} // namespace cratchit::ink2
