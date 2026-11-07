#include "pyroceph/tui/state.hpp"

namespace pyroceph::tui {

InputResult State::on_input(std::u32string_view) { return std::unexpected(std::string(no_input_error)); }

Navigator::Navigator(std::unique_ptr<State> root) { path_.push_back(std::move(root)); }

bool Navigator::press(CodePoint key) {
  auto next = current().on_key(key);
  if (!next) return false;
  path_.push_back(std::move(next));
  return true;
}

bool Navigator::back() {
  if (!can_go_back()) return false;
  path_.pop_back();
  return true;
}

} // namespace pyroceph::tui
