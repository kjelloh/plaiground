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

ChildIterator Children::begin() const { return parent_ ? parent_->begin() : ChildIterator{}; }

namespace {

// The value an object or trailer holds: what follows "n g obj" or "trailer"
std::optional<Value> body_of(std::string_view bytes, Content const& content,
                             std::string_view keyword) {
  auto const text = bytes.substr(content.offset(), content.bytes().size());
  auto const at = text.find(keyword);
  if (at == std::string_view::npos) {
    return std::nullopt;
  }
  return value_at(bytes, content.offset() + at + keyword.size(),
                  content.offset() + content.bytes().size());
}

} // namespace

ChildIterator::ChildIterator(Content const& parent) : parent_(parent), position_(parent.begin_) {
  auto const bytes = parent.document_->bytes();
  std::optional<Value> body;
  switch (parent.kind_) {
  case Kind::object: body = body_of(bytes, parent, "obj"); break;
  case Kind::trailer: body = body_of(bytes, parent, "trailer"); break;
  case Kind::dictionary:
  case Kind::array: body = Value{parent.kind_, parent.begin_, parent.end_}; break;
  default: break;
  }
  if (body && (body->kind == Kind::dictionary || body->kind == Kind::array)) {
    container_ = body;
    position_ = first_child_at(*body);
    if (parent.kind_ == Kind::object && body->kind == Kind::dictionary) {
      stream_ = stream_data(bytes, body->end, parent.end_);
    }
  }
  find_next();
}

void ChildIterator::emit(Segment segment, Kind kind, std::size_t begin, std::size_t end) {
  auto const& document = *parent_->document_;
  auto path = parent_->path_.child(std::move(segment));
  document.remember(path, kind, begin, end);
  current_.emplace(path, Content{document, path, kind, begin, end});
}

ChildIterator& ChildIterator::operator++() {
  find_next();
  return *this;
}

// Finds the parent's next child from position_. The child's path segment is
// its object number for objects, and its ordinal among its kind otherwise.
// A damaged file may define an object number twice in one revision; then the
// path finds the first.
void ChildIterator::find_next() {
  current_.reset();
  auto const& parent = *parent_;
  auto const& document = *parent.document_;

  // Inside objects: a dictionary's entries or an array's items, then a stream's data
  if (container_) {
    if (auto const child = next_child(document.bytes(), *container_, position_)) {
      position_ = child->value.end;
      auto const index = ordinals_[static_cast<std::size_t>(Kind::index)]++;
      emit(container_->kind == Kind::dictionary ? Segment::entry(child->key) : Segment::item(index),
           child->value.kind, child->value.begin, child->value.end);
      return;
    }
    container_.reset();
  }
  if (stream_) {
    emit(Segment::stream(), Kind::stream, stream_->begin, stream_->end);
    stream_.reset();
    return;
  }

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
  case Kind::object: segment = Segment::object(found->number); break;
  default: segment = Segment::ordinal(found->kind, ordinal); break;
  }
  ++ordinal;

  position_ = found->end;
  emit(segment, found->kind, found->begin, found->end);
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
  if (path.is_root()) {
    return root();
  }
  // Found before (or a sibling of something found before)
  if (auto const known = index_.find(path); known != index_.end()) {
    auto const& [kind, begin, end] = known->second;
    return Content{*this, path, kind, begin, end};
  }
  // Otherwise find the parent, and look among its children. That scan
  // remembers every child it passes, so their lookups need no scan.
  auto const parent = find(path.parent());
  if (!parent) {
    return std::nullopt;
  }
  auto const& segment = path.segments().back();
  auto const child = std::ranges::find_if(
      *parent, [&](Element const& element) { return element.path.segments().back() == segment; });
  if (child == parent->end()) {
    return std::nullopt;
  }
  return child->content;
}

} // namespace walk_pdf
