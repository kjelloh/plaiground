#include "document.hpp"

#include "scanner.hpp"

#include <algorithm>
#include <ranges>

namespace walk_pdf {

static_assert(std::input_iterator<ChildIterator>);
static_assert(std::ranges::input_range<Content>);

std::string_view Content::bytes() const {
  return document_->bytes().substr(begin_, end_ - begin_);
}

ChildIterator Content::begin() const { return ChildIterator{*this}; }

ChildIterator::ChildIterator(Content const& parent) : parent_(parent), position_(parent.begin_) {
  find_next();
}

ChildIterator& ChildIterator::operator++() {
  find_next();
  return *this;
}

// Finds the parent's next child from position_. The child's path segment is
// its object number for objects, and its ordinal among its kind otherwise.
void ChildIterator::find_next() {
  current_.reset();
  auto const& parent = *parent_;
  auto const& document = *parent.document_;

  std::optional<Found> found;
  switch (parent.kind_) {
  case Kind::document: {
    // Revisions were found when the document was opened
    auto const& revisions = document.revisions_;
    auto const next = std::ranges::find_if(
        revisions, [&](Document::Range const& range) { return range.begin >= position_; });
    if (next != revisions.end()) {
      found = Found{Kind::revision, next->begin, next->end};
    }
    break;
  }
  case Kind::revision:
    // Only the file's first bytes can be its header
    found = scan_element(document.bytes(), position_, parent.end_, 0);
    break;
  default:
    // Not looked into yet: everything else has no children
    break;
  }
  if (!found) {
    return;
  }

  auto& ordinal = ordinals_[static_cast<std::size_t>(found->kind)];
  Segment segment;
  switch (found->kind) {
  case Kind::header: segment = Segment::header(); break;
  case Kind::object: segment = Segment::object(found->number, found->generation); break;
  default: segment = Segment::ordinal(found->kind, ordinal); break;
  }
  ++ordinal;

  auto path = parent.path_.child(segment);
  current_.emplace(path, Content{document, path, found->kind, found->begin, found->end});
  position_ = found->end;
}

namespace {

// The revisions: the original file and each appended update, each running up
// to and including its %%EOF. Bytes after the last %%EOF form a revision of their own.
std::vector<std::pair<std::size_t, std::size_t>> find_revisions(std::string_view bytes) {
  std::vector<std::pair<std::size_t, std::size_t>> revisions;
  std::size_t position = 0;
  std::optional<std::size_t> begin;
  while (auto const found = scan_element(bytes, position, bytes.size(), 0)) {
    if (!begin) {
      begin = found->begin;
    }
    position = found->end;
    if (found->kind == Kind::eof) {
      revisions.emplace_back(*begin, position);
      begin.reset();
    }
  }
  if (begin) {
    revisions.emplace_back(*begin, position);
  }
  return revisions;
}

} // namespace

Document::Document(std::string bytes) : bytes_(std::move(bytes)) {
  for (auto const& [begin, end] : find_revisions(bytes_)) {
    revisions_.push_back(Range{begin, end});
  }
}

Content Document::root() const { return Content{*this, Path{}, Kind::document, 0, bytes_.size()}; }

std::optional<Content> Document::find(Path const& path) const {
  // Look up one segment at a time, among the children of the previous one
  auto content = root();
  for (auto const& segment : path.segments()) {
    auto const child = std::ranges::find_if(content, [&](Element const& element) {
      return element.path.segments().back() == segment;
    });
    if (child == content.end()) {
      return std::nullopt;
    }
    content = child->content;
  }
  return content;
}

} // namespace walk_pdf
