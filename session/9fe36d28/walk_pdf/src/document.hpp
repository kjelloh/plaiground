#pragma once

#include "path.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace walk_pdf {

class Document;
class ChildIterator;

// What is at a path: its kind, its bytes, and its children.
// Content views bytes owned by its Document, which must outlive it.
// Looping over a Content gives its children as Elements.
class Content {
public:
  Path const& path() const { return path_; }
  Kind kind() const { return kind_; }
  std::size_t offset() const { return begin_; }
  std::string_view bytes() const;

  ChildIterator begin() const;
  std::default_sentinel_t end() const { return {}; }

private:
  friend class Document;
  friend class ChildIterator;
  Content(Document const& document, Path path, Kind kind, std::size_t begin, std::size_t end)
      : document_(&document), path_(std::move(path)), kind_(kind), begin_(begin), end_(end) {}

  Document const* document_;
  Path path_;
  Kind kind_;
  std::size_t begin_; // offset of the first byte in the file
  std::size_t end_;   // offset just after the last byte
};

// A child found while looping over a Content
struct Element {
  Path path;
  Content content;
};

// Walks the children of one Content, in file order
class ChildIterator {
public:
  using value_type = Element;
  using difference_type = std::ptrdiff_t;

  ChildIterator() = default;

  Element const& operator*() const { return *current_; }
  Element const* operator->() const { return &*current_; }
  ChildIterator& operator++();
  void operator++(int) { ++*this; }
  friend bool operator==(ChildIterator const& it, std::default_sentinel_t) {
    return !it.current_;
  }

private:
  friend class Content;
  explicit ChildIterator(Content const& parent);
  void find_next();

  std::optional<Content> parent_;
  std::size_t position_ = 0;                       // where the search for the next child starts
  std::array<std::uint64_t, 16> ordinals_{};       // children seen so far, per kind
  std::optional<Element> current_;                 // nullopt at the end
};

// One PDF file, and the lookup of paths in it
class Document {
public:
  explicit Document(std::string bytes);

  Document(Document const&) = delete;            // Content refers to the document
  Document& operator=(Document const&) = delete;

  std::string_view bytes() const { return bytes_; }

  // The whole file, path "/"
  Content root() const;

  // What the path refers to, or nullopt if the file has no such element
  std::optional<Content> find(Path const& path) const;

private:
  friend class ChildIterator;

  struct Range {
    std::size_t begin;
    std::size_t end;
  };

  std::string bytes_;
  std::vector<Range> revisions_; // found when the document is opened
};

} // namespace walk_pdf
