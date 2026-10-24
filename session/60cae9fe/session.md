# Vibe code a Swedish book keeping app named cratchit - take 3 (Drop feed Claude)

```#60cae9fe```

## 20261024

I have now decided to 'brute force' functionality by Vibe coding and don't care too much about code structure or design.

So what's next?

I asked Claude to vibe code a 'pick voucher template' as an option when editing and or posting a financial event.

I tested the 'pick template'.

* It seems to work sufficiently?
* I think what I now need most is a way to remove vouchers I have posted?
    * I need this anyhow for live application of cratchit.
    * And this will enable me to test and revert on my current example data.

So I asked Claude to implement view and edit existing vouchers.

```text
Both tasks are done: you can view the vouchers of a fiscal year, correct a voucher you entered in cratchit, and start a new voucher from any viewed one. 
```

I made a quick view run-through and it seems to work just fine.

* I now tried the following prompt:

```
Time for next task to parse account statements into something to use as base for creating appropriate events and vouchers. I have added a larger set of account statements in _resources/_from_bank_or_skv. I think I want a parser that creats some form of 'account statement document' in memory. I think I want it to hold only valid debit and credit posting to the account. A valid account posting shall have at least the Date, posted amount and a caption text. The account statement file may list the postings in forward or reverse order. It may provide an optional saldo amount. It would be nice if the parser is able to identify the Date and the posted amount on column content only. That is, we expect a valid row to contain only one pure date column and at least one pure amount column. Maybe we can assemble a text by somehow assemble identified text columns into one text? If possible, I want the parser to verify Saldo amounts by running a row-by-row accumulation to verify that posted amounts add up to the running saldo amount where ther is one. I want to leave using the account statement document for now to first see what we can design? What can you propose?
```

* Now Claude also picked up my spec in this session document (which turned out to be a good thing)!
* It cae back with questions about single line statement files, encoding, text joining and opening closing saldo.
    * I asked it to accept single line if there is only a single amount and date field (assuming this should always mean it is a debit/credit entry?)
    * I asked it to reject the file if no BOM and UTF8 did not work (seems safe for now?)
    * Also, fall back to any Latin-based encoding may not work world-wide?
    * I mean, For slavic languages, different scandinavian langues etc., we can assume we know what sub-set of Latin we are dealing with?
    * I asked for text joining with ','
    * I asked for identifying open and close balance by matching running posted amounts against them (open slado + running posts = close saldo)

I have now vibe coded 'import account statement' file(s)

* There is a UI from project and year (pruned to that year)
* Imported postings are stashed in a statements file in dot-folder named after start-date

And now I have vibe coded 'statement entry to waiting event'

* I can view the unbooked statement entries
* I can turn a statement entry into a waiting event

The UI is very cluttered compared to my previous cratchit. But maybe that is what we need. And I can always clean this up later?

* We have statement::Entry to represent an entry in an account statement
* We have books::Posting to represent posting to a BAS account

## 20261023

It is now afternoon and I will try to make Claude implement an account statement file reader.

I am not yet sure if I want to create an 'account statement document' or directly to plain BAS account postings?

Anyhow, I edited the following specification by going back and forth with Claude.

* The reader shall turn account statement rows into BAS account postings
* Any consolidation of postings into events and then vouchers are separate later operations.
    * The reader shall only be concerned with creating valid BAS account postings.
* The reader shall be able to read csv-files in at least UTF8 (using existing character set transcode mechanisms for simple detection)
* It shall be able to consume any BOM in the file (so it does not turn up as garbage in read rows) 
* It shall be able to detect and parse a file using separators from a small set (and implement at least one default one for now that works on examples I provide from Nordea and SKV)
* It shall identify a continuous range of account statement rows that represents an account debit or credit.
    * All rows must match the criteria of a valid debit or credit statement.
    * All rows must share at least one Date column, one amount column and one text column (to be used as posting caption)
    * All date values must be pure (date found in text counts as text, not a valid date)
    * All amount values must be pure (amounts found in text counts as text, not a valid amount)
    * An invalid row terminates the range
* The reader shall be able to read an extra amount from some rows to handle optional Saldo amounts.
* The reader shall apply an algorithm to determine the posted amount column from any saldo amount column.
    * If only one column carries an amount for all rows then this must be the posted amount.
    * It shall accumulate posted amount over the range both forward and backward to detect row order of postings
    * It shall accumulate suspected posted amount and compare with suspected saldo amount (both ways if not yet determined)
    * It shall apply these checks row-by-row (all rows must comply with the rules)
* If no conflicts are detected the algorithm shall accept the identified columns content.
* If rows order and amount types are confirmed it shall transform them into potential BAS account postings.
    * Initially it shall use a BAS account that is either nullopt or a 'observe' BAS account for later assignment.
* The reader shall NOT try to determine meta-data that is not explicitly in the file.
    * None of e.g., The bank name, account name, bank account number, SKV account number etc. shall be assumed to be in the file
    * Explicitly, linking postings in the file to a BAS account is a later separate step and NOT a task for the reader.
* The reader shall NOT try to find separate open and close balance rows
 


Today I decided to ask Claude to refactor the current design so we can treat a 'voucher' as having two parts.

* One part is the Heading, Amount and Date (The overall thing about a financial event)
* The other part is the aggregate of postings to BAS accounts for this event.

Claude came back with a design and refactoring tasks that I found reasonable. So I accepted and let Claude do its thing.

* The term 'Transaction' (inspired by SIE entry #TRANS) is now refactored to 'Posting' in code
    * This makes sense as it is in fact not a transaction (not a from-to thing) so 'posting' is better.
* The term Voucher remains the complete registered financial event in the books (Matches SIE #VER)
    * So I think we will end up with Voucher being an Event + Postings?
    * YES. Claude seems to confirm this.

```text
Step 1 is done: vouchers are now built from an Event and its Postings, and the UI works as before. The build has no warnings and all 343 tests pass. I also ran it in a scratch folder: a Fortnox sample imports, importing it again gives "Nothing to import", and the voucher report looks as before.
``` 

I went ahead and made Claude implement the 'enter events' UI also.

```text
Step 2 is done: you can now note events like invoices and receipts at the year prompt with n, and post or delete them later with w. The build has no warnings and all 347 tests pass. I also ran it in the app on a copy of your Fortnox sample: I noted two events, posted one as voucher D 27, and deleted the other.
```

I now tried entering events.

* It would be nice if we remained in 'notes' state so we can continue to enter events.
    * Remove the 'y/n' confirmation (show the waiting list)
    * Remain in 'notes' ode so the user can enter a new one directly
* It would be nice to enter an event with a Euro amount (e.g., Anthropic Receipt)
    * Store amount with currency
    * Resolve in posting to BAS accounts (E.e., withdrawal from swedish bank account in SEK)?
    * Allow to create SIE voucher (should be OK as SIE voucher has no amount of its own?)
* Make empty enter in 'event' list waiting
    * This makes it easy for user to check where in the list the are (what has been entered so far)
* We actually would like an Event to carry negative amounts (e.g., credited invoice)
    * I had an invoice credited and I think it would be nice to have the amount sig be carried along?
    * On the other hand, it will be handled in BAS account posting anyways?
    * But maybe a negative amount makes it easier for the user to get Debit/Credit correct?


## 20261022

I wonder if I still lack trust in the vibe coding process? What happens if I just go for it? Still task-by-tak though?

* So I decide if a task is done or needs tweaks.
* I decide what the nxt task should be.
* But I do NOT care about what the code looks like?

Do I dare to do this?

* I still need to trust cratchit to perform book keeping and reporting correct!
* I can accomplish this by asking for cratchit being able to show reports about what it has done.
* I can integrate required book keeping information into the use manual (help file)?
* In this way I can read the requirements and compare with what cratchit does?

Ok, enough thinking and second guessing. What is next?

* Read in an SIE file!
* Cratchit now needs an internal representations of the books.
* How much do I need to tell Claude?
* Should cratchit stash a clone of the imported SIE file?
* Should cratchit remember what was imported to keep track of changes compared to the imported books?

I asked Claude to implement SIE import with user interface, parser and some open hints about if to stash the import and use sie.se and bas.se for specs.

* It came back with the proposal to begin with the SIE file lexer.
* But it also laid out task steps towards SIE import, internal book update and checks before accept.
* I asked me about overlaps, support for SIE 5 etc.
* I actually felt reassured to go ahead from the questions and what it seemed Claude (and the model) understood about the goal.

Claude came back with some interesting insights.

* Fortnox SIE file provides no checksum (fragile SIE file content?)
```sh
- The samples confirm a few things the parser will need to handle:
  - #RTRANS is followed by a duplicate #TRANS, as I suspected.
  - Fortnox also writes #BTRANS, #PSALDO, #DIM and #FNR.
  - There is no #KSUMMA, so the checksum check is only possible when a file has one.
```

* Claude asked.

```sh
Decision for you: CP437 maps every byte to a character, so decoding never fails and there is no failure count to abort on. How should cratchit decide that a file is really in another encoding? Two options:
- abort when the file is valid UTF-8 with multi-byte characters;
- abort when CP437 decoding gives box-drawing or graphics characters, which don't belong in bookkeeping text.
```

Claude have now defined a list of tasks to implement SIE file import.

```

Seven tasks remain in the SIE import plan, numbered 20–26 in progress.md:

20. SIE document, part 1: company details (#FNAMN, #ORGNR, #RAR, #KPTYP and similar) and the chart of accounts (#KONTO, #SRU, #KTYP), read from the lexed lines into the SIE document.
21. SIE document, part 2: opening and closing balances (#IB, #UB, #RES, #PSALDO), vouchers (#VER with #TRANS, #RTRANS, #BTRANS) and dimensions (#DIM, #OBJEKT). In the Fortnox files each #RTRANS is followed by a duplicate #TRANS, which has to be skipped.
22. File checks: the checksum (#KSUMMA) when the file has one, vouchers that sum to zero, accounts that exist in the chart, and opening balance plus transactions equal to the closing balance.
23. Import dry run: i at project> lets you type a file path. Cratchit then shows a summary, the check results and any encoding alert, and changes nothing. This is the first step you can try in the app.
24. Books model: cratchit's own representation of the books, and the files it is saved in under the project's .cratchit.
25. Import:
    - You confirm with y/n, and the books are updated.
    - The first import sets the org number and other company details. A later file that doesn't match them is an error.
    - New financial events are added; existing ones are never changed.
    - The file is stashed in the project with a record of the import, the import is logged, and the manual is updated.
26. Reports: saldobalans (trial balance), a voucher list and an account ledger, so you can compare cratchit's books with Fortnox.

Later, outside this plan: checking the chart of accounts against BAS, exporting SIE (and testing that an import followed by an export gives the same file), and SIE 5.
```

At this stage I went for it and told Claude to implement all remaining tasks in one go. ANd ask only if critical decisions needs to be made by me.

* It came cak with questions about miss-matched in and out balances in SIE files.
* It detected that input balance for 2026/2027 did now match output balance for 20205/2026.
    * I suggested cratchit should warn and use its own in and out balances if it has them
    * And alert in log and to the user about the discrepancy but ignore and use contained vouchers.

Now when I think about it.

* If the in-balance in an SIE-file differs from the out-balance in a previous year imported SIE file, then this means the SIE file snapshots differs.
    * But we don't know where the difference is?
    * Cratchit should always treat its own books as the 'truth'
    * And update this truth from SIE files only if there is NO conflict!
    * We can later add a 'force' flag to re-synk cratchit with an external truth.
    * But for now SIE file import only allows for new financial events - NOT mutations!

 
 Claude have now implemented SIE import with user interface and persistent storage.

 * The user interface works (but is over complicated!)

 ```text
 project> 
Keys at project>:
  i  Import an SIE file
  r  Reports
  l  Show the log
  -  Back to workspace>
  h  Help (the manual)
  q  Quit
project> i
Type the path of the SIE file to import. Tab completes a folder or file name, Backspace erases,
Enter accepts. Enter on an empty path goes back.
path> /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/62c6984e/_sie_in/TheITfiedAB20261019_105220.se
SIE file /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/62c6984e/_sie_in/TheITfiedAB20261019_105220.se
  Read as      CP437
  Program      Fortnox 3.62.17, generated 2026-10-19
  Company      The ITfied AB, 556782-8172
  Fiscal year  2025-05-01 - 2026-04-30
  Accounts     501: 501 new, 0 changed
  Vouchers     116: 116 new, 0 already in the books
Notes:
  - The project gets organisation number 556782-8172 (#ORGNR)
  - The project's name changes from "ITfied AB" to "The ITfied AB" (#FNAMN)
Import it? (y/n) y
Initiated .cratchit/books/books.txt in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/accounts.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/dimensions.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/objects.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/2025-05-01/opening.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/2025-05-01/vouchers.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/2025-05-01/transactions.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Updated .cratchit/project.txt in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Stashed the file as .cratchit/imports/2026-10-22_1_TheITfiedAB20261019_105220.se in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Imported 116 vouchers into fiscal year 2025-05-01 - 2026-04-30 (0 already in the books).
project> 
 ```

What are ALL THOSE new project files!!

At first I thought I should ask Claude to clean this mess up.

* On second thought I imagine I can leave the result as is and just carry on?
* This is Vibe Coding so we can take this as an exercise to see how agentic coding behaves?

So next we can now implement actual accounting and accounting support?

* Do we need to somehow base this functionality on BAS specification(s)?
* Maybe to provide BAS accounting help in the manual?
* Do we need to store our own machine readable 'account plan'?
* If so, how do we use account information from SIE files with BAS accounting specification?
* Can they clash?
    * If so, what do we doo?

Finally, I actually already know we will later need to know how class 8 BAS accounts map to their types (asset, liability, cost, income).

* Is there a BAS specification from which we can extract account types for each BAS account?

So I asked Claude to investigate and the answer is annoying.

* It seems BAS charges 4000 SEK ex VAT for a json-file.
    * And this json-file is then only licensed for won use (not copy, share or sub-license)
* And the free pdf-s are fragile to read (sais Claude)

OK, So where are we?

* It seems BAS free specifications are in fact worthless?
* At best, they can be used to get the names of BAS accounts?
* But no meta data about counter accounts, account types etc.
* So it seems for now totally pointless to try and download and parse them?
    * Also, previous cratchit::zeroth also did NOT extract any meta data from these BAS specs.
* Does the SIE file in fact specify BAS account names?
    * YES!

But it irritates me that I fail to get any specs on BAS account class 8 account types!

* I can buy the book "Svensk Redovisning. Bokföringsboken 2026" and read about it?
* I could then manually create a meta-data map for BAS accounts?
    * I may then be able to manually define the account types for BAS account class 8 accounts?

OK, I bought it as e-book.

* It does NOT state the account types in account class 8!
    * Do I have to deduce the type from the account name?

I have now studied BAS vs SRU vs INK2.

* It seems the information in the SIE-file is self contained and sufficient?
    * Each BAS account is mapped to SRU code
* And maybe the INK2 fields are always defined by their SRU codes?
    * So as long as BAS vs SRU is correct, then INK2 from SRU will also be correct?

Also, for account class 8.

* The BAS Account to SRU code is sign dependant!
    * So if the BAS account sums to a positive value it links to one SRU code
    * But if it is negative it links to another SRU code!
    * E.g., BAS account 8810

    ```
    7420    3.21    Aterföring av periodiseringsfond    8810 (Om netto +), 8819
    7525    3.22    Avsättning till periodiseringsfond  8810 (Om netto -), 8811
    ```

    * 8810, if saldo positive, accumulates to SRU 7420 (INK 3.21)
    * 8810, if saldo negative, accumulates to SRU 7525 (INK 3.22)

So how does the SIE file (My Fortnox export) handle this?

* In Fortnox BAS 88100 seems linked to SRU 7420 ?!
* And YES, in _sie_in/TheITfiedAB20261019_105324.se the mapping is '#SRU 8810 7420'!

HOW ON EARTH IS THIS SUPPOSED TO WORK??!!

* AHA! You are supposed to use explicit BAS 8819 or 8811 as an accountant ?!!
* YES. In SIE file '#SRU 8811 7525'!

OK, maybe time to see if we can implement some basic accounting?

* It seems Fortnox applies (suggests?) a list of pre-defined series to enter vouchers?

```
A - Redovisning	
B - Kundfakturor	
C - Inbetalningar från kunder	
D - Leverantörsfakturor	
E - Utbetalningar till leverantörer	
F - Kassa	
G - Avskrivning	
H - Periodisering	
I - Bokslut	
J - Kontantfaktura	
K - Kvitto	
L - Lön	
M - Momsrapport	
N - Lager
```

After some forth and back I made Claude implement accounting vouchers.

* I now discover we lack 'open fiscal year' (should list option 0,1,2,... for one key select current, previous, before previous...)

I made Claude fix that.

**Todo SIE Import, New voucher, VAT support, Template voucher from history**

* I now discovered I would like import SIE file to list available SIE files if the path is a folder and I press enter
    * I would like each SIE file to be read for financial year and show at least that in the options list
    * A fancy 'latest of same year' filter would be nice to.
* I think new voucher y/n after Heading Amount Date is redundant and can be removed.
    * I imagine if the input could not be parsed we can guide the user to edit?
* The Series selection can be clearer by listing the default with a '*' prefix?
* When choosing posting with a word or number and get account listing we can enhance the user interface.
    * We can 'drill down' with more user input (match only current list moving on)?
    * We can provide bullet list with single letter from first word of listen account names?
    * We can drill down from user entering part or whole account number (reduce list to matching input number)?
    * Also, entering a number should math from beginning of account number (not 'contains' as now )
* We need VAT support
    * Can we detect if we select a VAT account?
    * Can we then also detect VAT percentage?
    * Then we can propose a VAT
    * Then maybe this is overkill as invoices always states VAT?
* Cratchit did not understand to switch to Credit for VAT
    * If Debit matches input amount, Cratchit can switch to assuming remaining Credit to be posted?
    * The opposite should also be true?
* I want some hints to what accounts have been used in historic vouchers of same type?
    * Provide the user with an initial list of 'templates' to mirror account postings from?
    * On empty enter, provide a list of accounts posted to in vouchers of 'same form'?
* I can't, don't know how to (and get no help on how to) exit from 'post' state?
* Ctrl-C exists without safety net!
    * Is it possible to catch Ctrl-C and auto-save?

## 20261021

Now it is time to introduce the scaffolding for character set encoding and decoding.

* I want character set transcoding to be based on a stack-like buffer with a 'push' operation.
* Two buffers are required to transcode from one character set encoding to another.
* One character set to Unicode buffer that accepts push of bytes from the input character set encoding and produces Unicode code code points as they are available.
* One Unicode to character set buffer that accepts push of Unicode code points and produces an std::array of multi byte encoding in the encoded character set.
* The character set to Unicode buffer 'push' returns an optional Unicode code pointer to signal when there is one available in the buffer.
* The Unicode to character set buffer 'push' returns a single byte or an std::array as required to return single or multi byte encoding for the target character set encoding.

I want to try this architecture out for the CP437 character set.

* I want the code places in namespace 'charset::cp437'
* I want to first implement CP437 to Unicode based on this design.

What can Claude propose as an implementation for this design?

Claude implemented CP437 to Unicode buffer for me.

* Now I want Claude to take this design an implement Unicode to UTF8 for me.

I have now iterated some task steps and think I have a character set transcoding architecture in place.

* I cleared Claude and asked fresh about some clean boundary between internal character set string encoding and terminal io.
* And Claude came back and said - Not so!
* It then proposed several aspects to attend to.

So I decided to continue the 'vibe coding' approach and let it do its thing as it found 'best' based on my asking for the attention and design goals.

Now I have reviewed the change Claude proposed and I do no longer agree.

* It identified a problem with UTF8 to Unicode where a broken sequence may result in one unknown character (the broken escape) and one potentially valid one (the sequence breaking byte).

```sh
The problem is narrower. It's the case where the byte that reveals an error is itself a valid character:

bytes:   E2      82      61 ('a')
push:    nothing nothing ?

After E2 82, the decoder is waiting for a third byte. Then a arrives. It isn't a continuation byte, so two things are now true:

1. E2 82 was a broken sequence, so the output owes a U+FFFD.
2. a is a perfectly good character, so the output owes an a.
```

* But on second thought I do not believe returning both the unknown character and the breaking one as a valid one is the way to go?
* I think I will revert Claudes fixes and ask for a clarification in code that in this case the breaking sequence shall consume all three into an unknown character?
* I mean, We AER assuming it is UTF-8. So if we find an invalid sequence, then the whole sequence can be mapped to an unknown char (assuming the breaking char is itself a valid char is NOT automatically a good resolution)?

I started fresh again and asked Claude for this UTF8 to Unicode solution.

* It now again commented on throwing away a valid character that breaks an escape sequence.
* It states the Unicode standard recommends to produce two Unicode code points invalid-char + valid-char.
* I still disagree based on my argument above.

I think I will try to make Claude keep the breaking char if it is a new sync char?

* Then we consume the invalid sequence into the invalid char
* And we start a new escape sequence in the buffer.

What do Claude say?

DARN! Claude has gone off the rails without me noting it.

* It has implemented full to_unicode(std::string_view,&UnicodeBytes)
* This iw WRONG for two reasons.
    1. I do NOT want out-parameters (Result should always come from function return value)
    2. I never intended Unicode to be used in buffering (only as intermediate code points) used by transcoding buffers.

I wonder if I can trust Claude to reactor this away? Or if the to_unicode architecture is already manifested in to much code?

I think I made Claude correct this error for now. At any case, I did what I can for now and want to proceed.

I now think I need a better harness in the generated code.

* I think I will ask Claude to add spdlog with my preferred facade LOG_BUSINESS, LOG_DEVELOPMENT_TRACE and LOG_DESIGN_INSUFFICIENCY.
* I think I will ask Claude to remember to NEVER use out parameters?
* I think I will ask Claude to NEVER implement code that is not explicitly required by implemented functionality or asked for by me?

I now vibe coded UTF8 to Unicode and made Claude configure for all-warnings-as-errors.

So now I have iterated to having 'open/ create project' based on uuid. 

* Seems to work just fine?
* Although allows for path with spaces (should use lowercase and '_' for CLI convenience?)

## 20261019

So this is take three to try and 'vibe code' a Swedish book keeping app in modern C++23.

This time I will try to 'drop feed' claude to get the code I can trust and want to maintain and use.

1. I provide a small task to Claude that shall produce a small output
2. I review, test run, tweak and commit the code and files
3. I update the input information that Claude can use for the next task
4. I repeat 1..4 until I have the app I want.

I started Claude but the sandboxing did NOT work.

* It quickly read the previous sessions for cratchit development.
* So now the context window is poisoned with presumptions from those session.

I will try to restart claude with NO permissions to read outside the sandbox!

This worked fine so far.

* I asked Claude for what files it proposed as 'context' during development.

```sh
.
├── CLAUDE.md
├── architecture.md
├── brief.md
├── conventions.md
├── decisions.md
├── domain.md
└── task.md
```

I now tried to edit these document. But this is already too complicated!

* The Claude.md is fine (the global genera guard rails about sandboxing and iterating small tasks based on listed md-files)
* The task.md is overkill. I enter the task in Claude Code as we go.

You know what. I chat with Claude to tweak the set of documents.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/60cae9fe % tree -L 1
.
├── CLAUDE.md
├── progress.md
└── session.md
```

MUCH BETTER!

So the first task is to create the scaffolding for a cross platform cmake based modern C++ application. It shall implement all functionality natively or consume from conan package manager when appropriate.

Claude asked me some clarifying questions and then created the scaffolding just fine.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/60cae9fe % tree
.
├── CLAUDE.md
├── cratchit
│   ├── CMakeLists.txt
│   ├── CMakeUserPresets.json
│   ├── README.md
│   ├── conanfile.py
│   ├── src
│   │   ├── core
│   │   │   ├── version.cpp
│   │   │   └── version.hpp
│   │   └── main.cpp
│   └── test
│       ├── CMakeLists.txt
│       └── test_version.cpp
├── progress.md
└── session.md

5 directories, 12 files
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/60cae9fe % 
```