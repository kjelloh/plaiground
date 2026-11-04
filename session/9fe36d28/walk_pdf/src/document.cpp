#include "document.hpp"

#include "font.hpp"
#include "scanner.hpp"
#include "text_string.hpp"

#include "crypto/rc4.hpp"

#include <charconv>
#include <cstdlib>
#include <set>

#include <algorithm>
#include <ranges>

namespace walk_pdf {

static_assert(std::input_iterator<ChildIterator>);
static_assert(std::ranges::input_range<Content>);

std::string_view Content::source() const {
  return buffer_ ? std::string_view{*buffer_} : document_->bytes();
}

std::string_view Content::bytes() const { return source().substr(begin_, end_ - begin_); }

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
  auto const bytes = parent.source();
  std::optional<Value> body;
  switch (parent.kind_) {
  case Kind::object: body = body_of(bytes, parent, "obj"); break;
  case Kind::trailer: body = body_of(bytes, parent, "trailer"); break;
  case Kind::dictionary:
  case Kind::array: body = Value{parent.kind_, parent.begin_, parent.end_}; break;
  default: break;
  }
  decoded_ = parent.kind_ == Kind::stream && parent.in_file();
  operations_ = (parent.kind_ == Kind::decoded && parent.document_->is_content_stream(parent.path_.parent())) ||
                parent.kind_ == Kind::text_block;
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
  emit(std::move(segment), kind, begin, end, parent_->buffer_);
}

// A child in the given bytes: the parent's for most, a decoded stream for "decoded"
void ChildIterator::emit(Segment segment, Kind kind, std::size_t begin, std::size_t end,
                         std::shared_ptr<std::string const> buffer) {
  auto const& document = *parent_->document_;
  auto path = parent_->path_.child(std::move(segment));
  document.remember(path, kind, begin, end);
  current_.emplace(path, Content{document, path, kind, begin, end, std::move(buffer)});
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
    if (auto const child = next_child(parent.source(), *container_, position_)) {
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
  if (decoded_) {
    // A stream's data, decrypted and with its filters applied
    decoded_ = false;
    if (auto const decoded = document.decode(parent.path_)) {
      emit(Segment::decoded(), Kind::decoded, 0, decoded->bytes->size(), decoded->bytes);
      return;
    }
  }

  // Inside a content stream: its text blocks, a block's operations, an operation's operands
  if (operations_ && parent.kind_ == Kind::decoded) {
    std::optional<std::size_t> block;
    while (auto const operation = next_operation(parent.source(), position_, parent.end_)) {
      position_ = operation->end;
      if (operation->name == "BT") {
        block = operation->begin;
      } else if (operation->name == "ET" && block) {
        auto const ordinal = ordinals_[static_cast<std::size_t>(Kind::text_block)]++;
        emit(Segment::ordinal(Kind::text_block, ordinal), Kind::text_block, *block, operation->end);
        return;
      }
    }
    if (block) { // A text block without its ET runs to the end
      auto const ordinal = ordinals_[static_cast<std::size_t>(Kind::text_block)]++;
      emit(Segment::ordinal(Kind::text_block, ordinal), Kind::text_block, *block, parent.end_);
    }
    return;
  }
  if (operations_ && parent.kind_ == Kind::text_block) {
    while (auto const operation = next_operation(parent.source(), position_, parent.end_)) {
      position_ = operation->end;
      if (operation->name != "BT" && operation->name != "ET") {
        auto const ordinal = ordinals_[static_cast<std::size_t>(Kind::operation)]++;
        emit(Segment::ordinal(Kind::operation, ordinal), Kind::operation, operation->begin, operation->end);
        return;
      }
    }
    return;
  }
  if (parent.kind_ == Kind::operation) {
    auto const source = parent.source();
    if (source.substr(parent.begin_, 2) == "BI") {
      return; // An inline image's parameters and data are not looked into
    }
    auto const operand = value_at(source, position_, parent.end_);
    if (!operand || (operand->kind == Kind::unknown && operand->end == parent.end_)) {
      return; // The operator
    }
    position_ = operand->end;
    auto const index = ordinals_[static_cast<std::size_t>(Kind::index)]++;
    emit(Segment::item(index), operand->kind, operand->begin, operand->end);
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
// An object found when the file is opened
struct FoundObject {
  std::uint64_t revision;
  std::uint64_t number;
  std::size_t begin;
  std::size_t end;
};

// The revisions: the original file and each appended update, each running up
// to and including its %%EOF. Bytes after the last %%EOF form a revision of
// their own. And every object in them.
std::pair<std::vector<std::pair<std::size_t, std::size_t>>, std::vector<FoundObject>>
find_revisions(std::string_view bytes) {
  std::vector<std::pair<std::size_t, std::size_t>> revisions;
  std::vector<FoundObject> objects;
  std::size_t position = 0;
  std::optional<std::size_t> begin;
  while (auto const found = scan_element(bytes, position, bytes.size(), 0)) {
    if (!begin) {
      begin = found->begin;
    }
    position = found->end;
    if (found->kind == Kind::object) {
      objects.push_back({revisions.size(), found->number, found->begin, found->end});
    }
    if (found->kind == Kind::eof) {
      revisions.emplace_back(*begin, position);
      begin.reset();
    }
  }
  if (begin) {
    revisions.emplace_back(*begin, position);
  }
  return {revisions, objects};
}

} // namespace

Document::Document(std::string bytes) : bytes_(std::move(bytes)) {
  auto const [revisions, objects] = find_revisions(bytes_);
  for (auto const& [begin, end] : revisions) {
    revisions_.push_back(Range{begin, end});
  }
  for (auto const& object : objects) {
    // A damaged file may define a number twice in a revision: the first counts
    objects_.try_emplace({object.revision, object.number}, Range{object.begin, object.end});
    latest_[object.number] = object.revision;
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
    auto buffer = buffer_for(path);
    if (!buffer && std::ranges::any_of(path.segments(), [](Segment const& s) { return s.kind == Kind::decoded; })) {
      return std::nullopt; // Its stream can no longer be decoded
    }
    return Content{*this, path, kind, begin, end, std::move(buffer)};
  }
  // An object, from the table made when the file was opened
  auto const& segments = path.segments();
  if (segments.size() == 2 && segments[0].kind == Kind::revision && segments[1].kind == Kind::object) {
    auto const known = objects_.find({segments[0].number, segments[1].number});
    if (known == objects_.end()) {
      return std::nullopt;
    }
    return Content{*this, path, Kind::object, known->second.begin, known->second.end};
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
  if (std::ranges::any_of(segments, [](Segment const& s) { return s.kind == Kind::decoded; })) {
    return std::nullopt; // Decoded stream data was decrypted as a whole
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

std::vector<Filter> Document::filters(Path const& stream) const {
  auto const object = stream.parent();
  auto const filter = find(object.child(Segment::entry("Filter")));
  auto const parameters = object.child(Segment::entry("DecodeParms"));
  std::vector<Filter> result;
  auto const add = [&](std::string name, Path const& parms) {
    Filter f{std::move(name)};
    auto const number = [&](std::string key, int fallback) {
      return static_cast<int>(integer_of(find(parms.child(Segment::entry(std::move(key))))).value_or(fallback));
    };
    f.predictor = number("Predictor", 1);
    f.colors = number("Colors", 1);
    f.bits_per_component = number("BitsPerComponent", 8);
    f.columns = number("Columns", 1);
    result.push_back(std::move(f));
  };
  if (auto const name = name_of(filter)) {
    add(*name, parameters);
  } else if (filter && filter->kind() == Kind::array) {
    std::uint64_t index = 0;
    for (auto const& item : children(filter->path())) {
      add(name_of(item.content).value_or("?"), parameters.child(Segment::item(index++)));
    }
  }
  return result;
}

std::optional<Document::DecodedStream> Document::decode(Path const& stream) const {
  if (auto const known = decoded_.find(stream); known != decoded_.end()) {
    return known->second;
  }
  auto const content = find(stream);
  if (!content || content->kind() != Kind::stream || !content->in_file()) {
    return std::nullopt;
  }
  auto const filters = this->filters(stream);
  if (!std::ranges::all_of(filters, [](Filter const& f) { return can_decode(f.name); })) {
    return std::nullopt;
  }
  auto data = decrypt(stream, content->bytes()).value_or(std::string{content->bytes()});
  auto result = walk_pdf::decode(std::move(data), filters);
  DecodedStream decoded{std::make_shared<std::string const>(std::move(result.bytes)), result.problem};

  // Keep it, and drop the oldest while over budget (but always the newest)
  decoded_bytes_ += decoded.bytes->size();
  decoded_.emplace(stream, decoded);
  decoded_order_.push_back(stream);
  while (decoded_bytes_ > decoded_budget && decoded_order_.size() > 1) {
    auto const oldest = decoded_.find(decoded_order_.front());
    decoded_bytes_ -= oldest->second.bytes->size();
    decoded_.erase(oldest);
    decoded_order_.pop_front();
  }
  return decoded;
}

bool Document::is_content_stream(Path const& stream) const {
  auto const object = stream.parent();
  auto const entry = [&](std::string key) { return find(object.child(Segment::entry(std::move(key)))); };
  if (auto const type = name_of(entry("Type")); type && type != "XObject" && type != "Pattern") {
    return false; // e.g. ObjStm, XRef, Metadata, EmbeddedFile, CMap
  }
  if (auto const subtype = name_of(entry("Subtype")); subtype && subtype != "Form") {
    return false; // e.g. Image, a font program, XML
  }
  if (entry("Length1") || entry("Length2") || entry("Length3")) {
    return false; // a font program
  }
  auto const decoded = decode(stream);
  if (!decoded) {
    return false;
  }
  auto const start = std::string_view{*decoded->bytes}.substr(0, 4096);
  if (start.find("begincmap") != std::string_view::npos) {
    return false; // a CMap, e.g. a font's /ToUnicode
  }
  // Drawing instructions are text; more than a quarter other bytes is binary data
  auto const other = std::ranges::count_if(start, [](char c) {
    auto const byte = static_cast<unsigned char>(c);
    return byte == 0 || (byte < 0x20 && c != '\n' && c != '\r' && c != '\t' && c != '\f') || byte >= 0x7F;
  });
  return static_cast<std::size_t>(other) * 4 <= start.size();
}

std::shared_ptr<std::string const> Document::buffer_for(Path const& path) const {
  auto const& segments = path.segments();
  auto const decoded = std::ranges::find_if(segments.rbegin(), segments.rend(),
                                            [](Segment const& s) { return s.kind == Kind::decoded; });
  if (decoded == segments.rend()) {
    return nullptr;
  }
  auto const stream = Path{std::vector<Segment>{segments.begin(), decoded.base() - 1}};
  auto const result = decode(stream);
  return result ? result->bytes : nullptr;
}

std::optional<Path> Document::object(std::uint64_t number) const {
  auto const latest = latest_.find(number);
  if (latest == latest_.end()) {
    return std::nullopt;
  }
  return Path{}.child(Segment::revision(latest->second)).child(Segment::object(number));
}

std::optional<Path> Document::resolve(Path const& reference) const {
  auto const number = referenced_object(find(reference));
  return number ? object(*number) : std::nullopt;
}

std::optional<Path> Document::follow(Path const& path) const {
  auto at = path;
  // A reference leads to an object, which is not itself a reference, so this
  // ends after one hop; the limit is only a guard
  for (int hops = 0; hops < 8; ++hops) {
    auto const content = find(at);
    if (!content) {
      return std::nullopt;
    }
    if (content->kind() != Kind::reference) {
      return at;
    }
    auto const target = resolve(at);
    if (!target) {
      return std::nullopt;
    }
    at = *target;
  }
  return std::nullopt;
}

namespace {

// The object numbers a /Contents entry refers to: one reference, or an array of them
std::vector<std::uint64_t> referenced_objects(Document const& document, Path const& contents) {
  std::vector<std::uint64_t> result;
  auto const content = document.find(contents);
  if (auto const number = referenced_object(content)) {
    result.push_back(*number);
  } else if (content && content->kind() == Kind::array) {
    for (auto const& item : document.children(contents)) {
      if (auto const n = referenced_object(item.content)) {
        result.push_back(*n);
      }
    }
  }
  return result;
}

} // namespace

void Document::find_resources() const {
  std::map<std::uint64_t, Path> found;
  for (auto const& [number, revision] : latest_) {
    auto const object = Path{}.child(Segment::revision(revision)).child(Segment::object(number));
    auto const entry = [&](std::string key) { return object.child(Segment::entry(std::move(key))); };
    if (name_of(find(entry("Type"))) == "Page") {
      // A page's resources may be inherited from its ancestors in the page tree
      std::optional<Path> resources;
      std::set<Path> visited;
      for (std::optional<Path> node = object; node && !resources && visited.insert(*node).second;) {
        if (find(node->child(Segment::entry("Resources")))) {
          resources = follow(node->child(Segment::entry("Resources")));
        } else {
          node = follow(node->child(Segment::entry("Parent")));
        }
      }
      if (resources) {
        for (auto const content : referenced_objects(*this, entry("Contents"))) {
          found.try_emplace(content, *resources);
        }
      }
    } else if (name_of(find(entry("Subtype"))) == "Form" && find(entry("Resources"))) {
      if (auto const resources = follow(entry("Resources"))) {
        found.try_emplace(number, *resources);
      }
    }
  }
  resources_ = std::move(found);
}

std::optional<Path> Document::resources(Path const& stream) const {
  auto const& segments = stream.segments();
  if (segments.size() < 2 || segments[1].kind != Kind::object) {
    return std::nullopt;
  }
  if (!resources_) {
    find_resources();
  }
  auto const found = resources_->find(segments[1].number);
  return found == resources_->end() ? std::nullopt : std::optional{found->second};
}

std::optional<Path> Document::font(Path const& path, std::string const& name) const {
  auto const resources = this->resources(path);
  if (!resources) {
    return std::nullopt;
  }
  auto const fonts = follow(resources->child(Segment::entry("Font")));
  return fonts ? follow(fonts->child(Segment::entry(name))) : std::nullopt;
}

std::shared_ptr<Font const> Document::load_font(Path const& font) const {
  if (auto const known = fonts_.find(font); known != fonts_.end()) {
    return known->second;
  }
  auto loaded = std::make_shared<Font const>(Font::load(*this, font));
  fonts_.emplace(font, loaded);
  return loaded;
}

std::optional<std::string> Document::font_name_at(Path const& decoded, std::size_t offset) const {
  auto known = font_changes_.find(decoded);
  if (known == font_changes_.end()) {
    auto const content = find(decoded);
    if (!content) {
      return std::nullopt;
    }
    // Where the font changes: after each Tf, and after each Q that restores one
    std::vector<std::pair<std::size_t, std::string>> changes;
    std::string current;
    std::vector<std::string> saved;
    auto const bytes = content->bytes();
    for (std::size_t pos = 0; auto const operation = next_operation(bytes, pos, bytes.size());) {
      pos = operation->end;
      if (operation->name == "Tf") {
        if (auto const name = value_at(bytes, operation->begin, operation->operator_begin);
            name && name->kind == Kind::name) {
          current = decode_name(bytes.substr(name->begin, name->end - name->begin));
          changes.emplace_back(operation->end, current);
        }
      } else if (operation->name == "q") {
        saved.push_back(current);
      } else if (operation->name == "Q" && !saved.empty()) {
        current = saved.back();
        saved.pop_back();
        changes.emplace_back(operation->end, current);
      }
    }
    if (font_changes_.size() >= 256) {
      font_changes_.clear();
    }
    known = font_changes_.emplace(decoded, std::move(changes)).first;
  }
  auto const& changes = known->second;
  auto const after = std::ranges::upper_bound(changes, offset, {}, &std::pair<std::size_t, std::string>::first);
  if (after == changes.begin() || std::prev(after)->second.empty()) {
    return std::nullopt;
  }
  return std::prev(after)->second;
}

std::optional<std::string> Document::shown_text(Content const& operation) const {
  auto const bytes = operation.bytes();
  auto const parsed = next_operation(bytes, 0, bytes.size());
  if (!parsed || (parsed->name != "Tj" && parsed->name != "TJ" && parsed->name != "'" && parsed->name != "\"")) {
    return std::nullopt;
  }
  // The decoded content stream it is in, and the font in effect there
  auto const& segments = operation.path().segments();
  auto const decoded = std::ranges::find_if(segments, [](Segment const& s) { return s.kind == Kind::decoded; });
  if (decoded == segments.end()) {
    return std::nullopt;
  }
  auto const stream = Path{std::vector<Segment>{segments.begin(), decoded + 1}};
  auto const name = font_name_at(stream, operation.offset());
  auto const font_path = name ? font(operation.path(), *name) : std::nullopt;
  if (!font_path) {
    return std::nullopt;
  }
  auto const font = load_font(*font_path);

  std::string text;
  std::optional<Value> last_string;
  for (std::size_t pos = 0; auto const operand = value_at(bytes, pos, parsed->operator_begin);) {
    pos = operand->end;
    if (operand->kind == Kind::string) {
      last_string = operand; // Tj and ' show their one string, " its third operand
    } else if (operand->kind == Kind::array && parsed->name == "TJ") {
      // Strings, and numbers that move the next one left (positive) or right
      for (auto at = first_child_at(*operand); auto const item = next_child(bytes, *operand, at);) {
        at = item->value.end;
        auto const token = bytes.substr(item->value.begin, item->value.end - item->value.begin);
        if (item->value.kind == Kind::string) {
          text += font->text(string_bytes(token));
        } else if (item->value.kind == Kind::integer || item->value.kind == Kind::real) {
          auto const gap = std::strtod(std::string{token}.c_str(), nullptr); // no from_chars for double in libc++
          if (gap <= -250 && !text.empty() && text.back() != ' ') {
            text += ' '; // a gap of a quarter of the font size or more: a space between words
          }
        }
      }
    }
  }
  if (last_string) {
    text += font->text(string_bytes(bytes.substr(last_string->begin, last_string->end - last_string->begin)));
  }
  return text;
}

std::optional<std::string> Document::text(Path const& path) const {
  auto const content = find(path);
  if (!content) {
    return std::nullopt;
  }
  if (content->kind() == Kind::operation) {
    return shown_text(*content);
  }
  if (content->kind() != Kind::text_block) {
    return std::nullopt;
  }
  std::string text;
  bool shown = false;
  auto const new_line = [&] {
    if (!text.empty() && text.back() != ' ') {
      text += ' ';
    }
  };
  for (auto const& child : children(path)) {
    auto const bytes = child.content.bytes();
    auto const parsed = next_operation(bytes, 0, bytes.size());
    if (!parsed) {
      continue;
    }
    auto const name = parsed->name;
    if (name == "T*" || name == "'" || name == "\"" || name == "Tm") {
      new_line();
    } else if (name == "Td" || name == "TD") {
      // A move down (or up) is a new line; a move along the line is not
      auto const x = value_at(bytes, 0, parsed->operator_begin);
      auto const y = x ? value_at(bytes, x->end, parsed->operator_begin) : std::nullopt;
      if (y && bytes.substr(y->begin, y->end - y->begin).find_first_not_of("+-0.") != std::string_view::npos) {
        new_line();
      }
    }
    if (auto const shown_here = shown_text(child.content)) {
      text += *shown_here;
      shown = true;
    }
  }
  if (!shown && text.empty()) {
    return std::nullopt;
  }
  return text;
}

} // namespace walk_pdf
