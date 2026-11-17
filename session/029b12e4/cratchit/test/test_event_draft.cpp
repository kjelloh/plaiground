#include "core/books/books.hpp"
#include "core/event_draft.hpp"
#include "core/sie_project.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace cratchit;
using cratchit::books::Event;
using namespace std::chrono;

namespace {

Date const oct20{2026y, October, 20d};

Event event(std::string_view text) {
  auto const parsed = parse_event(text);
  EXPECT_TRUE(parsed.has_value()) << text << ": " << (parsed ? "" : parsed.error());
  EXPECT_EQ(parsed ? parsed->size() : 0u, 1u) << text;
  return parsed && !parsed->empty() ? parsed->front() : Event{};
}

std::vector<Event> readings(std::string_view text) {
  auto const parsed = parse_event(text);
  EXPECT_TRUE(parsed.has_value()) << text << ": " << (parsed ? "" : parsed.error());
  return parsed.value_or(std::vector<Event>{});
}

std::string error_of(std::string_view text) {
  auto const parsed = parse_event(text);
  EXPECT_FALSE(parsed.has_value()) << text;
  return parsed ? std::string() : parsed.error();
}

} // namespace

TEST(ParseEvent, HeadingAmountAndDateInAnyOrder) {
  EXPECT_EQ(event("Telia faktura oktober 1118,00 2026-10-20"), (Event{"Telia faktura oktober", 111800, oct20}));
  EXPECT_EQ(event("20261020 1118.5 Telia faktura"), (Event{"Telia faktura", 111850, oct20}));
  EXPECT_EQ(event("Telia 2026-10-20 faktura 1118"), (Event{"Telia faktura", 111800, oct20}));
  EXPECT_EQ(event("qliro faktura 500 2026-10-20"), (Event{"qliro faktura", 50000, oct20}));
}

TEST(ParseEvent, TheLastNumberIsTheAmount) {
  EXPECT_EQ(event("Faktura 87 Payex 500 2026-10-20"), (Event{"Faktura 87 Payex", 50000, oct20}));
  // Not groups of thousands: the last word is not three digits, or the word before is too long
  EXPECT_EQ(event("Faktura 87 50,00 2026-10-20"), (Event{"Faktura 87", 5000, oct20}));
  EXPECT_EQ(event("Faktura 8700 500 2026-10-20"), (Event{"Faktura 8700", 50000, oct20}));
  EXPECT_EQ(event("Faktura 87,5 500 2026-10-20"), (Event{"Faktura 87,5", 50000, oct20}));
}

TEST(ParseEvent, SpacedNumbersGiveOneReadingPerGrouping) {
  EXPECT_EQ(readings("Faktura 87 500 2026-10-20"), (std::vector<Event>{{"Faktura 87", 50000, oct20}, {"Faktura", 8750000, oct20}}));
  EXPECT_EQ(readings("Telia 1 118,00 2026-10-20"), (std::vector<Event>{{"Telia 1", 11800, oct20}, {"Telia", 111800, oct20}}));
  EXPECT_EQ(readings("Lön 1 234 567,50 2026-10-20"),
            (std::vector<Event>{{"Lön 1 234", 56750, oct20}, {"Lön 1", 23456750, oct20}, {"Lön", 123456750, oct20}}));
  // A reading without heading is left out
  EXPECT_EQ(readings("1 118,00 Telia 2026-10-20"), (std::vector<Event>{{"1 Telia", 11800, oct20}, {"Telia", 111800, oct20}}));
}

TEST(ParseEvent, NoBreakSpaceInsideANumberIsClear) {
  EXPECT_EQ(event("Telia 1\xC2\xA0" "118,00 2026-10-20"), (Event{"Telia", 111800, oct20}));
}

TEST(ParseEvent, TheLastDateIsTheDate) {
  EXPECT_EQ(event("Momsrapport 2026-07-01 2026-09-30 47,11"),
            (Event{"Momsrapport 2026-07-01", 4711, Date{2026y, September, 30d}}));
}

TEST(ParseEvent, Errors) {
  EXPECT_EQ(error_of("Telia faktura 2026-10-20"), "Inget belopp. Skriv det med siffror, till exempel 1118,00.");
  EXPECT_EQ(error_of("Telia 1118"), "Inget datum. Skriv datumet, till exempel 2026-10-22.");
  EXPECT_EQ(error_of("1118 2026-10-20"), "Ingen rubrik. Skriv vad som hände, till exempel Telia faktura oktober.");
  EXPECT_EQ(error_of("Telia 0,00 2026-10-20"), "Beloppet är noll.");
  EXPECT_EQ(error_of("Telia 1118,005 2026-10-20"), "1118,005 är inget belopp (högst två decimaler).");
  EXPECT_EQ(error_of(""), "Inget belopp. Skriv det med siffror, till exempel 1118,00.");
}

TEST(ParseEvent, AnAmountBelowZeroKeepsItsSign) {
  // Minus for an event the other way than usual, e.g. a credit note (beslut 2026-11-17)
  EXPECT_EQ(event("Telia kreditfaktura -1245,00 2026-10-20"), (Event{"Telia kreditfaktura", -124500, oct20}));
  // Groups of thousands after a minus are unclear as after any number
  EXPECT_EQ(readings("Utbetalning -1 118,00 2026-10-20"),
            (std::vector<Event>{{"Utbetalning -1", 11800, oct20}, {"Utbetalning", -111800, oct20}}));
}

TEST(AddEventDraft, AddsTheEventAsADraftInsideTheYear) {
  auto const folder = fs::temp_directory_path() / "cratchit_event_draft_test";
  fs::remove_all(folder);
  fs::create_directories(folder);
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20260101 20261231\r\n"
         "#KONTO 1930 \"Bank\"\r\n";
  auto plan = plan_project_from_sie(folder / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
  Date const start{2026y, January, 1d};
  Date const today{2026y, November, 17d};
  auto added = add_event_draft(folder, start, {"Telia faktura oktober", 111800, oct20}, today);
  ASSERT_TRUE(added) << added.error();
  EXPECT_EQ(*added, 0u);
  added = add_event_draft(folder, start, {"Loopia", 130875, Date{2026y, September, 28d}}, today);
  ASSERT_TRUE(added) << added.error();
  EXPECT_EQ(*added, 0u); // earlier: first
  auto const refused = add_event_draft(folder, start, {"Nästa år", 100, Date{2027y, January, 2d}}, today);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "2027-01-02 ligger utanför räkenskapsåret 2026-01-01 – 2026-12-31");
  auto const books = books::read_books(folder);
  ASSERT_TRUE(books) << books.error();
  auto const* year = books->year_starting(start);
  ASSERT_NE(year, nullptr);
  ASSERT_EQ(year->drafts.size(), 2u);
  EXPECT_EQ(year->drafts[1].event, (Event{"Telia faktura oktober", 111800, oct20}));
  EXPECT_EQ(year->drafts[1].noted, today);
  EXPECT_TRUE(year->drafts[1].postings.empty());
  EXPECT_EQ(books::stage(year->drafts[1]), books::DraftStage::event);
  fs::remove_all(folder);
}

TEST(ForgetDraft, ForgetsTheDraftSeenOnly) {
  auto const folder = fs::temp_directory_path() / "cratchit_forget_draft_test";
  fs::remove_all(folder);
  fs::create_directories(folder);
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20260101 20261231\r\n";
  auto plan = plan_project_from_sie(folder / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
  Date const start{2026y, January, 1d};
  Date const today{2026y, November, 17d};
  ASSERT_TRUE(add_event_draft(folder, start, {"Telia", 111800, oct20}, today));
  ASSERT_TRUE(add_event_draft(folder, start, {"Loopia", 130875, Date{2026y, November, 1d}}, today));
  auto const books = books::read_books(folder);
  ASSERT_TRUE(books);
  auto const loopia = books->years.front().drafts[1];
  // Not the draft seen at that place: refused, nothing changes
  auto const refused = forget_draft(folder, start, 0, loopia);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Utkastet finns inte längre på sin plats i bokföringen. Öppna det igen från Utkast.");
  EXPECT_FALSE(forget_draft(folder, start, 5, loopia));
  // The one seen
  ASSERT_TRUE(forget_draft(folder, start, 1, loopia));
  auto const after = books::read_books(folder);
  ASSERT_TRUE(after);
  ASSERT_EQ(after->years.front().drafts.size(), 1u);
  EXPECT_EQ(after->years.front().drafts[0].event.heading, "Telia");
  fs::remove_all(folder);
}

TEST(DraftStage, ACreditNoteIsReadyWhenDebitAddsUpToItsAmountWithoutSign) {
  books::Draft draft{{"Telia kreditfaktura", -124500, oct20}, oct20, {}};
  auto posting = [](std::string account, Amount amount) {
    books::Posting p;
    p.account = std::move(account);
    p.amount = amount;
    return p;
  };
  draft.postings = {posting("2440", 124500), posting("6212", -99600), posting("2641", -24900)};
  EXPECT_EQ(books::stage(draft), books::DraftStage::ready);
  draft.postings[0].amount = 124400;
  EXPECT_EQ(books::stage(draft), books::DraftStage::unbalanced);
}
