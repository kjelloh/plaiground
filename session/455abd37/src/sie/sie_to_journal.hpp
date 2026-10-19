#pragma once

// Builds Books from generic SIE records.
// Every record the SIE spec marks 'use = read' must be handled here;
// tests/specs_test.cpp checks that handled_labels() matches the spec.

#include "core/books.hpp"
#include "sie/sie_reader.hpp"

#include <expected>
#include <set>
#include <string>
#include <vector>

namespace cratchit::sie {

std::expected<Books, std::string> to_books(std::vector<Record> const& records);

std::set<std::string> const& handled_labels();

} // namespace cratchit::sie
