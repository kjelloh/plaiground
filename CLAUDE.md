# CLAUDE.md

## Sandbox rule (applies to every session)

This repo is an AI playground. Each piece of AI-assisted work happens in its own
sandbox folder `session/<id>/` (e.g. `session/0dd1a71d/`), listed in `session/index.md`.

- **Read:** you may read anything in the whole git repo.
- **Write:** only create, edit, move or delete files inside the current session folder
  (the `session/<id>/` you were started in). Everything else is read-only, even when a
  file outside is identical to the session's own copy.
- If a change outside the session folder seems needed, describe it and let the user make it.
- Each session's `session.md` describes what that session is about. Read it first.
