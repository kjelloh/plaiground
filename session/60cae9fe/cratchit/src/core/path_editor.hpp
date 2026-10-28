#pragma once

#include "core/charset/unicode.hpp"

#include <ostream>
#include <string>

namespace cratchit {

// Types a folder path, a file path, a name, an account number, a voucher line,
// an event line, a voucher row or an event heading, key by key in cratchit's own buffer, as the terminal is in
// single-key mode. In a path Tab completes folder names (and file names for a
// file) like a shell. Writes its own echo.
class PathEditor {
public:
  enum class Result { Typing, Accepted, Cancelled, Back, Hint };
  enum class Kind { Path, File, Name, Account, Voucher, Event, Row, Heading, Force };

  // Start over with text in the buffer, e.g. the root folder
  void start(std::string text, Kind kind = Kind::Path);

  // Enter accepts the text, or cancels when it is empty. On an empty buffer - is not typed but
  // goes back, and in a row ? gives a hint at once and stays typed, so an account can follow it.
  Result on_key(charset::unicode::CodePoint key, std::ostream& out);

  std::string const& text() const;

  // "path> ", "name> ", "account> ", "voucher> ", "event> ", "post> ", "heading> " or "force> " followed by the text
  // so far
  std::string prompt() const;

private:
  enum class Escape { None, Started, Sequence };

  std::string text_;
  Kind kind_ = Kind::Path;
  Escape escape_ = Escape::None;

  void erase(std::ostream& out);
  void complete(std::ostream& out);
};

} // namespace cratchit
