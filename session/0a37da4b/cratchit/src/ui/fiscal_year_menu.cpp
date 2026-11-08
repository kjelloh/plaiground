#include "ui/fiscal_year_menu.hpp"

#include "ui/help_topics.hpp"

#include <format>
#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

class FiscalYearState : public State {
public:
  FiscalYearState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) {}
  std::string name() const override { return label_of(year_); }
  View view() const override { return {std::format("Räkenskapsår {} – {}", year_.start, year_.end)}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::fiscal_year); }

private:
  Project project_;
  FiscalYear year_;
};

} // namespace

std::unique_ptr<State> make_fiscal_year_menu(Project project, FiscalYear year) {
  return std::make_unique<FiscalYearState>(std::move(project), std::move(year));
}

} // namespace cratchit::ui
