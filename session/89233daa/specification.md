# pyroceph specification

What pyroceph shall do, as agreed so far. Updated whenever a design question is settled.

- `session.md` is the original brief and stays as written.
- `progress.md` tracks the build steps and what the code does today.
- Each rule has an ID that stays the same, so tests and discussion can refer to it.
- Rules marked *Wish* are wanted if possible, but not yet committed to.

## 1. Purpose

pyroceph is a console TUI engine. The user navigates a DAG of view states with single key presses.
The engine does not know what the states or their data mean. Its client supplies them.

The name means "head on fire": the user is always at the head of a stack of states, and the head is
where the light is.

## 2. Terms

| Term | Meaning |
|------|---------|
| View state | One node in the client's DAG. It provides a data view, options and a prompt. |
| Stack | The states entered so far. The top of the stack is the head. |
| Option | A numbered choice in the head state, selected with one digit key. |
| Input buffer | Text the user has typed, held as Unicode code points. |
| Buffer edit mode | The UI mode while the input buffer holds text. |
| Model | The single value that holds the whole app state. |
| List state | A view state that shows a list of items as numbered options. |
| Drill-down | Picking a list item one decimal digit at a time, most significant digit first. |

## 3. Navigation

- **NAV-1** Each key is processed at once, with no Enter needed.
- **NAV-2** A digit key that matches an option in the head state acts on it right away.
- **NAV-3** Entering a state pushes it onto the stack.
- **NAV-4** `-` leaves the head state and returns to the previous state.
- **NAV-5** `q` quits the application.
- **NAV-6** Each state has 0..n options. The client designs its DAG so that non-list options fit digits 0..9.

## 4. Screen

- **SCR-1** The screen shows, top to bottom: the state's data view, the options, the input prompt.

## 5. Input buffer

- **BUF-1** A key that is not an option goes into the input buffer.
- **BUF-2** The UI is in buffer edit mode while the buffer holds text.
- **BUF-3** In buffer edit mode, the next key adds a character.
- **BUF-4** Delete removes the last character.
- **BUF-5** A clear key empties the buffer. Which key is still to be chosen (Q6).
- **BUF-6** Enter posts the buffer to the current view state.
- **BUF-7** The buffer operates on Unicode code points.

## 6. Lists and drill-down

- **LST-1** A list state shows its items as numbered options. The engine does this from the item list or item count that the client provides.
- **LST-2** 1..10 items show as items 0..9.
- **LST-3** More than 10 items show as groups, one significant digit at a time.
- **LST-4** Each digit picks a group, from the most significant digit down, until one item is picked.
- **LST-5** Each drill-down digit is one stack level, so `-` steps back one digit.

Example with 25 items, numbered 00..24 (the display format is still open, see Q5):

```text
0  items 00..09
1  items 10..19
2  items 20..24
```

Pressing `1` then shows items 10..19 as options 0..9, and pressing `7` picks item 17.

## 7. Architecture

- **ARC-1** The client configures the engine with its view states. The engine stays indifferent to what they contain.
- **ARC-6** pyroceph code is kept separate from the console app code.
- **ARC-7** pyroceph acts on one key at a time, given as a Unicode code point.
- **ARC-8** pyroceph administers the input buffer when a key does not trigger an option.
- **ARC-9** The code starts with no client API: only `pyroceph::core::Orchestrator`, which decides what a key means, and `pyroceph::tui::KeyLoop`, which handles the console. How a client injects behaviour is parked, and the TEA wishes below wait until that is worked out.
- **ARC-10** `core` knows nothing about the terminal or the screen. `tui` depends on `core`, never the reverse.
- **ARC-2** *Wish:* the engine fits into `update` in The Elm Architecture, with a single Model holding the whole app state. View states also operate on a Model instance.
- **ARC-3** *Wish:* the Model is built on `immer` persistent data types, so copying a Model is cheap.
- **ARC-4** *Wish:* stored Models give a state history, with keys for undo and redo.
- **ARC-5** `-` changes to the previous state, while undo restores the whole Model. Both update the view.

Parked for now: ARC-1 and every other rule that needs a view state or a client. These rules remain the goal, but are not being built yet.

### Current types

```cpp
namespace pyroceph::core {

enum class KeyResult { Continue, Quit };

class Orchestrator {
public:
  KeyResult on_key(CodePoint key);
};

} // namespace pyroceph::core

namespace pyroceph::tui {

class KeyLoop {
public:
  explicit KeyLoop(core::Orchestrator& orchestrator);
  int run(); // returns the process exit code
};

} // namespace pyroceph::tui
```

## 8. Platform and build

- **PLT-1** Modern C++ console app. The code currently uses C++23.
- **PLT-2** Cross-platform, built with CMake and Conan.
- **PLT-3** Keys are handled as Unicode code points.
- **PLT-4** The app detects the text encoding of the current platform and runtime (see `60cae9fe/cratchit/src/core/terminal_encoding.*`).
- **PLT-5** Transcoding code is copied into this session, only the files needed (from `9fe36d28/walk_pdf/src`).
- **PLT-6** Raw key reading is plain code (termios / conio), with no Conan package.

## 9. Open questions

Each proposal below is Claude's suggestion and not yet agreed. A settled question becomes a rule above.

| # | Question | Proposal |
|---|----------|----------|
| Q1 | Do `q`, `-` and digits act while the buffer holds text? Must be settled before the buffer step. | No. They are typed characters in buffer edit mode. Only Enter, Delete and the clear key are special there. |
| Q2 | With an empty buffer, can the user type text that starts with `q`, `-` or a digit that matches an option? | No. Those keys always act first, so such text cannot be started. A digit that matches no option starts the buffer (BUF-1). |
| Q3 | What does `-` do in the root state? | Nothing. Only `q` leaves the app. |
| Q4 | How do non-character keys behave, such as arrows and function keys? On macOS and Linux an arrow arrives as three code points (`ESC`, `[`, `A`), which would land in the buffer. On Windows the code skips them. | Ignore them on all platforms for now, and never let them reach the buffer. |
| Q5 | In a drill-down, are item numbers zero-padded, and what happens to a group with a single item, like item 20 of 21? | Pad to the width of the highest index. A group with one item is picked at once, with no extra digit. |
| Q6 | Which keys are clear, undo and redo? They must not take away common typed characters. | Control keys: `Ctrl-U` clear, `Ctrl-Z` undo, `Ctrl-Y` redo. `Ctrl-Z` is end of input on Windows and suspend on Unix today, so this needs handling in the terminal code. |
| Q7 | *Parked.* What does the client get when an item is picked or the buffer is posted, and can it change the Model and the next state? | To be designed in the client API step. |
| Q8 | *Parked.* What does a client write: view states as data, as functions over the Model, or both? | Decide after Q13 has been tried in code. |
| Q9 | Does the TEA `update` also return commands for side effects, such as file reads? | Decide when TEA is taken up (ARC-9). |
| Q10 | Does the engine check that the client's graph has no cycles? | No. The stack works anyway, so the client answers for its own graph. |
| Q11 | Is each frame drawn by clearing the screen or by appending below the last one? | Clear and redraw, so only the head is on screen. |
| Q12 | Does the undo history survive a new action after an undo, and does it survive quitting? | A new action discards the redo branch. History lives only in memory. |
| Q13 | *Parked.* How does pyroceph find out whether a key is an option? | It asks the view state, through a function like `bool try_option(CodePoint key)` that the client implements. The state acts on the key itself, so pyroceph never sees what an option does. |
