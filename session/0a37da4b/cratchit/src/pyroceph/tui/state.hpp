#pragma once

#include "pyroceph/tui/key_reader.hpp"

#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pyroceph::tui {

// The key that goes back to the state before. A choice with this key cannot be reached.
inline constexpr CodePoint back_key = U'-';

struct Choice;

// What a state answers to text the user entered: a reply to show (may be
// empty) when it accepts the text, or an error text when it does not. All UTF-8.
using InputResult = std::expected<std::string, std::string>;

// Acts on text the user entered in a state
using OnInput = std::function<InputResult(std::u32string_view)>;

// One place in the UI: its name, the choices it offers and what it does with
// text the user enters there
struct State {
  // Overloads rather than default arguments, since Choice is not complete yet
  explicit State(std::string name);
  State(std::string name, std::vector<Choice> choices);
  State(std::string name, std::vector<Choice> choices, OnInput on_input);

  std::string name;            // UTF-8, e.g. "Huvudmeny". Shown in the path, so keep it short.
  std::vector<Choice> choices; // in display order
  OnInput on_input;            // empty: the state takes no input
};

// A key that leads to another state
struct Choice {
  CodePoint key;       // preferably '0'..'9', but any code point
  std::string caption; // UTF-8, e.g. "Bokslut". Shown in the menu only.
  State target;
};

inline State::State(std::string name) : name(std::move(name)) {}

inline State::State(std::string name, std::vector<Choice> choices)
    : name(std::move(name)), choices(std::move(choices)) {}

inline State::State(std::string name, std::vector<Choice> choices, OnInput on_input)
    : name(std::move(name)), choices(std::move(choices)), on_input(std::move(on_input)) {}

// Where the user is in a tree of states, and the way there
class Navigator {
public:
  // The tree must outlive the navigator
  explicit Navigator(State const& root) : path_{&root} {}
  explicit Navigator(State&&) = delete;

  State const& current() const { return *path_.back(); }

  // The states from the root to the current state, in order
  std::vector<State const*> const& path() const { return path_; }

  // True when there is a state before the current one
  bool can_go_back() const { return path_.size() > 1; }

  // A key that is a choice moves to its state, back_key moves back to the
  // state before. True when the state changed.
  bool press(CodePoint key);

private:
  std::vector<State const*> path_; // front() is the root, back() is the current state
};

} // namespace pyroceph::tui
