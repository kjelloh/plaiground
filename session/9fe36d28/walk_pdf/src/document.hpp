#pragma once

#include "path.hpp"
#include "syntax.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
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
  void emit(Segment segment, Kind kind, std::size_t begin, std::size_t end);

  std::optional<Content> parent_;
  std::size_t position_ = 0;                         // where the search for the next child starts
  std::array<std::uint64_t, kind_count> ordinals_{}; // children seen so far, per kind
  std::optional<Value> container_;                   // the dictionary or array whose children these are
  std::optional<Value> stream_;                      // a stream object's data, its last child
  std::optional<Element> current_;                   // nullopt at the end
};

// The children of the element at a path; empty if there is no such element
class Children {
public:
  explicit Children(std::optional<Content> parent) : parent_(std::move(parent)) {}

  ChildIterator begin() const;
  std::default_sentinel_t end() const { return {}; }

private:
  std::optional<Content> parent_;
};

// One PDF file, and the lookup of paths in it.
// Clients act on paths only: find(path) looks up an element, and
// children(path) gives its children, each with its own path.
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

  // The children of the element at path, in file order
  Children children(Path const& path) const { return Children{find(path)}; }

private:
  friend class ChildIterator;

  struct Range {
    std::size_t begin;
    std::size_t end;
  };

  // Where an element found by a scan is, so that looking up its path again
  // does not scan again. Filled whenever children are found; a path's meaning
  // never depends on it, it only makes lookups fast.
  //
  // It is a cache of the most recently found elements, not a record of all:
  // a client walking the file looks each element up just after the scan that
  // found it, so recent ones are enough. When full it starts over, which keeps
  // memory bounded for files with millions of elements.
  struct Location {
    Kind kind;
    std::size_t begin;
    std::size_t end;
  };
  static constexpr std::size_t index_capacity = 65536;
  void remember(Path const& path, Kind kind, std::size_t begin, std::size_t end) const {
    if (index_.size() >= index_capacity) {
      index_.clear();
    }
    index_.try_emplace(path, Location{kind, begin, end});
  }

  std::string bytes_;
  std::vector<Range> revisions_;                  // found when the document is opened
  mutable std::map<Path, Location> index_;        // not thread-safe
};

} // namespace walk_pdf
