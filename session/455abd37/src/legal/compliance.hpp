#pragma once

// Code side of specs/legal/requirements.cspec: which adherence mechanisms exist,
// and the checks that are implemented.

#include "core/books.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::legal {

struct Mechanism {
  std::string_view m_id;
  std::string_view m_status; // implemented | partial (planned ones are only in the spec)
};

// Must agree with the mechanism.*.status entries in the spec (checked by tests).
std::span<Mechanism const> mechanisms();

struct Finding {
  std::string m_mechanism;
  std::string m_message;
};

std::vector<Finding> check_voucher(Voucher const& voucher);
std::vector<Finding> check_books(Books const& books);

} // namespace cratchit::legal
