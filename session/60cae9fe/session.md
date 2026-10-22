# Vibe code a Swedish book keeping app named cratchit - take 3 (Drop feed Claude)

```#60cae9fe```

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