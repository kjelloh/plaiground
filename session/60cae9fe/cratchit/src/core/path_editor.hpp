#pragma once

#include <ostream>
#include <string>

namespace cratchit {

// Types a folder path key by key in cratchit's own buffer, as the terminal is in
// single-key mode. Tab completes folder names like a shell. Writes its own echo.
class PathEditor {
public:
  enum class Result { Typing, Accepted, Cancelled };

  // Start over with text in the buffer, e.g. the root folder
  void start(std::string text);

  // Enter accepts the text, or cancels when it is empty
  Result on_key(char key, std::ostream& out);

  std::string const& text() const;

  // "path> " followed by the text so far
  std::string prompt() const;

private:
  enum class Escape { None, Started, Sequence };

  std::string text_;
  Escape escape_ = Escape::None;

  void erase(std::ostream& out);
  void complete(std::ostream& out);
};

} // namespace cratchit
