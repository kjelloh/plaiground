#include "core/ink2/output.hpp"

#include <gtest/gtest.h>

#include <chrono>

using cratchit::ink2::format_kronor;
using namespace std::chrono;

TEST(Ink2Output, Kronor) {
  EXPECT_EQ(format_kronor(0), "0");
  EXPECT_EQ(format_kronor(999), "999");
  EXPECT_EQ(format_kronor(65924), "65 924");
  EXPECT_EQ(format_kronor(-896), "-896");
  EXPECT_EQ(format_kronor(-1234567), "-1 234 567");
}

TEST(Ink2Output, Form) {
  auto const lines = cratchit::ink2::parse_mapping("bas;sru;ink2;text\n"
                                                   ";;;BALANSRÄKNING\n"
                                                   "19xx;7281;2.26;Kassa, bank och redovisningsmedel\n"
                                                   "1088;7202;2.2;Förskott avseende immateriella anläggningstillgångar "
                                                   "och en text som är för lång för en rad\n"
                                                   ";;;\n"
                                                   ";;;RESULTATRÄKNING\n"
                                                   "49xx (Om netto +);7411;3.2;Förändring av lager\n"
                                                   "49xx (Om netto -);7510;;\n");
  ASSERT_TRUE(lines) << lines.error();
  auto const form = cratchit::ink2::build_form(*lines, {{"1930", 6592396}, {"4900", 1000}});
  cratchit::ink2::Header const header{"Test AB", "556000-0001", 2024y / May / 1, 2025y / April / 30, "test.csv", true};
  EXPECT_EQ(cratchit::ink2::format_form(header, *lines, form),
            "INK2R Räkenskapsschema\n"
            "Test AB, 556000-0001\n"
            "Räkenskapsår 2024-05-01 - 2025-04-30   7011 20240501   7012 20250430\n"
            "Mapping test.csv\n"
            "PRELIMINÄR: the books are not closed; the year's result is shown as if booked\n"
            "\n"
            "BALANSRÄKNING\n"
            "  2.26  Kassa, bank och redovisningsmedel                           7281          65 924\n"
            "  2.2   Förskott avseende immateriella anläggningstillgångar och    7202\n"
            "        en text som är för lång för en rad\n"
            "\n"
            "RESULTATRÄKNING\n"
            "  3.2   Förändring av lager                                         7411 +\n"
            "                                                                    7510 -            10\n"
            "\n"
            "Summor av fälten, i hela kronor\n"
            "  Tillgångar (72xx)                                                               65 924\n"
            "  Eget kapital och skulder (73xx)                                                      0\n"
            "  Resultaträkningens fält (74xx - 75xx, utom 3.26-3.27)                              -10\n"
            "  Årets resultat (3.26 - 3.27)                                                         0\n");
}
