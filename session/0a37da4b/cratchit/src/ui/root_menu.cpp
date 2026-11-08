#include "ui/root_menu.hpp"

#include "ui/test_menu.hpp"

#include <string>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;

class RootState : public State {
public:
  std::string name() const override { return "Cratchit"; }
  OptionsList options() const override { return {{U'9', "Testmeny"}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'9') return make_test_menu();
    return nullptr;
  }
};

} // namespace

std::unique_ptr<State> make_root_menu() { return std::make_unique<RootState>(); }

} // namespace cratchit::ui
