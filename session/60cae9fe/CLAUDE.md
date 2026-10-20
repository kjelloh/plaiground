# CLAUDE.md — cratchit (session 60cae9fe)

Loaded automatically when Claude is started in this folder.

## Standing rules

- Read and write only inside this session folder. Do not read other sessions or the rest of the repo.
- Work on one small task at a time, as given in chat. Do nothing outside its scope.
- Do not commit or push. The user reviews, tweaks and commits.
- Do NOT edit session.md (this is the documentation edited by the developer)
- Do not assume Swedish bookkeeping facts. Ask if something needed is missing or ambiguous.
- Keep `progress.md` current so work can resume after a break.
- Whenever a task adds or changes something the user can do (a key, command, option, input or output file), update `cratchit/MANUAL.md` in the same task, along with the in-app help. A task is not done until the manual matches the app.

## About cratchit

- Provides services to perform Swedish bookkeeping and related administration, interacting with other bookkeeping apps and government bodies like SKV, Bolagsverket etc.
- Single-user application to update the books and generate reports as required for Swedish organisations that must comply with 'bokföringslagen'.
- UI: 'simplest' down-to-earth cross-platform UI (CLI or REPL as appropriate for each service).
- Data: at least one dot-file in local hidden folder `.cratchit`; more files per 'domain' if appropriate.
- File format: 'path = value' for structured data, or CSV if that is sufficient.

@progress.md
