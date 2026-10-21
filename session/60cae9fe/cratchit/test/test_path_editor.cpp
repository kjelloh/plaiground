#include "core/path_editor.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>
#include <string>

using cratchit::PathEditor;

namespace {

PathEditor::Result type(PathEditor& editor, std::u32string const& keys, std::ostream& out) {
  auto result = PathEditor::Result::Typing;
  for (auto const key : keys) {
    result = editor.on_key(key, out);
  }
  return result;
}

} // namespace

TEST(PathEditor, EchoesTypedKeys) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  EXPECT_EQ(type(editor, U"books", out), PathEditor::Result::Typing);
  EXPECT_EQ(editor.text(), "/books");
  EXPECT_EQ(out.str(), "books");
  EXPECT_EQ(editor.prompt(), "path> /books");
}

TEST(PathEditor, EnterAccepts) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  EXPECT_EQ(type(editor, U"a\r", out), PathEditor::Result::Accepted);
  EXPECT_EQ(editor.text(), "/a");
}

TEST(PathEditor, EnterOnEmptyTextCancels) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  EXPECT_EQ(type(editor, U"\x7f\n", out), PathEditor::Result::Cancelled);
}

TEST(PathEditor, BackspaceErasesOneCharacter) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  type(editor, U"ab\x7f\b\b", out);
  EXPECT_EQ(editor.text(), "");
  EXPECT_EQ(out.str(), "ab\b \b\b \b\b \b");
}

TEST(PathEditor, BackspaceErasesAWholeNonAsciiCharacter) {
  PathEditor editor;
  editor.start("/b\xc3\xb6"); // "/bö"
  std::ostringstream out;
  type(editor, U"\x7f", out);
  EXPECT_EQ(editor.text(), "/b");
}

TEST(PathEditor, ArrowKeysAreDropped) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  type(editor, U"a\x1b[A\x1bOBb", out);
  EXPECT_EQ(editor.text(), "/ab");
}

TEST(PathEditor, OtherControlKeysAreDropped) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  type(editor, std::u32string(U"a\0\x01" "b", 4), out);
  EXPECT_EQ(editor.text(), "/ab");
}

TEST(PathEditor, TabWithoutMatchRingsTheBell) {
  PathEditor editor;
  editor.start((std::filesystem::temp_directory_path() / "cratchit_no_such_folder_x").string());
  std::ostringstream out;
  type(editor, U"\t", out);
  EXPECT_EQ(out.str(), "\a");
}

TEST(PathEditor, KeepsNonAsciiCharactersAsUtf8) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  type(editor, U"Räk", out);
  EXPECT_EQ(editor.text(), "/R\xc3\xa4k");
  EXPECT_EQ(out.str(), "R\xc3\xa4k");
}

TEST(PathEditor, C1ControlsAndKeysThatDidNotDecodeAreDropped) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  type(editor, U"a\x85\xFFFD" "b", out);
  EXPECT_EQ(editor.text(), "/ab");
}
