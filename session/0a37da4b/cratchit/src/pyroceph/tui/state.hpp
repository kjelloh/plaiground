#pragma once

#include "pyroceph/tui/key_reader.hpp"

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pyroceph::tui {

// The key that goes back to the state before, unless the current state maps it itself
inline constexpr CodePoint back_key = U'-';

// The error text when a state that takes no input gets text
inline constexpr std::string_view no_input_error = "Här finns inget att göra med indata.";

class State;

// What a state answers to a key: the next state, or nullptr when the state has
// no mapping for the key. The TUI takes over the new state.
using OnKeyResult = std::unique_ptr<State>;

// One option a state offers: its key and its caption (UTF-8, e.g. "Bokslut")
using Option = std::pair<CodePoint, std::string>;

// The options a state offers, in display order. The TUI formats the list.
using OptionsList = std::vector<Option>;

// The lines of a state's view: its content, shown above its options. The TUI
// formats the lines. All UTF-8.
using View = std::vector<std::string>;

// What a state answers to text the user entered: a reply to show (may be
// empty) when it accepts the text, or an error text when it does not. All UTF-8.
using InputResult = std::expected<std::string, std::string>;

// One place in the UI. The client derives its concrete states from it.
//
// A state has two parts that the TUI shows: its view (its content) and, below
// it, its options (what the user can do from here with a key). What a key
// reaches is an option; everything else is the view.
//
// A state creates the next state itself, in on_key, when the user goes there.
// So the tree of states grows as the user navigates, and a state can offer
// options that are only known when the program runs (projects, years, ...).
class State {
public:
  virtual ~State() = default;

  // The place's name, e.g. "Huvudmeny". Shown in the path, so keep it short. UTF-8.
  virtual std::string name() const = 0;

  // The state's content, shown above the options. Empty when there is nothing to show.
  virtual View view() const = 0;

  // The options to show the user. The state keeps them in step with on_key.
  virtual OptionsList options() const = 0;

  // The next state for the key, or nullptr when the state has no mapping for it
  virtual OnKeyResult on_key(CodePoint key) = 0;

  // Acts on text the user entered. By default the state takes no input and
  // answers with no_input_error.
  virtual InputResult on_input(std::u32string_view text);
};

// The states from the root to the current one. Owns them: a state is
// destroyed when the user leaves it.
class Navigator {
public:
  // root must not be empty
  explicit Navigator(std::unique_ptr<State> root);

  State& current() const { return *path_.back(); }

  // The states from the root to the current state, in order
  std::vector<std::unique_ptr<State>> const& path() const { return path_; }

  // True when there is a state before the current one
  bool can_go_back() const { return path_.size() > 1; }

  // Gives the key to the current state. When it answers with a new state,
  // that becomes the current state. True when the state changed.
  bool press(CodePoint key);

  // Leaves the current state, which is destroyed, for the state before. Does
  // nothing at the root. True when the state changed.
  bool back();

private:
  std::vector<std::unique_ptr<State>> path_; // front() is the root, back() is the current state
};

} // namespace pyroceph::tui
