#pragma once

#include "core/charset/unicode.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <vector>

namespace cratchit {

// The operations of a prompt, each picked by its own key 1-9. A number belongs to its operation
// for good: an operation not offered now leaves a gap, so the others keep their numbers.
// The prompt's other keys (back, delete, help ...) are not listed; they are named in one line
// when an unknown key is pressed.
template <typename Id>
class Operations {
public:
  // Offer the operation at key number (1-9); two operations of one prompt may share a number
  // when only one of them is ever offered (e.g. close / open)
  Operations& add(std::size_t number, std::string label, Id id, bool offered = true) {
    if (offered) {
      operations_.push_back({number, std::move(label), id});
    }
    return *this;
  }

  // A key that is not a number, e.g. ("-", "back"), named in the one-liner in the order added
  Operations& other(std::string key, std::string word) {
    others_.push_back(std::move(key) + " " + std::move(word));
    return *this;
  }

  // The operation of a number key, if it is offered
  std::optional<Id> on_key(charset::unicode::CodePoint key) const {
    auto const found = std::ranges::find_if(operations_, [&](Operation const& operation) {
      return key == U'0' + static_cast<charset::unicode::CodePoint>(operation.number);
    });
    if (found == operations_.end()) {
      return std::nullopt;
    }
    return found->id;
  }

  // One line per operation offered, in the order added: "  1  Label"
  void show(std::ostream& out) const {
    for (auto const& operation : operations_) {
      std::println(out, "  {}  {}", operation.number, operation.label);
    }
  }

  // The other keys: "Other keys: - back, h help, q quit."
  std::string one_liner() const {
    std::string line = "Other keys:";
    for (std::size_t i = 0; i < others_.size(); ++i) {
      line += (i == 0 ? " " : ", ") + others_[i];
    }
    return line + ".";
  }

private:
  struct Operation {
    std::size_t number;
    std::string label;
    Id id;
  };
  std::vector<Operation> operations_;
  std::vector<std::string> others_;
};

} // namespace cratchit
