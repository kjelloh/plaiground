#pragma once

#include "pyroceph/tui/key_reader.hpp"

#include <string>
#include <vector>

namespace pyroceph::tui {

// The key that goes back to the state before. A choice with this key cannot be reached.
inline constexpr CodePoint back_key = U'-';

struct Choice;

// One place in the UI: its name and the choices it offers
struct State {
  std::string name;            // UTF-8, e.g. "Huvudmeny"
  std::vector<Choice> choices; // in display order
};

// A key that leads to another state
struct Choice {
  CodePoint key;       // preferably '0'..'9', but any code point
  std::string caption; // UTF-8, e.g. "Bokslut"
  State target;
};

// Where the user is in a tree of states, and the way there
class Navigator {
public:
  // The tree must outlive the navigator
  explicit Navigator(State const& root) : path_{&root} {}
  explicit Navigator(State&&) = delete;

  State const& current() const { return *path_.back(); }

  // True when there is a state before the current one
  bool can_go_back() const { return path_.size() > 1; }

  // A key that is a choice moves to its state, back_key moves back to the
  // state before. True when the state changed.
  bool press(CodePoint key);

private:
  std::vector<State const*> path_; // front() is the root, back() is the current state
};

} // namespace pyroceph::tui
