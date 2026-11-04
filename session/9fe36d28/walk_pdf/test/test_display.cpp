#include "document.hpp"
#include "peek.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using walk_pdf::Document;
using walk_pdf::Kind;
using walk_pdf::Path;

namespace {

std::string stream_object(int number, std::string_view dictionary, std::string_view data) {
  return std::to_string(number) + " 0 obj\n<< " + std::string{dictionary} + " /Length " +
         std::to_string(data.size()) + " >>\nstream\n" + std::string{data} + "\nendstream\nendobj\n";
}

// Catalog 1 -> pages 2 -> [pages 3 -> [page 5], page 4]: page order 5, 4.
// Page 5 draws streams 6 and 7, and form 8 (which draws form 9, and itself).
// Fonts: F1 = 10 (WinAnsi) from pages 2; the form 8 has its own F2 = 11.
std::string const pdf =
    "%PDF-1.4\n"
    "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
    "2 0 obj\n<< /Type /Pages /Kids [3 0 R 4 0 R] /Count 2"
    " /Resources << /Font << /F1 10 0 R >> /XObject << /Fm 8 0 R /Im 12 0 R >> >> >>\nendobj\n"
    "3 0 obj\n<< /Type /Pages /Parent 2 0 R /Kids [5 0 R] /Count 1 >>\nendobj\n"
    "4 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 7 0 R >>\nendobj\n"
    "5 0 obj\n<< /Type /Page /Parent 3 0 R /Contents [6 0 R 7 0 R] >>\nendobj\n" +
    stream_object(6, "", "BT /F1 12 Tf (one) Tj ET") +
    stream_object(7, "", "BT (two) Tj ET /Fm Do /Im Do") +
    stream_object(8, "/Type /XObject /Subtype /Form /Resources << /Font << /F2 11 0 R >> "
                     "/XObject << /Self 8 0 R >> >>",
                  "BT (inherited) Tj /F2 9 Tf (own) Tj ET /Self Do") +
    "10 0 obj\n<< /Type /Font /Subtype /Type1 /Encoding /WinAnsiEncoding >>\nendobj\n"
    "11 0 obj\n<< /Type /Font /Subtype /Type1 /Encoding << /Differences [111 /odieresis] >> >>\nendobj\n" +
    stream_object(12, "/Type /XObject /Subtype /Image /Width 1 /Height 1", "x") +
    "trailer\n<< /Size 13 /Root 1 0 R >>\n%%EOF\n";

Path path(std::string_view text) { return *Path::parse(text); }

std::vector<std::string> paths_of(Document const& document, std::string_view parent) {
  std::vector<std::string> result;
  for (auto const& child : document.children(path(parent))) {
    result.push_back(child.path.to_string());
  }
  return result;
}

std::string text_at(Document const& document, std::string_view at) {
  return document.text(path(at)).value_or("<none>");
}

} // namespace

TEST(Display, PagesInPageTreeOrder) {
  Document const document{pdf};
  ASSERT_EQ(document.pages().size(), 2U);
  EXPECT_EQ(document.pages()[0], path("/rev[0]/obj[5]"));
  EXPECT_EQ(document.pages()[1], path("/rev[0]/obj[4]"));
  EXPECT_EQ(paths_of(document, "/pages"), (std::vector<std::string>{"/pages/page[0]", "/pages/page[1]"}));
  EXPECT_EQ(document.find(path("/pages/page[0]"))->kind(), Kind::page);
  EXPECT_FALSE(document.find(path("/pages/page[2]")));
}

TEST(Display, PagesAreNotListedUnderTheRoot) {
  Document const document{pdf};
  EXPECT_EQ(paths_of(document, "/"), (std::vector<std::string>{"/rev[0]"}));
  EXPECT_EQ(walk_pdf::peek(document.root()), "1 revision; 2 pages (/pages)");
}

TEST(Display, OperationsOfAllContentStreamsInOrder) {
  Document const document{pdf};
  std::vector<std::string> operations;
  for (auto const& child : document.children(path("/pages/page[0]"))) {
    operations.emplace_back(child.content.bytes());
  }
  EXPECT_EQ(operations, (std::vector<std::string>{"BT", "/F1 12 Tf", "(one) Tj", "ET", "BT", "(two) Tj", "ET",
                                                  "/Fm Do", "/Im Do"}));
}

TEST(Display, FontCarriesAcrossContentStreams) {
  // Stream 7's "(two) Tj" uses F1, selected in stream 6
  Document const document{pdf};
  EXPECT_EQ(text_at(document, "/pages/page[0]/op[5]"), "two");
  EXPECT_EQ(text_at(document, "/pages/page[1]/op[1]"), "<none>"); // page 1 draws stream 7 alone: no font
}

TEST(Display, FormDrawnByDo) {
  Document const document{pdf};
  // /Fm Do: its operand, then the form's operations
  EXPECT_EQ(paths_of(document, "/pages/page[0]/op[7]"),
            (std::vector<std::string>{"/pages/page[0]/op[7]/[0]", "/pages/page[0]/op[7]/op[0]",
                                      "/pages/page[0]/op[7]/op[1]", "/pages/page[0]/op[7]/op[2]",
                                      "/pages/page[0]/op[7]/op[3]", "/pages/page[0]/op[7]/op[4]",
                                      "/pages/page[0]/op[7]/op[5]"}));
  EXPECT_EQ(document.resources(path("/pages/page[0]/op[7]/op[1]")), path("/rev[0]/obj[8]/Resources"));
  // F1 in effect where the form is drawn; then the form's own F2 (o is ö there)
  EXPECT_EQ(text_at(document, "/pages/page[0]/op[7]/op[1]"), "inherited");
  EXPECT_EQ(text_at(document, "/pages/page[0]/op[7]/op[3]"), "\xC3\xB6wn");
}

TEST(Display, ImageDrawnByDoHasOnlyItsOperand) {
  Document const document{pdf};
  EXPECT_EQ(paths_of(document, "/pages/page[0]/op[8]"), (std::vector<std::string>{"/pages/page[0]/op[8]/[0]"}));
}

TEST(Display, FormDrawingItselfStops) {
  Document const document{pdf};
  // /Self Do in form 8 draws form 8 again, and again, until the depth limit
  std::string at = "/pages/page[0]/op[7]";
  int depth = 1;
  while (document.find(path(at + "/op[5]"))) {
    at += "/op[5]";
    ++depth;
  }
  EXPECT_GT(depth, 2);
  EXPECT_LE(depth, 10);
}

TEST(Display, PageWithoutCatalog) {
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Type /Page >>\nendobj\n%%EOF\n"};
  EXPECT_TRUE(document.pages().empty());
  EXPECT_TRUE(paths_of(document, "/pages").empty());
}
