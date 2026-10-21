# Vibe code a Swedish book keeping app named cratchit - take 3 (Drop feed Claude)

```#60cae9fe```

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