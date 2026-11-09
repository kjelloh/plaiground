#pragma once

#include "pyroceph/tui/error_state.hpp"

#include <array>
#include <string_view>

namespace cratchit::ui {

// The help topics of cratchit's states (see State::help_topic). Each one marks
// a help paragraph in the user manual, docs/anvandarmanual.md, as
// `<!-- hjälp: <topic> -->`.
namespace help_topic {
inline constexpr std::string_view root = "start";
inline constexpr std::string_view workspace = "samlingsmapp";
inline constexpr std::string_view add_project = "lagg-till";
inline constexpr std::string_view open_existing = "oppna-befintlig";
inline constexpr std::string_view new_project = "ny-bokforing";
inline constexpr std::string_view from_sie = "fran-sie";
inline constexpr std::string_view confirm_sie_project = "bekrafta-sie";
inline constexpr std::string_view from_details = "fran-uppgifter";
inline constexpr std::string_view details_name = "uppgifter-namn";
inline constexpr std::string_view details_orgnr = "uppgifter-orgnr";
inline constexpr std::string_view details_year = "uppgifter-rakenskapsar";
inline constexpr std::string_view details_months = "uppgifter-langd";
inline constexpr std::string_view project = "bokforing";
inline constexpr std::string_view forget_project = "glom";
inline constexpr std::string_view fiscal_year = "rakenskapsar";
inline constexpr std::string_view export_sie = "exportera-sie";
inline constexpr std::string_view sie_difference = "skillnad-sie";
inline constexpr std::string_view sie_comparison = "jamforelse-sie";
inline constexpr std::string_view design_insufficiency = "inte-an";
inline constexpr std::string_view error = pyroceph::tui::error_help_topic;
} // namespace help_topic

// All of them, so that a test can check that the manual has each one
inline constexpr std::array all_help_topics{
    help_topic::root,    help_topic::workspace,           help_topic::add_project, help_topic::open_existing,
    help_topic::new_project, help_topic::from_sie, help_topic::confirm_sie_project, help_topic::from_details,
    help_topic::details_name, help_topic::details_orgnr, help_topic::details_year, help_topic::details_months, help_topic::project, help_topic::forget_project, help_topic::fiscal_year, help_topic::export_sie,
    help_topic::sie_difference, help_topic::sie_comparison, help_topic::design_insufficiency,
    help_topic::error,
};

} // namespace cratchit::ui
