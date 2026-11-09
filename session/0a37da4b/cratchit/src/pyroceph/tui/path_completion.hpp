#pragma once

#include "pyroceph/tui/state.hpp"

#include <filesystem>
#include <functional>
#include <string_view>

namespace pyroceph::tui {

// What to show for a folder in a path the user types: the folder and a
// separator after it, e.g. U"/Users/me/Documents/"
std::u32string path_input_of(std::filesystem::path const& folder);

// The path the user typed, as the platform reads it
std::filesystem::path path_of(std::u32string_view typed);

// Completes the last name in typed to the existing folders (and, with files,
// files) whose names start with it, like Tab in a shell (from take 3):
// * One match: the rest of its name, and a separator after a folder.
// * Several: as far as they agree, or, when they agree on nothing more than
//   typed, nothing added and their names listed, sorted.
// * Hidden names (.name) count only when the typed name starts with a dot.
// * Nothing for a typed text without a separator, or a folder that cannot be read.
// Names compare as typed on macOS and Linux, and ignoring case (A-Z) on Windows.
// With accept_file, only the files it accepts count (folders always count).
Completion complete_path(std::u32string_view typed, bool files = false,
                         std::function<bool(std::filesystem::path const&)> const& accept_file = {});

} // namespace pyroceph::tui
