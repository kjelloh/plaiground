#include "core/statement/statement.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

using namespace cratchit;
using namespace cratchit::statement;
using namespace std::chrono;

namespace {

std::vector<std::uint8_t> bytes_of(std::string_view text) {
  return {text.begin(), text.end()};
}

// The bytes of a UTF-8 file with a byte order mark
std::vector<std::uint8_t> with_bom(std::string_view text) {
  return bytes_of("\xEF\xBB\xBF" + std::string(text));
}

Document parsed(std::string_view text) {
  auto const document = parse_statement(with_bom(text));
  EXPECT_TRUE(document.has_value()) << document.error();
  return document.value_or(Document{});
}

std::string error_of(std::string_view text) {
  auto const document = parse_statement(with_bom(text));
  EXPECT_FALSE(document.has_value());
  return document ? std::string{} : document.error();
}

// As Nordea writes it: newest first, ; also ending the line, a saldo only on the last posting of a day
std::string const nordea =
    "Datum;Belopp;Avsändare;Mottagare;Namn;Ytterligare detaljer;Meddelande;Egna anteckningar;Saldo;Valuta;\n"
    "2024/07/22;-3800,00;;5249-4309;PostNord Sverige AB;PostNord Sverige AB;902887105820;Postbox;2964,85;SEK;\n"
    "2024/07/08;-20000,00;;3259  0174586;Nordea;Utdelning 2015;UTDELNING;;6764,85;SEK;\n"
    "2024/07/08;1077,49;;;HÖGDAL,KJELL-OLOV;HÖGDAL,KJELL-OLOV;HÖGDAL,KJELL-OLOV;;;SEK;\n"
    "2024/07/08;20000,00;32592317244;;Insättning;Insättning;UTDELNING HÖGDAL;;;SEK;\n"
    "2024/07/03;-5,55;;;PRIS ENL SPEC;PRIS ENL SPEC;;;5687,36;SEK;\n";

// As SKV writes it: oldest first, quoted, spaced thousands, balance lines that are not postings
std::string const skv = "\"THE ITFIED AB\";\"556782-8172\";\"\";\"\"\r\n"
                        "\"\";\"\";\"\";\"\"\r\n"
                        "\"\";\"Ingående saldo 2026-07-01\";\"\";\"666\"\r\n"
                        "\"2026-07-04\";\"Intäktsränta\";\"1\";\"667\"\r\n"
                        "\"2026-07-18\";\"Moms april 2026 - juni 2026\";\"2 292\";\"2 959\"\r\n"
                        "\"2026-07-21\";\"Utbetalning\";\"-2 292\";\"667\"\r\n"
                        "\"\";\"Utgående saldo 2026-09-30\";\"\";\"667\"\r\n";

} // namespace

TEST(StatementDecode, NeedsAUtf8ByteOrderMark) {
  EXPECT_EQ(decode(with_bom("Datum;Belopp")).value(), "Datum;Belopp");
  EXPECT_EQ(decode(bytes_of("Datum;Belopp")).error(),
            "The file does not start with a UTF-8 byte order mark (BOM), so its encoding is unknown. Save it as UTF-8 "
            "with a program that writes a BOM.");
  EXPECT_EQ(decode(with_bom("a\nb\xE5" "c\n")).error(),
            "Line 2 is not UTF-8, although the file starts with a UTF-8 byte order mark");
}

TEST(StatementSplit, CellsQuotesAndLineEnds) {
  // Every line ends with ;, so the empty cell after it is left out
  EXPECT_EQ(split("a; b ;c;\n\n\"x;y\";\"say \"\"hi\"\"\";\r\n", ';'),
            (std::vector<Row>{{1, {"a", "b", "c"}}, {2, {}}, {3, {"x;y", "say \"hi\""}}}));
  EXPECT_EQ(split("a;;", ';'), (std::vector<Row>{{1, {"a", ""}}}));
  // Not every line: an empty last cell is a cell
  EXPECT_EQ(split("a;b\nc;\n", ';'), (std::vector<Row>{{1, {"a", "b"}}, {2, {"c", ""}}}));
  EXPECT_EQ(split("\"two\nlines\";b\nc;d", ';'), (std::vector<Row>{{1, {"two\nlines", "b"}}, {3, {"c", "d"}}}));
}

TEST(StatementSplit, FindsTheSeparator) {
  EXPECT_EQ(find_separator(nordea), ';');
  EXPECT_EQ(find_separator("a\tb\tc\n1\t2,5\t3\n"), '\t');
  EXPECT_EQ(find_separator("a,b,c\n1,2,3\n"), ',');
  EXPECT_FALSE(find_separator("a;b\n1;2\n"));
}

TEST(StatementCells, PureDates) {
  EXPECT_EQ(date_of("2026-07-04"), (Date{2026y, July, 4d}));
  EXPECT_EQ(date_of("2026/07/04"), (Date{2026y, July, 4d}));
  EXPECT_FALSE(date_of("2026-07/04"));
  EXPECT_FALSE(date_of("20260704"));
  EXPECT_FALSE(date_of("2026-02-30"));
  EXPECT_FALSE(date_of("Ingående saldo 2026-07-01"));
}

TEST(StatementCells, PureAmounts) {
  EXPECT_EQ(amount_of("-1308,75"), -130875);
  EXPECT_EQ(amount_of("2 292"), 229200);
  EXPECT_EQ(amount_of("-2 292"), -229200);
  EXPECT_EQ(amount_of("1 234 567.5"), 123456750);
  EXPECT_EQ(amount_of("1\xC2\xA0" "118,00"), 111800);
  EXPECT_EQ(amount_of("1997"), 199700);
  EXPECT_EQ(amount_of("62600010249540"), 6260001024954000);
  EXPECT_FALSE(amount_of("91939000100432371")); // 17 digits
  EXPECT_FALSE(amount_of("51 86 87-9"));
  EXPECT_FALSE(amount_of("3259  0174586"));
  EXPECT_FALSE(amount_of("1234 567"));
  EXPECT_FALSE(amount_of("1 23"));
  EXPECT_FALSE(amount_of("1,234"));
  EXPECT_FALSE(amount_of("+5"));
  EXPECT_FALSE(amount_of(""));
  EXPECT_FALSE(amount_of("SEK"));
}

TEST(ParseStatement, NordeaNewestFirstWithSaldoOnSomeRows) {
  auto const document = parsed(nordea);
  EXPECT_EQ(document.separator, ';');
  EXPECT_EQ(document.date_column, 0u);
  EXPECT_EQ(document.amount_column, 1u);
  EXPECT_EQ(document.saldo_column, 8u);
  EXPECT_EQ(document.order, Order::newest_first);
  EXPECT_EQ(document.text_columns, (std::vector<std::size_t>{2, 3, 4, 5, 6, 7})); // not SEK, the same on every row
  EXPECT_EQ(document.first_line, 2u);
  EXPECT_EQ(document.last_line, 6u);
  EXPECT_EQ(document.opening, 569291);
  EXPECT_EQ(document.closing, 296485);
  EXPECT_EQ(document.notes, (std::vector<std::string>{"Line 1 is not an entry."}));
  ASSERT_EQ(document.entries.size(), 5u);
  EXPECT_EQ(document.entries[0], (Entry{Date{2024y, July, 3d}, -555, 568736, "PRIS ENL SPEC", 6}));
  EXPECT_EQ(document.entries[1], (Entry{Date{2024y, July, 8d}, 2000000, std::nullopt,
                                           "32592317244 | Insättning | UTDELNING HÖGDAL", 5}));
  EXPECT_EQ(document.entries[2].text, "HÖGDAL,KJELL-OLOV"); // the same cell three times
  EXPECT_EQ(document.entries[4].text, "5249-4309 | PostNord Sverige AB | 902887105820 | Postbox");
}

TEST(ParseStatement, SkvOldestFirstQuoted) {
  auto const document = parsed(skv);
  EXPECT_EQ(document.amount_column, 2u);
  EXPECT_EQ(document.saldo_column, 3u);
  EXPECT_EQ(document.order, Order::oldest_first);
  EXPECT_EQ(document.opening, 66600);
  EXPECT_EQ(document.closing, 66700);
  EXPECT_EQ(document.notes, (std::vector<std::string>{"Lines 1, 3 and 7 are not entries."}));
  ASSERT_EQ(document.entries.size(), 3u);
  EXPECT_EQ(document.entries[1], (Entry{Date{2026y, July, 18d}, 229200, 295900, "Moms april 2026 - juni 2026", 5}));
}

TEST(ParseStatement, WithoutSaldoTheDatesTellTheOrder) {
  auto const document = parsed("Date;Text;Amount\n2026-01-02;Fee;-1,85\n2026-01-01;Interest;1\n");
  EXPECT_FALSE(document.saldo_column);
  EXPECT_FALSE(document.opening);
  EXPECT_EQ(document.order, Order::newest_first);
  EXPECT_EQ(document.entries.front().text, "Interest");
  EXPECT_EQ(document.notes.back(), "No saldo: the amounts are not checked.");
  EXPECT_EQ(parsed("2026-01-01;Fee;-1,85\n2026-01-01;Interest;1\n").order, Order::unknown);
  EXPECT_EQ(error_of("2026-01-02;a;1\n2026-01-01;b;1\n2026-01-03;c;1\n"), "Lines 1-3: the dates neither rise nor fall");
}

TEST(ParseStatement, ASingleRowNeedsASingleAmount) {
  auto const document = parsed("2026-01-01;Fee;-1,85\n");
  ASSERT_EQ(document.entries.size(), 1u);
  EXPECT_EQ(document.entries.front().amount, -185);
  EXPECT_EQ(document.text_columns, (std::vector<std::size_t>{1})); // one row: nothing is the same on every row
  EXPECT_EQ(error_of("Datum;Belopp;Saldo;Text\n2025/01/24;50000,00;117992,79;PG TILL SPAR\n"),
            "Only one entry (line 2), with 2 amounts: which is the amount and which the saldo cannot be told from one "
            "line");
}

TEST(ParseStatement, ASaldoThatDoesNotAddUpIsAnError) {
  EXPECT_EQ(error_of("2026-01-01;a;1;10;x\n2026-01-02;b;2;12;y\n2026-01-03;c;3;16;z\n"),
            "Lines 1-3: columns 3 and 4 each hold an amount on every line, and no saldo adds up to tell which is the "
            "amount");
  EXPECT_EQ(error_of("2026-01-01;a;1;10\n2026-01-02;b;2;\n2026-01-03;c;3;17\n"),
            "Lines 1-3: the saldo does not add up (amount column 3, possible saldo columns 4)");
}

TEST(ParseStatement, AnInvalidRowEndsTheRun) {
  auto const document = parsed("2026-01-01;a;1;11\n2026-01-02;b;2;13\nSumma;3\n2026-01-03;c;3;16\n");
  EXPECT_EQ(document.last_line, 2u);
  EXPECT_EQ(document.notes.front(), "Lines 3-4 are not entries.");
  EXPECT_EQ(error_of("2026-01-01;a;1\nSumma;1\n2026-01-03;c;3\n"),
            "Two runs of 1 entry lines (lines 1-1 and 3-3): which are the entries?");
  EXPECT_EQ(error_of("Datum;Belopp;Text\n"), "No entries: no line has exactly one date and an amount");
}
