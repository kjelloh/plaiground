#include "core/orchestrator.hpp"
#include "tui/key_loop.hpp"

int main() {
  pyroceph::core::Orchestrator orchestrator;
  pyroceph::tui::KeyLoop key_loop{orchestrator};
  return key_loop.run();
}
