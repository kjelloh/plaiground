#pragma once

// Checks that the spec files and the code agree (cratchit check-specs, tests/specs_test.cpp):
//  - every specs/**/*.cspec parses and has a valid header
//  - every interface in specs/interfaces.cspec points to existing spec and code files
//    and to defined legal requirements
//  - legal mechanisms in the spec match legal::mechanisms() in code
//  - SIE records marked 'use = read' match what sie_to_journal handles
//  - report generators match their interfaces

#include <filesystem>
#include <string>
#include <vector>

namespace cratchit {

// 'root' is the folder that holds specs/ and src/. Returns one line per problem.
std::vector<std::string> check_specs(std::filesystem::path const& root);

} // namespace cratchit
