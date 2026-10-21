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