#include "ui/add_project_menu.hpp"

#include "ui/help_topics.hpp"
#include "core/log.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/details_project_menu.hpp"
#include "ui/project_menu.hpp"
#include "ui/sie_project_menu.hpp"

#include <format>
#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::Accepted;
using pyroceph::tui::CodePoint;
using pyroceph::tui::Completion;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

// The captions, used both in the menu and for the states they lead to
constexpr char const* open_existing_caption = "Öppna befintlig";
constexpr char const* new_project_caption = "Ny bokföring";
constexpr char const* from_sie_caption = "Från SIE";
constexpr char const* from_details_caption = "Från uppgifter";

// Creates a new project, from an SIE file or from the details the user enters
// (name, organisation number and fiscal year).
class NewProjectState : public State {
public:
  NewProjectState(Workspace workspace, std::filesystem::path start_folder, State const* origin)
      : workspace_(std::move(workspace)), start_folder_(std::move(start_folder)), origin_(origin) {}
  std::string name() const override { return new_project_caption; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'1', from_sie_caption}, {U'2', from_details_caption}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return make_project_from_sie(workspace_, start_folder_, origin_);
    if (key == U'2') return make_project_from_details(workspace_, origin_);
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::new_project); }

private:
  Workspace workspace_;
  std::filesystem::path start_folder_;
  State const* origin_;
};

class OpenExistingState : public State {
public:
  OpenExistingState(Workspace workspace, std::filesystem::path start_folder)
      : workspace_(std::move(workspace)), start_folder_(std::move(start_folder)) {}
  std::string name() const override { return open_existing_caption; }
  View view() const override {
    return {"Skriv sökvägen till mappen med bokföringen (den som har .cratchit/project.txt).",
            "Tab fyller i mappnamn och Enter öppnar. - går tillbaka innan du skrivit något, Esc tömmer raden."};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::open_existing); }
  std::u32string start_input() const override { return pyroceph::tui::path_input_of(start_folder_); }
  Completion complete(std::u32string_view text) const override { return pyroceph::tui::complete_path(text); }
  InputResult on_input(std::u32string_view text) override {
    auto const folder = pyroceph::tui::path_of(text);
    auto project = open_project(folder);
    if (!project) {
      LOG_BUSINESS("Kunde inte öppna bokföringen i {}: {}", folder.string(), project.error());
      return std::unexpected(std::format("cratchit kunde inte öppna bokföringen: {}", project.error()));
    }
    LOG_BUSINESS("Öppnade projektet {} i {}", project->name, project->folder.string());
    std::string reply;
    if (auto const remembered = remember_project(workspace_, *project); remembered) {
      LOG_BUSINESS("Samlingsmappen {} kommer ihåg projektet {}", workspace_.folder.string(), project->uuid);
      reply = std::format("Samlingsmappen kommer ihåg {}.", project->name);
    } else {
      LOG_BUSINESS("Samlingsmappen kunde inte komma ihåg projektet: {}", remembered.error());
      reply = std::format("cratchit kunde inte skriva in bokföringen i samlingsmappen: {}", remembered.error());
    }
    return Accepted(make_project_menu(std::move(*project)), std::move(reply));
  }

private:
  Workspace workspace_;
  std::filesystem::path start_folder_;
};

class AddProjectState : public State {
public:
  AddProjectState(Workspace workspace, std::filesystem::path start_folder, State const* origin)
      : workspace_(std::move(workspace)), start_folder_(std::move(start_folder)), origin_(origin) {}
  std::string name() const override { return "Lägg till"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'1', open_existing_caption}, {U'2', new_project_caption}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return make_open_existing_project(workspace_, start_folder_);
    if (key == U'2') return std::make_unique<NewProjectState>(workspace_, start_folder_, origin_);
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::add_project); }

private:
  Workspace workspace_;
  std::filesystem::path start_folder_;
  State const* origin_;
};

} // namespace

std::unique_ptr<State> make_add_project_menu(Workspace workspace, std::filesystem::path start_folder,
                                             State const* origin) {
  return std::make_unique<AddProjectState>(std::move(workspace), std::move(start_folder), origin);
}

std::unique_ptr<State> make_open_existing_project(Workspace workspace, std::filesystem::path start_folder) {
  return std::make_unique<OpenExistingState>(std::move(workspace), std::move(start_folder));
}

} // namespace cratchit::ui
