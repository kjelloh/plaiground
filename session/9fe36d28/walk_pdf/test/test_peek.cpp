#include "peek.hpp"

#include <gtest/gtest.h>

#include <string>

using walk_pdf::Document;
using walk_pdf::Path;

namespace {

std::string peek_at(Document const& document, std::string_view path, std::size_t width = 60) {
  auto const content = document.find(*Path::parse(path));
  EXPECT_TRUE(content) << path;
  return content ? walk_pdf::peek(*content, width) : std::string{};
}

std::string const sample =
    "%PDF-1.3\n"
    "%\xc4\xe5\xf2\xe5\n"
    "% made  by\thand\n"
    "1 0 obj\n<< /Type /Page\n   /Parent 2 0 R >>\nendobj\n"
    "2 0 obj\n<< /Length 10 /Filter /FlateDecode >>\nstream\nendobj xyz\nendstream\nendobj\n"
    "3 0 obj\n<< /Filter [ /ASCII85Decode /FlateDecode ] >>\nstream\r\nab\r\nendstream\nendobj\n"
    "4 0 obj\n<< /Length 3 >>\nstream\nabc\nendstream\nendobj\n"
    "5 0 obj\n(Malm\xf6)\nendobj\n"
    "6 0 obj\n42\nendobj\n"
    "xref\n0 7\n0000000000 65535 f \n0000000000 00000 n \n0000000000 00000 n \n"
    "0000000000 00000 n \n0000000000 00000 n \n0000000000 00000 n \n0000000000 00000 n \n"
    "trailer\n<< /Size 7\n/Root 1 0 R >>\n"
    "startxref\n123\n"
    "%%EOF\n";

} // namespace

TEST(Peek, Header) { EXPECT_EQ(peek_at(Document{sample}, "/rev[0]/header"), "version 1.3"); }

TEST(Peek, BinaryMarkerComment) {
  EXPECT_EQ(peek_at(Document{sample}, "/rev[0]/comment[0]"), "<binary marker, 4 bytes>");
}

TEST(Peek, TextCommentWithWhitespaceSqueezed) {
  EXPECT_EQ(peek_at(Document{sample}, "/rev[0]/comment[1]"), "made by hand");
}

TEST(Peek, ObjectShowsBodyWithoutKeywords) {
  EXPECT_EQ(peek_at(Document{sample}, "/rev[0]/obj[1]"), "<< /Type /Page /Parent 2 0 R >>");
  EXPECT_EQ(peek_at(Document{sample}, "/rev[0]/obj[6]"), "42");
}

TEST(Peek, StreamIsDescribed) {
  EXPECT_EQ(peek_at(Document{sample}, "/rev[0]/obj[2]", 100),
            "<< /Length 10 /Filter /FlateDecode >> + stream <10 bytes, FlateDecode>");
  EXPECT_EQ(peek_at(Document{sample}, "/rev[0]/obj[3]", 100),
            "<< /Filter [ /ASCII85Decode /FlateDecode ] >> + stream <2 bytes, ASCII85Decode "
            "FlateDecode>");
  EXPECT_EQ(peek_at(Document{sample}, "/rev[0]/obj[4]"), "<< /Length 3 >> + stream <3 bytes>");
}

TEST(Peek, NonAsciiShownAsDot) {
  EXPECT_EQ(peek_at(Document{sample}, "/rev[0]/obj[5]"), "(Malm.)");
}

TEST(Peek, Xref) {
  EXPECT_EQ(peek_at(Document{sample}, "/rev[0]/xref[0]"), "7 entries, objects 0-6");
  Document const two{"%PDF-1.4\nxref\n0 1\n0000000000 65535 f \n5 2\n0000000009 00000 n \n"
                     "0000000009 00000 n \ntrailer\n<<>>\n%%EOF\n"};
  EXPECT_EQ(peek_at(two, "/rev[0]/xref[0]"), "3 entries in 2 subsections");
}

TEST(Peek, TrailerStartxrefEof) {
  Document const document{sample};
  EXPECT_EQ(peek_at(document, "/rev[0]/trailer[0]"), "<< /Size 7 /Root 1 0 R >>");
  EXPECT_EQ(peek_at(document, "/rev[0]/startxref[0]"), "123");
  EXPECT_EQ(peek_at(document, "/rev[0]/eof[0]"), "");
}

TEST(Peek, RevisionAndDocument) {
  Document const document{sample};
  EXPECT_EQ(peek_at(document, "/rev[0]"), "13 elements: 6 objects, xref, trailer");
  EXPECT_EQ(walk_pdf::peek(document.root()), "1 revision");
}

TEST(Peek, LongTextIsCut) {
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Type /Annot /Contents (" +
                          std::string(100, 'x') + ") >>\nendobj\n%%EOF\n"};
  auto const text = peek_at(document, "/rev[0]/obj[1]", 30);
  EXPECT_EQ(text.size(), 30U);
  EXPECT_EQ(text, "<< /Type /Annot /Contents (...");
}

TEST(Peek, LongStreamDictionaryIsCutButDescriptionKept) {
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Type /XObject /Subtype /Image /Width 100 "
                          "/Height 100 /Filter /DCTDecode >>\nstream\nxyz\nendstream\nendobj\n"};
  EXPECT_EQ(peek_at(document, "/rev[0]/obj[1]", 50),
            "<< /Type /XObject... + stream <3 bytes, DCTDecode>");
  // The dictionary gets the width the description leaves, but at least 20
  EXPECT_EQ(peek_at(document, "/rev[0]/obj[1]", 10),
            "<< /Type /XObject... + stream <3 bytes, DCTDecode>");
}

TEST(Peek, SyntaxWithBinaryStringsIsShownAsText) {
  // e.g. an encrypted file's strings
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Title (\x01\x02\x03\x04\x05\x06\x07\x08) >>\n"
                          "endobj\n%%EOF\n"};
  EXPECT_EQ(peek_at(document, "/rev[0]/obj[1]"), "<< /Title (........) >>");
}

TEST(Peek, BinaryUnknownIsDescribed) {
  Document const document{"%PDF-1.4\n\x01\x02\x03\x04\x05garbage\n%%EOF\n"};
  EXPECT_EQ(peek_at(document, "/rev[0]/unknown[0]"), "<13 bytes, binary>");
}
