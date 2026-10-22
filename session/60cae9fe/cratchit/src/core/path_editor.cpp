#include "core/path_editor.hpp"

#include "core/charset/utf8.hpp"
#include "core/workspace.hpp"

#include <print>

namespace cratchit {

namespace {

constexpr charset::unicode::CodePoint escape = U'\x1b';
constexpr charset::unicode::CodePoint backspace = U'\x7f'; // what macOS and Linux terminals send
constexpr charset::unicode::CodePoint ctrl_h = U'\b';      // what Windows sends

// A key that types a character: not a control character (C0, DEL, C1), nor a key that did not decode
bool is_printable(charset::unicode::CodePoint key) {
  return key >= 0x20 && key != 0x7F && !(key >= 0x80 && key <= 0x9F) && key != charset::unicode::replacement_character;
}

bool is_utf8_continuation(char c) {
  return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

} // namespace

void PathEditor::start(std::string text, Kind kind) {
  text_ = std::move(text);
  kind_ = kind;
  escape_ = Escape::None;
}

PathEditor::Result PathEditor::on_key(charset::unicode::CodePoint key, std::ostream& out) {
  // Arrow and function keys send Esc, then [ or O, then codes up to a final letter; drop them
  if (escape_ == Escape::Started) {
    escape_ = key == U'[' || key == U'O' ? Escape::Sequence : Escape::None;
    if (escape_ == Escape::Sequence) {
      return Result::Typing;
    }
  } else if (escape_ == Escape::Sequence) {
    if (key >= U'@' && key <= U'~') {
      escape_ = Escape::None;
    }
    return Result::Typing;
  }
  if (key == escape) {
    escape_ = Escape::Started;
    return Result::Typing;
  }
  if (key == U'\r' || key == U'\n') {
    std::println(out, "");
    return text_.empty() ? Result::Cancelled : Result::Accepted;
  }
  if (key == U'\t') {
    if (kind_ == Kind::Path || kind_ == Kind::File) {
      complete(out);
    } else {
      out << '\a';
    }
  } else if (key == backspace || key == ctrl_h) {
    erase(out);
  } else if (is_printable(key)) {
    // The text is kept as UTF-8, which is also what the terminal is written in
    for (auto const byte : charset::utf8::FromUnicodeBuffer{}.push(key)) {
      text_ += static_cast<char>(byte);
      out << static_cast<char>(byte);
    }
  }
  return Result::Typing;
}

std::string const& PathEditor::text() const {
  return text_;
}

std::string PathEditor::prompt() const {
  switch (kind_) {
  case Kind::Name:
    return "name> " + text_;
  case Kind::Account:
    return "account> " + text_;
  default:
    return "path> " + text_;
  }
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
  auto const completion = complete_folder(text_, kind_ == Kind::File);
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
