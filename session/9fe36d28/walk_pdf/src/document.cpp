#include "document.hpp"

#include "scanner.hpp"
#include "text_string.hpp"

#include "crypto/rc4.hpp"

#include <charconv>

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

namespace {

std::optional<long long> integer_of(std::optional<Content> const& content) {
  if (!content || content->kind() != Kind::integer) {
    return std::nullopt;
  }
  auto const text = content->bytes();
  long long value = 0;
  auto const* first = text.data() + (text.starts_with('+') ? 1 : 0);
  if (std::from_chars(first, text.data() + text.size(), value).ec != std::errc{}) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::string> name_of(std::optional<Content> const& content) {
  if (!content || content->kind() != Kind::name) {
    return std::nullopt;
  }
  return decode_name(content->bytes());
}

std::optional<std::string> string_of(std::optional<Content> const& content) {
  if (!content || content->kind() != Kind::string) {
    return std::nullopt;
  }
  return string_bytes(content->bytes());
}

// The object number of a reference "12 0 R"
std::optional<std::uint64_t> referenced_object(std::optional<Content> const& content) {
  if (!content || content->kind() != Kind::reference) {
    return std::nullopt;
  }
  auto const text = content->bytes();
  std::uint64_t number = 0;
  if (std::from_chars(text.data(), text.data() + text.size(), number).ec != std::errc{}) {
    return std::nullopt;
  }
  return number;
}

// The generation of an object, from its "n g obj"
std::uint32_t generation_of(Content const& object) {
  auto const text = object.bytes();
  auto const space = text.find_first_of(" \t\r\n\f");
  if (space == std::string_view::npos) {
    return 0;
  }
  auto const start = text.find_first_not_of(" \t\r\n\f", space);
  std::uint32_t generation = 0;
  if (start != std::string_view::npos) {
    std::from_chars(text.data() + start, text.data() + text.size(), generation);
  }
  return generation;
}

} // namespace

std::optional<Security> const& Document::security() const {
  if (!security_) {
    security_ = find_security();
  }
  return *security_;
}

std::optional<Security> Document::find_security() const {
  // The latest dictionaries with /Encrypt and /ID: trailers, or in files
  // without them, cross-reference streams
  std::optional<Path> encrypt;
  std::optional<Path> id;
  for (auto r = revisions_.size(); r-- > 0 && !(encrypt && id);) {
    auto const revision = Path{}.child(Segment::revision(r));
    std::vector<Path> holders;
    for (auto const& child : children(revision)) {
      if (child.content.kind() == Kind::trailer) {
        holders.push_back(child.path);
      }
    }
    if (holders.empty()) {
      for (auto const& child : children(revision)) {
        auto const type = find(child.path.child(Segment::entry("Type")));
        if (child.content.kind() == Kind::object && name_of(type) == "XRef") {
          holders.push_back(child.path);
        }
      }
    }
    for (auto holder = holders.rbegin(); holder != holders.rend(); ++holder) {
      if (!encrypt && find(holder->child(Segment::entry("Encrypt")))) {
        encrypt = holder->child(Segment::entry("Encrypt"));
      }
      if (!id && find(holder->child(Segment::entry("ID")))) {
        id = holder->child(Segment::entry("ID"));
      }
    }
  }
  if (!encrypt) {
    return std::nullopt;
  }

  // The encryption dictionary: usually an object of its own, in the latest
  // revision that has it
  Security security;
  auto dictionary = *encrypt;
  if (auto const number = referenced_object(find(*encrypt))) {
    security.dictionary_object = number;
    for (auto r = revisions_.size(); r-- > 0;) {
      auto const object = Path{}.child(Segment::revision(r)).child(Segment::object(*number));
      if (find(object)) {
        dictionary = object;
        break;
      }
    }
  }
  auto const entry = [&](std::string key) { return find(dictionary.child(Segment::entry(std::move(key)))); };

  security.filter = name_of(entry("Filter")).value_or("");
  security.version = static_cast<int>(integer_of(entry("V")).value_or(0));
  security.revision = static_cast<int>(integer_of(entry("R")).value_or(0));
  security.key_bytes = static_cast<std::size_t>(integer_of(entry("Length")).value_or(40)) / 8;
  security.permissions = static_cast<std::uint32_t>(integer_of(entry("P")).value_or(0));
  if (auto const metadata = entry("EncryptMetadata")) {
    security.encrypt_metadata = metadata->bytes() != "false";
  }

  if (security.filter != "Standard") {
    security.status = "security handler /" + security.filter + ": not supported";
    return security;
  }
  if (security.version == 4) {
    // Crypt filters: the one for streams must be RC4 (/V2)
    auto const filter = name_of(entry("StmF")).value_or("Identity");
    auto const method =
        name_of(find(dictionary.child(Segment::entry("CF")).child(Segment::entry(filter)).child(Segment::entry("CFM"))));
    if (method == "AESV2") {
      security.status = "AES-128: not supported yet";
      return security;
    }
    if (method != "V2") {
      security.status = "crypt filter /" + method.value_or("?") + ": not supported";
      return security;
    }
  } else if (security.version != 1 && security.version != 2) {
    security.status = security.version == 5 ? "AES-256: not supported yet"
                                            : "version " + std::to_string(security.version) + ": not supported";
    return security;
  }

  auto const owner = string_of(entry("O")).value_or("");
  auto const user = string_of(entry("U")).value_or("");
  auto const first_id = id ? string_of(find(id->child(Segment::item(0)))).value_or("") : std::string{};
  security.key = key_for_empty_password(security.revision, security.key_bytes, owner, user,
                                        security.permissions, first_id, security.encrypt_metadata);
  auto const bits = (security.revision == 2 ? 5 : std::min<std::size_t>(security.key_bytes, 16)) * 8;
  security.status = security.key ? "RC4 " + std::to_string(bits) + "-bit, no password needed"
                                 : "RC4 " + std::to_string(bits) + "-bit, needs a password";
  return security;
}

std::optional<std::string> Document::decrypt(Path const& path, std::string_view bytes) const {
  auto const& security = this->security();
  if (!security || !security->key) {
    return std::nullopt;
  }
  auto const& segments = path.segments();
  if (segments.size() < 2 || segments[1].kind != Kind::object) {
    return std::nullopt; // Not in an object, e.g. in a trailer
  }
  auto const number = segments[1].number;
  if (security->dictionary_object == number) {
    return std::nullopt;
  }
  auto const object_path = Path{{segments[0], segments[1]}};
  auto const object = find(object_path);
  if (!object) {
    return std::nullopt;
  }
  // Cross-reference streams are never encrypted, nor metadata when it says so
  auto const type = name_of(find(object_path.child(Segment::entry("Type"))));
  if (type == "XRef" || (type == "Metadata" && !security->encrypt_metadata)) {
    return std::nullopt;
  }
  return crypto::rc4(object_key(*security->key, number, generation_of(*object)), bytes);
}

} // namespace walk_pdf
