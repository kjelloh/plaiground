#pragma once

#include <string_view>

namespace cratchit {

// The application version, as set by the project() call in CMakeLists.txt
std::string_view version();

} // namespace cratchit
