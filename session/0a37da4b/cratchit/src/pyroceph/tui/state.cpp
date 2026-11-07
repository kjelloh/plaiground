#include "pyroceph/tui/state.hpp"

namespace pyroceph::tui {

bool Navigator::press(CodePoint key) {
  if (key == back_key) {
    if (!can_go_back()) return false;
    path_.pop_back();
    return true;
  }
  for (auto const& choice : current().choices) {
    if (choice.key == key) {
      path_.push_back(&choice.target);
      return true;
    }
  }
  return false;
}

} // namespace pyroceph::tui
