#pragma once

// Small text helpers shared by all readers and writers

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::text {

std::string_view trim(std::string_view s);

// Splits on LF and strips a trailing CR from each line. A final empty line is dropped.
std::vector<std::string_view> split_lines(std::string_view s);

// Splits on 'separator' and trims each part. "" gives an empty list.
std::vector<std::string> split_list(std::string_view s, char separator = ',');

std::string latin1_to_utf8(std::string_view latin1);
std::expected<std::string, std::string> utf8_to_latin1(std::string_view utf8);

// Removes a UTF-8 byte order mark, if present.
std::string_view strip_bom(std::string_view s);

} // namespace cratchit::text
