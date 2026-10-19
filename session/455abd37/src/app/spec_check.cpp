#include "app/spec_check.hpp"

#include "core/text.hpp"
#include "cspec/spec_tree.hpp"
#include "legal/compliance.hpp"
#include "reports/report.hpp"
#include "sie/sie_reader.hpp"
#include "sie/sie_to_journal.hpp"

#include <algorithm>
#include <set>

namespace cratchit {

namespace {

using Problems = std::vector<std::string>;

bool one_of(std::string const& value, std::initializer_list<std::string_view> allowed) {
  return std::ranges::find(allowed, value) != allowed.end();
}

void check_header(cspec::SpecTree const& tree, Problems& problems) {
  for (auto const* path : {"spec.id", "spec.kind", "spec.version", "spec.status", "spec.title"}) {
    if (!tree.value(path)) problems.push_back(tree.source() + ": missing " + path);
  }
  auto const kind = tree.value_or("spec.kind", "");
  if (!one_of(kind, {"format", "mapping", "rules", "registry", "legal", "settings", "schema"})) {
    problems.push_back(tree.where("spec.kind") + ": unknown spec.kind \"" + kind + "\"");
  }
  auto const status = tree.value_or("spec.status", "");
  if (!one_of(status, {"draft", "checked", "verified"})) {
    problems.push_back(tree.where("spec.status") + ": unknown spec.status \"" + status + "\"");
  }
}

void check_registry(std::filesystem::path const& root, cspec::SpecTree const& registry, cspec::SpecTree const& legal, Problems& problems) {
  for (auto const& id : registry.children("interface")) {
    auto const base = "interface." + id + ".";
    for (auto const* field : {"title", "direction", "status", "milestone"}) {
      if (!registry.value(base + field)) problems.push_back(registry.source() + ": interface " + id + " has no " + field);
    }
    if (!one_of(registry.value_or(base + "direction", ""), {"in", "out", "internal"})) {
      problems.push_back(registry.where(base + "direction") + ": direction must be in, out or internal");
    }
    auto const status = registry.value_or(base + "status", "");
    if (!one_of(status, {"skeleton", "planned", "future", "listed"})) {
      problems.push_back(registry.where(base + "status") + ": unknown status \"" + status + "\"");
    }
    for (auto const& spec : text::split_list(registry.value_or(base + "spec", ""))) {
      if (!std::filesystem::exists(root / "specs" / spec)) problems.push_back(registry.where(base + "spec") + ": no file specs/" + spec);
    }
    auto const code = text::split_list(registry.value_or(base + "code", ""));
    if (status == "skeleton" && code.empty()) problems.push_back(registry.source() + ": interface " + id + " is skeleton but lists no code");
    for (auto const& file : code) {
      if (!std::filesystem::exists(root / file)) problems.push_back(registry.where(base + "code") + ": no file " + file);
    }
    for (auto const& requirement : text::split_list(registry.value_or(base + "legal", ""))) {
      if (!legal.contains("requirement." + requirement)) {
        problems.push_back(registry.where(base + "legal") + ": unknown requirement " + requirement);
      }
    }
  }
}

void check_legal(cspec::SpecTree const& legal, Problems& problems) {
  for (auto const& id : legal.children("requirement")) {
    auto const path = "requirement." + id + ".mechanism";
    for (auto const& mechanism : text::split_list(legal.value_or(path, ""))) {
      if (!legal.contains("mechanism." + mechanism)) problems.push_back(legal.where(path) + ": unknown mechanism " + mechanism);
    }
  }
  std::set<std::string> in_code;
  for (auto const& mechanism : legal::mechanisms()) {
    in_code.emplace(mechanism.m_id);
    auto const path = "mechanism." + std::string{mechanism.m_id} + ".status";
    auto const status = legal.value_or(path, "<missing>");
    if (status != mechanism.m_status) {
      problems.push_back(legal.where(path) + ": spec says " + status + ", code says " + std::string{mechanism.m_status});
    }
  }
  for (auto const& id : legal.children("mechanism")) {
    auto const status = legal.value_or("mechanism." + id + ".status", "");
    if (status != "planned" && !in_code.contains(id)) {
      problems.push_back(legal.where("mechanism." + id + ".status") + ": " + id + " is " + status + " but not in legal::mechanisms()");
    }
  }
}

void check_sie(cspec::SpecTree const& sie_tree, Problems& problems) {
  auto const spec = sie::SieSpec::from(sie_tree);
  if (!spec) {
    problems.push_back(spec.error());
    return;
  }
  std::set<std::string> read_labels;
  for (auto const& [label, record] : spec->records()) {
    if (record.m_use == "read" || record.m_use == "read-write") read_labels.insert(label);
  }
  if (read_labels != sie::handled_labels()) {
    problems.push_back(sie_tree.source() + ": the records marked 'use = read' differ from what sie_to_journal handles");
  }
}

void check_reports(cspec::SpecTree const& registry, Problems& problems) {
  for (auto const& report : reports::make_report_generators()) {
    auto const base = "interface." + std::string{report->interface_id()} + ".";
    if (!registry.contains("interface." + std::string{report->interface_id()})) {
      problems.push_back("report " + std::string{report->id()} + ": unknown interface " + std::string{report->interface_id()});
      continue;
    }
    auto const status = registry.value_or(base + "status", "");
    bool const registry_says_implemented = status == "skeleton";
    if (registry_says_implemented != report->implemented()) {
      problems.push_back(registry.where(base + "status") + ": report " + std::string{report->id()} + " implemented() disagrees with status " + status);
    }
  }
}

} // namespace

std::vector<std::string> check_specs(std::filesystem::path const& root) {
  Problems problems;
  auto const specs_dir = root / "specs";
  if (!std::filesystem::is_directory(specs_dir)) return {"no specs folder at " + specs_dir.string()};

  for (auto const& entry : std::filesystem::recursive_directory_iterator(specs_dir)) {
    if (entry.path().extension() != ".cspec") continue;
    auto const tree = cspec::SpecTree::load(entry.path());
    if (!tree) {
      problems.push_back(tree.error());
      continue;
    }
    check_header(*tree, problems);
  }

  auto const registry = cspec::SpecTree::load(specs_dir / "interfaces.cspec");
  auto const legal = cspec::SpecTree::load(specs_dir / "legal" / "requirements.cspec");
  auto const sie = cspec::SpecTree::load(specs_dir / "sie" / "sie4.cspec");
  if (!registry || !legal || !sie) {
    problems.push_back("cannot load interfaces.cspec, legal/requirements.cspec or sie/sie4.cspec");
    return problems;
  }
  check_registry(root, *registry, *legal, problems);
  check_legal(*legal, problems);
  check_sie(*sie, problems);
  check_reports(*registry, problems);
  return problems;
}

} // namespace cratchit
