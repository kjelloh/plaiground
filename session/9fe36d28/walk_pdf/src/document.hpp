#pragma once

#include "filters.hpp"
#include "path.hpp"
#include "security.hpp"
#include "syntax.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace walk_pdf {

class Document;
class ChildIterator;

// What is at a path: its kind, its bytes, and its children.
// Content views bytes owned by its Document, which must outlive it: the
// file's bytes, or for elements inside a decoded stream, that stream's
// decoded bytes (which the Content shares, so they stay while it lives).
// Looping over a Content gives its children as Elements.
class Content {
public:
  Document const& document() const { return *document_; }
  Path const& path() const { return path_; }
  Kind kind() const { return kind_; }
  std::size_t offset() const { return begin_; } // in the file, or in the decoded stream
  bool in_file() const { return !buffer_; }      // false inside a decoded stream
  std::string_view bytes() const;

  ChildIterator begin() const;
  std::default_sentinel_t end() const { return {}; }

private:
  friend class Document;
  friend class ChildIterator;
  Content(Document const& document, Path path, Kind kind, std::size_t begin, std::size_t end,
          std::shared_ptr<std::string const> buffer = {})
      : document_(&document), path_(std::move(path)), kind_(kind), begin_(begin), end_(end),
        buffer_(std::move(buffer)) {}

  // All the bytes this content's offsets are in: the file or a decoded stream
  std::string_view source() const;

  Document const* document_;
  Path path_;
  Kind kind_;
  std::size_t begin_; // offset of the first byte in the source
  std::size_t end_;   // offset just after the last byte
  std::shared_ptr<std::string const> buffer_; // a decoded stream, or null for the file
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
  void emit(Segment segment, Kind kind, std::size_t begin, std::size_t end,
            std::shared_ptr<std::string const> buffer);

  std::optional<Content> parent_;
  std::size_t position_ = 0;                         // where the search for the next child starts
  std::array<std::uint64_t, kind_count> ordinals_{}; // children seen so far, per kind
  std::optional<Value> container_;                   // the dictionary or array whose children these are
  std::optional<Value> stream_;                      // a stream object's data, its last child
  bool decoded_ = false;                             // a stream's decoded data, its only child
  bool operations_ = false;                          // the parent is a content stream
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

  // How the file is encrypted, or nullopt if it is not. Found on first use,
  // from the /Encrypt and /ID entries of the latest trailer (or
  // cross-reference stream) that has them.
  std::optional<Security> const& security() const;

  // The bytes of a string or of stream data inside the object at path,
  // decrypted. nullopt when they are not encrypted: the file is not, or
  // walk_pdf can not decrypt it, or path is not inside an object, or the
  // object is the encryption dictionary or a cross-reference stream.
  std::optional<std::string> decrypt(Path const& path, std::string_view bytes) const;

  // A stream's filters, from its object's /Filter and /DecodeParms
  std::vector<Filter> filters(Path const& stream) const;

  // A stream's data decrypted and with its filters applied, and any problem
  // that stopped decoding early. nullopt if a filter is one walk_pdf can not
  // apply (e.g. an image's DCTDecode). Kept in a cache of recent streams.
  struct DecodedStream {
    std::shared_ptr<std::string const> bytes;
    std::optional<std::string> problem;
  };
  std::optional<DecodedStream> decode(Path const& stream) const;

  // References (PDF 32000-1:2008, 7.3.10): "12 0 R" refers to object 12, in
  // the latest revision that has it. walk_pdf does not yet look into object
  // streams, so objects stored in them are not found.

  // The latest version of an object, e.g. /rev[1]/obj[12]
  std::optional<Path> object(std::uint64_t number) const;

  // The object the reference at path refers to
  std::optional<Path> resolve(Path const& reference) const;

  // The element at path, or if it is a reference, the object it refers to
  std::optional<Path> follow(Path const& path) const;

  // The resources dictionary a content stream draws with: its page's
  // /Resources (inherited through /Parent if the page has none), or a form's
  // own. stream is the content stream's object or anything inside it.
  std::optional<Path> resources(Path const& stream) const;

  // The font that name (e.g. "F8", as in "/F8 16 Tf") stands for in the
  // content stream that path is in
  std::optional<Path> font(Path const& path, std::string const& name) const;

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

  std::optional<Security> find_security() const;

  // Is the stream a content stream: drawing instructions, as a page's? Told
  // from its dictionary and data, as walk_pdf does not yet follow a page's
  // /Contents: not an image, font, CMap, object stream, metadata or the like.
  bool is_content_stream(Path const& stream) const;

  // The decoded stream that holds the element at path, or null if it is in the file
  std::shared_ptr<std::string const> buffer_for(Path const& path) const;

  std::string bytes_;
  std::vector<Range> revisions_;                  // found when the document is opened

  // Every object, found when the document is opened: where it is in each
  // revision, and the latest revision that has it
  std::map<std::pair<std::uint64_t, std::uint64_t>, Range> objects_; // (revision, number)
  std::map<std::uint64_t, std::uint64_t> latest_;                    // number -> revision

  // Content stream object number -> its resources, found on first use
  mutable std::optional<std::map<std::uint64_t, Path>> resources_;
  void find_resources() const;
  mutable std::optional<std::optional<Security>> security_; // found on first use
  mutable std::map<Path, Location> index_;        // not thread-safe

  // Recently decoded streams, the oldest dropped when they hold more than the budget
  static constexpr std::size_t decoded_budget = 64 * 1024 * 1024;
  mutable std::map<Path, DecodedStream> decoded_;
  mutable std::deque<Path> decoded_order_;
  mutable std::size_t decoded_bytes_ = 0;
};

} // namespace walk_pdf
