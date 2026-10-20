#include "core/path_editor.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>
#include <string>

using cratchit::PathEditor;

namespace {

PathEditor::Result type(PathEditor& editor, std::string const& keys, std::ostream& out) {
  auto result = PathEditor::Result::Typing;
  for (char key : keys) {
    result = editor.on_key(key, out);
  }
  return result;
}

} // namespace

TEST(PathEditor, EchoesTypedKeys) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  EXPECT_EQ(type(editor, "books", out), PathEditor::Result::Typing);
  EXPECT_EQ(editor.text(), "/books");
  EXPECT_EQ(out.str(), "books");
  EXPECT_EQ(editor.prompt(), "path> /books");
}

TEST(PathEditor, EnterAccepts) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  EXPECT_EQ(type(editor, "a\r", out), PathEditor::Result::Accepted);
  EXPECT_EQ(editor.text(), "/a");
}

TEST(PathEditor, EnterOnEmptyTextCancels) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  EXPECT_EQ(type(editor, "\x7f\n", out), PathEditor::Result::Cancelled);
}

TEST(PathEditor, BackspaceErasesOneCharacter) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  type(editor, "ab\x7f\b\b", out);
  EXPECT_EQ(editor.text(), "");
  EXPECT_EQ(out.str(), "ab\b \b\b \b\b \b");
}

TEST(PathEditor, BackspaceErasesAWholeNonAsciiCharacter) {
  PathEditor editor;
  editor.start("/b\xc3\xb6"); // "/bö"
  std::ostringstream out;
  type(editor, "\x7f", out);
  EXPECT_EQ(editor.text(), "/b");
}

TEST(PathEditor, ArrowKeysAreDropped) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  type(editor, "a\x1b[A\x1bOBb", out);
  EXPECT_EQ(editor.text(), "/ab");
}

TEST(PathEditor, OtherControlKeysAreDropped) {
  PathEditor editor;
  editor.start("/");
  std::ostringstream out;
  type(editor, std::string("a\0\x01" "b", 4), out);
  EXPECT_EQ(editor.text(), "/ab");
}

TEST(PathEditor, TabWithoutMatchRingsTheBell) {
  PathEditor editor;
  editor.start((std::filesystem::temp_directory_path() / "cratchit_no_such_folder_x").string());
  std::ostringstream out;
  type(editor, "\t", out);
  EXPECT_EQ(out.str(), "\a");
}
