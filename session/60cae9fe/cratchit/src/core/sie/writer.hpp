#pragma once

#include "core/sie/document.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Writing an SIE 4 file: the inverse of the lexer and parse_document
namespace cratchit::sie {

// The records of the document as SIE 4 text (UTF-8), one per line with CRLF, in
// the order SIE 4B lists them, the sections apart by a blank line as Fortnox
// does: identification, accounts, dimensions and objects, the balances of each
// year (#IB then #UB per account), the results, then the vouchers with their
// transactions. An #RTRANS is followed by a like #TRANS (without signature) for
// programs that do not know #RTRANS. Texts are quoted, " written \"; amounts
// have a point and no trailing zeros (e.g. 1299.7, 13000); dates YYYYMMDD.
// Unknown labels and #KSUMMA are not written.
std::string text_of(Document const& document);

// SIE text in CP437 (PC8), as SIE requires
struct Encoded {
  std::vector<std::uint8_t> bytes;
  std::size_t unmapped = 0; // characters CP437 does not have, each written as ?
};

// The UTF-8 text in CP437. A character CP437 does not have, or a byte that is
// not UTF-8, becomes ? and is counted. Does not log.
Encoded cp437_of(std::string_view text);

} // namespace cratchit::sie
