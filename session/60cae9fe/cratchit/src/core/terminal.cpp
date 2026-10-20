#include "core/terminal.hpp"

#include <cstdio>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
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

std::size_t page_lines() {
  CONSOLE_SCREEN_BUFFER_INFO info{};
  if (_isatty(_fileno(stdout)) == 0 || GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info) == 0) {
    return 0;
  }
  auto rows = info.srWindow.Bottom - info.srWindow.Top + 1;
  return rows > 1 ? static_cast<std::size_t>(rows - 1) : 0;
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

std::size_t page_lines() {
  winsize size{};
  if (isatty(STDOUT_FILENO) == 0 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0) {
    return 0;
  }
  return size.ws_row > 1 ? static_cast<std::size_t>(size.ws_row - 1) : 0;
}

#endif

} // namespace cratchit
