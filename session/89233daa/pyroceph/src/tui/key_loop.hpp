#pragma once

#include "core/orchestrator.hpp"

namespace pyroceph::tui {

// The console front end: reads one key at a time, shows it and hands it to the
// Orchestrator, until the Orchestrator says quit or the input ends.
class KeyLoop {
public:
  explicit KeyLoop(core::Orchestrator& orchestrator);

  // Returns the process exit code
  int run();

private:
  core::Orchestrator& orchestrator_;
};

} // namespace pyroceph::tui
