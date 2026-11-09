#include "ui/details_project_menu.hpp"

#include "charset/utf8.hpp"
#include "core/details_project.hpp"
#include "core/log.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "ui/help_topics.hpp"

#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::Accepted;
using pyroceph::tui::CodePoint;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

std::string utf8_of(std::u32string_view text) {
  charset::utf8::Encoder encoder;
  std::string result;
  for (auto const code_point : text) {
    for (auto const byte : encoder.push(code_point)) result.push_back(static_cast<char>(byte));
  }
  return result;
}

std::u32string u32_of(std::string_view utf8) {
  charset::utf8::Decoder decoder;
  std::u32string result;
  for (auto const c : utf8) {
    if (auto const code_point = decoder.push(static_cast<std::uint8_t>(c))) result.push_back(*code_point);
  }
  return result;
}

// What the user has given so far
struct Details {
  Workspace workspace;
  State const* origin = nullptr;
  std::string name;
  std::filesystem::path folder;
  std::string orgnr;
  FiscalYear year;
};

class ConfirmDetailsState : public State {
public:
  explicit ConfirmDetailsState(Details details) : details_(std::move(details)) {}
  std::string name() const override { return "Bekräfta"; }
  View view() const override {
    if (created_) return {std::format("cratchit har skapat bokföringen i {}.", details_.folder.string())};
    return {std::format("Företag: {} ({})", details_.name, details_.orgnr),
            std::format("Räkenskapsår: {} – {}", details_.year.start, details_.year.end),
            std::format("cratchit skapar bokföringen i {}.", details_.folder.string()),
            "Bokföringen får ingen kontoplan än."};
  }
  OptionsList options() const override {
    if (created_) return {};
    return {{U'1', "Skapa bokföringen"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (created_ || key != U'1') return nullptr;
    auto project = create_project_from_details(details_.folder, details_.name, details_.orgnr, details_.year, today_iso());
    if (!project) {
      LOG_BUSINESS("Kunde inte skapa bokföringen {}: {}", details_.name, project.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte skapa bokföringen: {}", project.error()));
    }
    created_ = true;
    LOG_BUSINESS("Skapade projektet {} ({}) i {} med räkenskapsåret {} – {}", project->name, details_.orgnr,
                 project->folder.string(), details_.year.start, details_.year.end);
    if (auto const remembered = remember_project(details_.workspace, *project); !remembered) {
      LOG_BUSINESS("Samlingsmappen kunde inte komma ihåg projektet: {}", remembered.error());
      return pyroceph::tui::make_error_state(
          std::format("cratchit skapade bokföringen men kunde inte skriva in den i samlingsmappen: {}", remembered.error()));
    }
    return nullptr; // back to where the user started (back_to)
  }
  State const* back_to() const override { return created_ ? details_.origin : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::confirm_details_project); }

private:
  Details details_;
  bool created_ = false;
};

// One step: a text the user types, checked and kept, leading to the next step
class StepState : public State {
public:
  explicit StepState(Details details) : details_(std::move(details)) {}
  View view() const override { return {instruction()}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::u32string start_input() const override { return u32_of(given_.empty() ? suggestion() : given_); }
  InputResult on_input(std::u32string_view text) override {
    auto const typed = utf8_of(text);
    auto next = take(typed);
    if (!next) return std::unexpected(next.error());
    given_ = typed;
    return Accepted(std::move(*next));
  }

protected:
  virtual std::string instruction() const = 0;
  virtual std::string suggestion() const { return {}; }
  // The next step with the text taken, or why the text cannot be taken
  virtual std::expected<std::unique_ptr<State>, std::string> take(std::string const& text) const = 0;

  Details details_;

private:
  std::string given_; // what the user gave here last, offered again
};

class YearStep : public StepState {
public:
  using StepState::StepState;
  std::string name() const override { return details_.orgnr; }
  std::string help_topic() const override { return std::string(help_topic::details_year); }

protected:
  std::string instruction() const override {
    return "Skriv första och sista dagen i det första räkenskapsåret och tryck Enter, till exempel 2026-01-01 2026-12-31.";
  }
  std::string suggestion() const override {
    auto const year = today_iso().substr(0, 4);
    return std::format("{}-01-01 {}-12-31", year, year);
  }
  std::expected<std::unique_ptr<State>, std::string> take(std::string const& text) const override {
    auto year = parse_first_fiscal_year(text);
    if (!year) return std::unexpected(year.error());
    auto details = details_;
    details.year = *year;
    return std::make_unique<ConfirmDetailsState>(std::move(details));
  }
};

class OrgnrStep : public StepState {
public:
  using StepState::StepState;
  std::string name() const override { return details_.name; }
  std::string help_topic() const override { return std::string(help_topic::details_orgnr); }

protected:
  std::string instruction() const override {
    return "Skriv företagets organisationsnummer och tryck Enter, till exempel 556677-8899.";
  }
  std::expected<std::unique_ptr<State>, std::string> take(std::string const& text) const override {
    auto orgnr = parse_orgnr(text);
    if (!orgnr) return std::unexpected(orgnr.error());
    auto details = details_;
    details.orgnr = *orgnr;
    return std::make_unique<YearStep>(std::move(details));
  }
};

class NameStep : public StepState {
public:
  using StepState::StepState;
  std::string name() const override { return "Från uppgifter"; }
  std::string help_topic() const override { return std::string(help_topic::from_details); }

protected:
  std::string instruction() const override {
    return "Skriv företagets namn och tryck Enter. Bokföringen får en mapp med det namnet i samlingsmappen.";
  }
  std::expected<std::unique_ptr<State>, std::string> take(std::string const& text) const override {
    auto folder = new_project_folder(details_.workspace, text);
    if (!folder) return std::unexpected(folder.error());
    auto details = details_;
    details.name = folder->filename().string();
    details.folder = *folder;
    return std::make_unique<OrgnrStep>(std::move(details));
  }
};

} // namespace

std::unique_ptr<State> make_project_from_details(Workspace workspace, State const* origin) {
  return std::make_unique<NameStep>(Details{std::move(workspace), origin, {}, {}, {}, {}});
}

} // namespace cratchit::ui
