# Consider to vibe code the pyroceph TUI engine?

```#89233daa```

## 20261105

It is time I think to see if we can vibe code my 'pyroceph TUI engine'.

Here is what I know about the behavior I want to implement.

* I want the user to navigate a DAG of 'view' or 'user' states.
* The user navigates with single-key-direct-action
    * Each key is processed at once
    * A key matching a current option gets acted on right away as defined by the option.
* I want the TUI Engine to be indifferent to what actual states and data there is.
    * Somehow the client of the engine configures the TUI engine with 'view states'
* Entered states 'stack' so that the user can navigate back to previous state.
* This behavior explains the name 'pyroceph' (head on fire, fire head)
    * The user is always at 'the head' of current stack of states
    * The user can navigate back and enter a new path into another branch 'head'
    * 'On fire' can be understood as 'the head is everything', 'the head is the focus', 'the head is where the light is'...
* Navigation is single numerical keys to select next state.
* Navigation shares common keys to leve a state (go back), quit the program etc.
    * 'q' quits the application
    * '-' leaves current state and goes back to previous state.
* Each state shows 0..n options for next state.
* Each state shows a view of current state.
* Each state shows a prompt
* The view is always, top to bottom, current state data view, next state options view, the input prompt.
* Any key press that is not an option goes into the input buffer
* While the buffer contains text the UI is in 'buffer edit mode'
    * Next key adds a character
    * A delete removes last character
    * There is a 'clear' key that when pressed empties the input buffer.
* View states of list-of-items list the items as numbered options.
    * 1..10 items shows as item 0..9
    * More items are shown grouped by modulus 10 one significant digit at a time.
* Modulus 10 view states drills down a factor of 10 (one significant digit a time)
    * The user picks an item by choosing its index one decimal digit at a time, from the most significant digit down.
    * The options are shown 0..9 grouped on current significant digit
    * The concept is a hierarchical numeric menu (digit-by-digit selection)
    * This behavior is implemented by the UI engine based on provided items list (or items count)

I also know something about the technical design.

* The app shall be a modern C++ console app.
* The app shall be cross platform based on cmake and conan package manager.
* The app shall use the text transcoding C++ code in session 9fe36d28 ('../9fe36d28')
* The app shall act on pressed keys as Unicode code points
* The input buffer shall operate on Unicode code points
* The app shall detect the text encoding of the current platform and runtime (see transcoding C++ code)
    * Or see app in session 60cae9fe (../60cae9fe)


I also have a whish list.

* If possible the app shall fit into 'update' in TEA (The elm architecture)
    * It shall operate on a single Model that holds the whole app state
    * And view states are assumed to also operate on a Model instance
* If possible the Model shall be based on the C++ library 'immer' persistent data types
    * With immer data types copy of Model becomes 'cheap'
* With a immer-persistent Model the app can implement a 'state history' and 'time travel'
    * Teh app could provide a key for 'undo' and redo' by traveling the history of stored Model values.

Wow, that was a mouthful! Am I ready to start this journey? What do I tell Claude?

* I want Claude to provide SMALL, easy to verify, development steps.
* I want to be asked to test and verify myself.
* I want to do the commits to the repo myself.
* I want Claude to keep a 'progress.md' to keep track of progress.

Here we go!

Claude have now implemented the first base console app.

* It is a cmake/conan C++ app
* It uses text transcode code from cratchit
* It reads single keys and acts on them by printing Unicode code point.

But then the next step got tricky quickly.

* I wanted separation of pyroceph code from the console app code so we did not mix them up.
* But this exposed the problem of what API we should go for?
* And then a discussion on how much TEA (The Elm Architecture) to consider already at this point?

I decided to start over. We need a much simpler API for now to flesh out how pyroceph can inject the behavior without knowing about whats inside the app state and view states the client provides?

So where can we start?

* pyroceph shall act on a key as a Unicode code point.
* It shall administrate teh input buffer when a key dos not trigger an option

So already here we know pyroceph needs to 'talk' to the current view state if a key is an option?

* I started by asking Claude to initiate a specification.
* And I got [specification](./specification.md)

I have now taken a coffee break and a short run.

* I decided to ask Claude to split the code into one pyroceph 'Orchestrator' and one TUI KeyLoop.
* I am betting this pins down a separation enough for now?

OK. So what is the next design decision to make?

Here are some thinking of mine.

* The orchestrator is to impose the navigation and buffer editing.
* So it needs to be able to map a key to an option.
* It is also to allow the client to configure the DAG of states to navigate.
* So it seems we at least need the concept of a 'path'?

So maybe the orchestrator shall provide the client with means to register a 'state' at a 'path' of the navigable UI?

* We can make pyroceph define a Path type as a vector of Unicode code points.
* Then we can make the orchestrator allow the client to register a 'State' at a 'Path'
* Now, the 'State' need to be a descriptor (We can't store pointers to volatile objects in the client)
* In this way the orchestrator can respond back to the client to enter a new state (and prodive the new state descriptor)

So for a key we now have some things we know can happen.

* If in buffer edit mode: Process the key to mutate the buffer
* If not in buffer edit mode: Pyroceph can extend current path with the key to see if there is a registered next state at that path.
    * If it does it answer back to the client 'new_state path'
    * It also sets this as the current path.
    * Otherwise it stores the key in the input buffer edit mode (enters edit buffer mode)
* The client look up the new state and use it to produce a new view and renders it.

So this did NOT work!

* I vibe coded so the orchestrator could register paths for navigable states.
* And aimed for making the client receive state changes as a path to the new state.
    * The idea was to allow the client to view and render the new state.
    * BUT: The client is UNABLE to render the options list (this is known by pyroceph)

I have to revert and re-define the design.