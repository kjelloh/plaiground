#include "core/terminal.hpp"

#include <cstdio>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace cratchit {

#ifdef _WIN32

// _getch() already reads one key without echo, so there is no mode to set
RawTerminal::RawTerminal() : is_raw_(_isatty(_fileno(stdin)) != 0) {}

RawTerminal::~RawTerminal() = default;

std::optional<char> RawTerminal::read_key() {
  if (!is_raw_) {
    int c = std::getchar();
    return c == EOF ? std::nullopt : std::optional<char>(static_cast<char>(c));
  }
  int c = _getch();
  if (c == 0 || c == 0xE0) {
    // Function and arrow keys come as two codes; skip both
    _getch();
    return '\0';
  }
  if (c == 0x1A) {
    // Ctrl-Z is end of input on Windows
    return std::nullopt;
  }
  return static_cast<char>(c);
}

#else

namespace {
termios saved_mode{};
}

RawTerminal::RawTerminal() {
  if (isatty(STDIN_FILENO) == 0 || tcgetattr(STDIN_FILENO, &saved_mode) != 0) {
    return;
  }
  termios raw = saved_mode;
  // Keep ISIG so Ctrl-C still interrupts
  raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  is_raw_ = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
}

RawTerminal::~RawTerminal() {
  if (is_raw_) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_mode);
  }
}

std::optional<char> RawTerminal::read_key() {
  char c{};
  if (read(STDIN_FILENO, &c, 1) != 1) {
    return std::nullopt;
  }
  return c;
}

#endif

} // namespace cratchit
