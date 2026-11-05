#include "core/orchestrator.hpp"

namespace pyroceph::core {

KeyResult Orchestrator::on_key(charset::unicode::CodePoint key) {
  return key == U'q' ? KeyResult::Quit : KeyResult::Continue;
}

} // namespace pyroceph::core
