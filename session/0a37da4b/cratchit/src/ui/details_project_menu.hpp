#pragma once

#include "core/dot_folder.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// Creates a new project from details the user types, one at a time, each step
// a state of its own whose name is the previous answer:
// * "Från uppgifter": the company's name (see new_project_folder),
// * "<name>": the organisation number (see parse_orgnr), required,
// * "<orgnr>": the first fiscal year's first and last day (see
//   parse_first_fiscal_year), starting with the current calendar year,
// * "Bekräfta": shows the details and offers 1 "Skapa bokföringen".
// A step that refuses the text shows why and keeps it. Each step starts with
// what the user gave there before, so - goes back while it is untouched.
//
// 1 creates the project in the workspace folder (see
// create_project_from_details), without a chart of accounts yet. The workspace
// remembers it, and the TUI goes back to origin (State::back_to), e.g. the
// workspace that now lists it. Without origin "Bekräfta" stays and says that the
// project was created. A failure leads to the error state.
std::unique_ptr<pyroceph::tui::State> make_project_from_details(Workspace workspace,
                                                                pyroceph::tui::State const* origin = nullptr);

} // namespace cratchit::ui
