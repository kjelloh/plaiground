#pragma once

// A State for tests: a name, options that each create their next state, and
// an optional on_input. Only for tests; a client writes its own State classes.

#include "pyroceph/tui/state.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace test {

using pyroceph::tui::CodePoint;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;

struct Item {
  CodePoint key;
  std::string caption;
  std::function<OnKeyResult()> next;
};

class MenuState : public State {
public:
  MenuState(std::string name, std::vector<Item> items,
            std::function<InputResult(std::u32string_view)> input, int* alive)
      : name_(std::move(name)), items_(std::move(items)), input_(std::move(input)), alive_(alive) {
    if (alive_) ++*alive_;
  }
  ~MenuState() override {
    if (alive_) --*alive_;
  }

  std::string name() const override { return name_; }

  OptionsList options() const override {
    OptionsList list;
    for (auto const& item : items_) list.emplace_back(item.key, item.caption);
    return list;
  }

  OnKeyResult on_key(CodePoint key) override {
    for (auto const& item : items_) {
      if (item.key == key) return item.next();
    }
    return nullptr;
  }

  InputResult on_input(std::u32string_view text) override {
    return input_ ? input_(text) : State::on_input(text);
  }

private:
  std::string name_;
  std::vector<Item> items_;
  std::function<InputResult(std::u32string_view)> input_;
  int* alive_; // counts the living states, when not null
};

inline std::unique_ptr<State> menu(std::string name, std::vector<Item> items = {},
                                   std::function<InputResult(std::u32string_view)> input = {},
                                   int* alive = nullptr) {
  return std::make_unique<MenuState>(std::move(name), std::move(items), std::move(input), alive);
}

} // namespace test
