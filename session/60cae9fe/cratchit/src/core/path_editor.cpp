#include "core/path_editor.hpp"

#include "core/workspace.hpp"

#include <print>

namespace cratchit {

namespace {

constexpr char escape = '\x1b';
constexpr char backspace = '\x7f'; // what macOS and Linux terminals send
constexpr char ctrl_h = '\b';      // what Windows sends

bool is_utf8_continuation(char c) {
  return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

} // namespace

void PathEditor::start(std::string text) {
  text_ = std::move(text);
  escape_ = Escape::None;
}

PathEditor::Result PathEditor::on_key(char key, std::ostream& out) {
  // Arrow and function keys send Esc, then [ or O, then codes up to a final letter; drop them
  if (escape_ == Escape::Started) {
    escape_ = key == '[' || key == 'O' ? Escape::Sequence : Escape::None;
    if (escape_ == Escape::Sequence) {
      return Result::Typing;
    }
  } else if (escape_ == Escape::Sequence) {
    if (key >= '@' && key <= '~') {
      escape_ = Escape::None;
    }
    return Result::Typing;
  }
  if (key == escape) {
    escape_ = Escape::Started;
    return Result::Typing;
  }
  if (key == '\r' || key == '\n') {
    std::println(out, "");
    return text_.empty() ? Result::Cancelled : Result::Accepted;
  }
  if (key == '\t') {
    complete(out);
  } else if (key == backspace || key == ctrl_h) {
    erase(out);
  } else if (static_cast<unsigned char>(key) >= 0x20) {
    // Bytes of a non-ASCII character (UTF-8) come one at a time; the terminal shows them once all are written
    text_ += key;
    out << key;
  }
  return Result::Typing;
}

std::string const& PathEditor::text() const {
  return text_;
}

std::string PathEditor::prompt() const {
  return "path> " + text_;
}

void PathEditor::erase(std::ostream& out) {
  if (text_.empty()) {
    return;
  }
  while (text_.size() > 1 && is_utf8_continuation(text_.back())) {
    text_.pop_back();
  }
  text_.pop_back();
  out << "\b \b";
}

void PathEditor::complete(std::ostream& out) {
  auto const completion = complete_folder(text_);
  if (!completion.added.empty()) {
    text_ += completion.added;
    out << completion.added;
  } else if (!completion.candidates.empty()) {
    std::println(out, "");
    for (auto const& name : completion.candidates) {
      std::println(out, "  {}", name);
    }
    out << prompt();
  } else {
    out << '\a';
  }
}

} // namespace cratchit
