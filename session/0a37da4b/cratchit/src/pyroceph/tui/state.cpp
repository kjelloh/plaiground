#include "pyroceph/tui/state.hpp"

namespace pyroceph::tui {

InputResult State::on_input(std::u32string_view) { return std::unexpected(std::string(no_input_error)); }

Completion State::complete(std::u32string_view) const { return {}; }

Navigator::Navigator(std::unique_ptr<State> root) { path_.push_back(std::move(root)); }

bool Navigator::press(CodePoint key) {
  auto next = current().on_key(key);
  if (!next) return false;
  enter(std::move(next));
  return true;
}

void Navigator::enter(std::unique_ptr<State> next) { path_.push_back(std::move(next)); }

bool Navigator::back() {
  if (!can_go_back()) return false;
  path_.pop_back();
  current().on_return();
  return true;
}

} // namespace pyroceph::tui
