#include "document.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using walk_pdf::Document;
using walk_pdf::Kind;
using walk_pdf::Path;

namespace {

// A small PDF with one appended update, so two revisions. Object 1 is in both.
std::string const two_revisions =
    "%PDF-1.7\n"
    "%\xe2\xe3\xcf\xd3\n"
    "1 0 obj\n<< /Type /Catalog >>\nendobj\n"
    "2 0 obj\n<< /Length 10 >>\nstream\nendobj xyz\nendstream\nendobj\n"
    "xref\n0 3\n0000000000 65535 f \n"
    "trailer\n<< /Size 3 /Root 1 0 R /Info << /X 1 >> >>\n"
    "startxref\n123\n"
    "%%EOF\n"
    "1 0 obj\n<< /Type /Catalog /Changed true >>\nendobj\n"
    "xref\n0 1\n"
    "trailer\n<< /Size 3 /Prev 123 >>\n"
    "startxref\n456\n"
    "%%EOF\n";

std::vector<std::string> paths_of(walk_pdf::Content const& content) {
  std::vector<std::string> paths;
  for (auto const& [path, child] : content) {
    paths.push_back(path.to_string());
  }
  return paths;
}

} // namespace

TEST(Document, RootChildrenAreRevisions) {
  Document const document{two_revisions};
  EXPECT_EQ(paths_of(document.root()), (std::vector<std::string>{"/rev[0]", "/rev[1]"}));
}

TEST(Document, RevisionChildren) {
  Document const document{two_revisions};
  auto const first = document.find(*Path::parse("/rev[0]"));
  ASSERT_TRUE(first);
  EXPECT_EQ(paths_of(*first),
            (std::vector<std::string>{"/rev[0]/header", "/rev[0]/comment[0]", "/rev[0]/obj[1]",
                                      "/rev[0]/obj[2]", "/rev[0]/xref[0]", "/rev[0]/trailer[0]",
                                      "/rev[0]/startxref[0]", "/rev[0]/eof[0]"}));
  auto const second = document.find(*Path::parse("/rev[1]"));
  ASSERT_TRUE(second);
  EXPECT_EQ(paths_of(*second),
            (std::vector<std::string>{"/rev[1]/obj[1]", "/rev[1]/xref[0]", "/rev[1]/trailer[0]",
                                      "/rev[1]/startxref[0]", "/rev[1]/eof[0]"}));
}

TEST(Document, SameObjectNumberInEachRevision) {
  Document const document{two_revisions};
  auto const original = document.find(*Path::parse("/rev[0]/obj[1]"));
  auto const updated = document.find(*Path::parse("/rev[1]/obj[1]"));
  ASSERT_TRUE(original);
  ASSERT_TRUE(updated);
  EXPECT_EQ(original->bytes(), "1 0 obj\n<< /Type /Catalog >>\nendobj");
  EXPECT_EQ(updated->bytes(), "1 0 obj\n<< /Type /Catalog /Changed true >>\nendobj");
}

TEST(Document, StreamDataMayContainEndobj) {
  Document const document{two_revisions};
  auto const object = document.find(*Path::parse("/rev[0]/obj[2]"));
  ASSERT_TRUE(object);
  EXPECT_TRUE(object->bytes().ends_with("endstream\nendobj"));
}

TEST(Document, TrailerWithNestedDictionary) {
  Document const document{two_revisions};
  auto const trailer = document.find(*Path::parse("/rev[0]/trailer[0]"));
  ASSERT_TRUE(trailer);
  EXPECT_EQ(trailer->kind(), Kind::trailer);
  EXPECT_EQ(trailer->bytes(), "trailer\n<< /Size 3 /Root 1 0 R /Info << /X 1 >> >>");
}

TEST(Document, FoundContentKnowsItsPathAndOffset) {
  Document const document{two_revisions};
  auto const path = *Path::parse("/rev[0]/header");
  auto const header = document.find(path);
  ASSERT_TRUE(header);
  EXPECT_EQ(header->path(), path);
  EXPECT_EQ(header->offset(), 0U);
  EXPECT_EQ(header->bytes(), "%PDF-1.7");
}

TEST(Document, MissingPath) {
  Document const document{two_revisions};
  EXPECT_FALSE(document.find(*Path::parse("/rev[2]")));
  EXPECT_FALSE(document.find(*Path::parse("/rev[0]/obj[9]")));
  EXPECT_FALSE(document.find(*Path::parse("/rev[1]/header")));
  EXPECT_FALSE(document.find(*Path::parse("/rev[0]/obj[1]/obj[1]")));
}

TEST(Document, UnknownBytesAreGathered) {
  Document const document{"%PDF-1.4\nnot pdf\nstill not\n1 0 obj\nnull\nendobj\n%%EOF\n"};
  auto const unknown = document.find(*Path::parse("/rev[0]/unknown[0]"));
  ASSERT_TRUE(unknown);
  EXPECT_EQ(unknown->bytes(), "not pdf\nstill not\n");
  EXPECT_TRUE(document.find(*Path::parse("/rev[0]/obj[1]")));
}

TEST(Document, ObjectPathHasNoGeneration) {
  Document const document{"%PDF-1.4\n7 3 obj\nnull\nendobj\n%%EOF\n"};
  auto const object = document.find(*Path::parse("/rev[0]/obj[7]"));
  ASSERT_TRUE(object);
  EXPECT_EQ(object->bytes(), "7 3 obj\nnull\nendobj");
}

TEST(Document, DuplicateObjectNumberInARevisionFindsTheFirst) {
  // Only a damaged file defines an object number twice in one revision
  Document const document{"%PDF-1.4\n1 0 obj\n(a)\nendobj\n1 0 obj\n(b)\nendobj\n%%EOF\n"};
  auto const object = document.find(*Path::parse("/rev[0]/obj[1]"));
  ASSERT_TRUE(object);
  EXPECT_EQ(object->bytes(), "1 0 obj\n(a)\nendobj");
}

TEST(Document, EmptyFileHasNoRevisions) {
  Document const document{""};
  EXPECT_TRUE(paths_of(document.root()).empty());
  EXPECT_TRUE(document.find(Path{}));
}
