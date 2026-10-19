// cratchit command line (interface user_cli)

#include "app/settings.hpp"
#include "app/spec_check.hpp"
#include "core/text.hpp"
#include "cspec/spec_tree.hpp"
#include "reports/report.hpp"
#include "sie/sie_reader.hpp"
#include "sie/sie_to_journal.hpp"
#include "statements/csv_statement_reader.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace {

using namespace cratchit;

constexpr std::string_view usage = R"(usage: cratchit [--root DIR] <command> [options]

commands:
  interfaces                         list all interfaces and their status
  check-specs                        check that spec files and code agree
  reports                            list reports
  report <id> --period P --sie FILE [--sie FILE …] [--settings FILE] [--out DIR] [--note TEXT]
                                     generate a report, e.g. report vat --period 202409
  events <statement-spec> FILE       read an account statement, print financial events
                                     e.g. events nordea_csv nordea.csv

--root is the folder with specs/ (default: the source folder cratchit was built from).
)";

struct Options {
  std::filesystem::path m_root{CRATCHIT_SOURCE_DIR};
  std::vector<std::string> m_positional;
  std::multimap<std::string, std::string> m_named;

  std::string named(std::string const& name, std::string fallback = {}) const {
    auto const it = m_named.find(name);
    return it == m_named.end() ? fallback : it->second;
  }
};

std::optional<Options> parse_options(int argc, char** argv) {
  Options options;
  for (int i = 1; i < argc; ++i) {
    std::string const arg = argv[i];
    if (arg.starts_with("--")) {
      if (i + 1 == argc) return std::nullopt;
      auto const name = arg.substr(2);
      if (name == "root") options.m_root = argv[++i];
      else options.m_named.emplace(name, argv[++i]);
    } else {
      options.m_positional.push_back(arg);
    }
  }
  return options;
}

std::optional<std::string> read_file(std::filesystem::path const& path) {
  std::ifstream file{path, std::ios::binary};
  if (!file) return std::nullopt;
  std::ostringstream content;
  content << file.rdbuf();
  return content.str();
}

int fail(std::string const& message) {
  std::cerr << "cratchit: " << message << "\n";
  return 1;
}

int list_interfaces(Options const& options) {
  auto const registry = cspec::SpecTree::load(options.m_root / "specs" / "interfaces.cspec");
  if (!registry) return fail(registry.error());
  for (auto const& id : registry->children("interface")) {
    auto const base = "interface." + id + ".";
    std::cout << registry->value_or(base + "milestone", "?") << "  " << registry->value_or(base + "status", "?") << "\t"
              << registry->value_or(base + "direction", "?") << "\t" << id << "\n      " << registry->value_or(base + "title", "")
              << "\n";
  }
  return 0;
}

int run_check_specs(Options const& options) {
  auto const problems = check_specs(options.m_root);
  for (auto const& problem : problems) std::cout << problem << "\n";
  std::cout << (problems.empty() ? "specs and code agree\n" : std::to_string(problems.size()) + " problem(s)\n");
  return problems.empty() ? 0 : 1;
}

int list_reports() {
  for (auto const& report : reports::make_report_generators()) {
    std::cout << report->id() << "\t" << (report->implemented() ? "implemented" : "planned") << "\tinterface " << report->interface_id()
              << "\n";
  }
  return 0;
}

int run_report(Options const& options) {
  if (options.m_positional.size() < 2) return fail("report: which report?");
  auto const specs_dir = options.m_root / "specs";
  auto const settings_file = options.named("settings", ".cratchit/settings.cspec");
  auto const settings = Settings::load(specs_dir, std::filesystem::path{settings_file});
  if (!settings) return fail(settings.error());

  auto const sie_tree = cspec::SpecTree::load(specs_dir / "sie" / "sie4.cspec");
  if (!sie_tree) return fail(sie_tree.error());
  auto const sie_spec = sie::SieSpec::from(*sie_tree);
  if (!sie_spec) return fail(sie_spec.error());

  std::vector<Books> books;
  std::vector<std::filesystem::path> inputs;
  for (auto [it, end] = options.m_named.equal_range("sie"); it != end; ++it) {
    auto const records = sie::read_file(it->second, *sie_spec);
    if (!records) return fail(records.error());
    auto year = sie::to_books(*records);
    if (!year) return fail(it->second + ": " + year.error());
    books.push_back(std::move(*year));
    inputs.emplace_back(it->second);
  }

  for (auto const& report : reports::make_report_generators()) {
    if (report->id() != options.m_positional[1]) continue;
    reports::ReportRequest const request{books, inputs, options.named("period"), *settings, specs_dir, options.named("note")};
    auto const files = report->generate(request);
    if (!files) return fail(files.error());
    std::filesystem::path const out_dir = options.named("out", ".");
    std::filesystem::create_directories(out_dir);
    for (auto const& file : *files) {
      std::ofstream out{out_dir / file.m_name, std::ios::binary};
      out << file.m_content;
      if (!out) return fail("cannot write " + (out_dir / file.m_name).string());
      std::cout << "wrote " << (out_dir / file.m_name).string() << "\n";
    }
    return 0;
  }
  return fail("unknown report '" + options.m_positional[1] + "' (see: cratchit reports)");
}

int run_events(Options const& options) {
  if (options.m_positional.size() < 3) return fail("events: need <statement-spec> FILE");
  auto const spec_name = options.m_positional[1];
  auto const tree = cspec::SpecTree::load(options.m_root / "specs" / "statements" / (spec_name + ".cspec"));
  if (!tree) return fail(tree.error());
  auto const spec = statements::StatementSpec::from(*tree);
  if (!spec) return fail(spec.error());
  auto const settings = Settings::load(options.m_root / "specs", std::filesystem::path{options.named("settings", ".cratchit/settings.cspec")});
  if (!settings) return fail(settings.error());
  std::optional<AccountNo> account;
  if (!spec->m_account_setting.empty()) account = std::stoi(settings->get(spec->m_account_setting));

  auto const content = read_file(options.m_positional[2]);
  if (!content) return fail("cannot read " + options.m_positional[2]);
  auto const interface = spec_name == "skv_account_csv" ? "skv_account_statement_import" : "bank_statement_import";
  auto const events = statements::read_statement(*content, *spec, interface, options.m_positional[2], account);
  if (!events) return fail(events.error());
  std::cout << to_cspec(*events);
  return 0;
}

} // namespace

int main(int argc, char** argv) {
  auto const options = parse_options(argc, argv);
  if (!options || options->m_positional.empty()) {
    std::cout << usage;
    return options ? 0 : 1;
  }
  auto const& command = options->m_positional.front();
  if (command == "interfaces") return list_interfaces(*options);
  if (command == "check-specs") return run_check_specs(*options);
  if (command == "reports") return list_reports();
  if (command == "report") return run_report(*options);
  if (command == "events") return run_events(*options);
  std::cout << usage;
  return 1;
}
