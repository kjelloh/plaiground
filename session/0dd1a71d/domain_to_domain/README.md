# domain_to_domain — user manual

Pick entries from one *domain* of a target repo (e.g. `mail/`, filled by
[eml_to_domain](../eml_to_domain/README.md)) and add them to another domain
(e.g. `todo/`).

```
<repo>/<source>/<hash>/<source>.md ──domain_to_domain.py──▶ <repo>/<target>/<hash>/<target>.md
```

The tool only ever **adds** to the target: entries already there are never
replaced or removed. That means you can edit entries in the target domain
and still add more picks later without losing your edits.

What gets copied is always opt-in: only entries named in a pick list. To get
a pick list, `diff` shows what is in the source but not yet in the target,
optionally leaving out an ignore list of entries you never want. The tool
writes no files of its own; the only file worth keeping is your ignore list.

## Requirements

* Python 3.10 or newer. Standard library only, nothing to install.
* A **target repo** folder holding the two scaffolding scripts. Copy them from
  [site_repo/](../site_repo/):
  * `init_new.py` (**required**): its hash function names the entry folders.
  * `update_index.py`: writes `<target>/index.md`. If it is missing, entries
    are still added, but you get a warning and no index.

## Usage

```sh
python3 domain_to_domain.py init <repo> <source> <target> [--pick <pick.md>] [--dry-run]
python3 domain_to_domain.py add  <repo> <source> <target>  --pick <pick.md>  [--dry-run]
python3 domain_to_domain.py diff <repo> <source> <target> [--ignore <ignore.md>]
```

| Argument | Meaning |
|---|---|
| `init` \| `add` \| `diff` | **Required first argument**, the mode (see below). Anything else is an error. |
| `repo` | Base folder of the target repo. |
| `source` | Domain to pick from. **Required.** Must exist. |
| `target` | Domain to add to. **Required.** Must differ from `source`. |
| `--pick` | The pick list (see below). **Required for `add`**, optional for `init`. |
| `--dry-run` | `init` / `add` only: report what would happen; change nothing. |
| `--ignore` | `diff` only: ignore list of source entries not to show (see below). |

### Modes

| Mode | Use it when | Fails if |
|---|---|---|
| `init` | Creating the target domain for the first time, with or without a pick list. | `<repo>/<target>/` already exists, and suggests `add`. |
| `add` | Adding more entries to an existing target domain. | `<repo>/<target>/` doesn't exist, and suggests `init`. |
| `diff` | Finding out what is left to pick. Changes nothing. | `<repo>/<target>/` doesn't exist, and suggests `init`. |

Apart from that check, `init` and `add` do the same thing: copy the picked
entries that aren't in the target yet and leave everything else alone.

`init` **without** `--pick` creates an empty target domain: just the folder
`<repo>/<target>/` and an empty `index.md`. Fill it later with `add`.

If an `init` run ends up adding nothing (e.g. every pick was a typo), the
empty target folder is removed again, so you can fix the pick list and simply
rerun `init`.

### The pick list

A plain text or markdown file in the same line format as `<source>/index.md`,
so you can copy lines straight from the source index:

```markdown
# Things to publish
* [Todo: Wrap up TestBench](1afd4852/mail.md)
* [Todo: Consider to learn regular expressions?]()
```

* An entry is matched by the folder hash in its link (`1afd4852`).
* A hand-written `* [Exact heading]()` also works: it is matched by the hash
  of the heading.
* Other lines (headings, notes, blank lines) are ignored.

The pick list can keep growing: lines whose entries are already in the target
are just reported as `SKIP`.

## Report

Each pick gets one line, followed by a summary:

```
ADD: todo/1afd4852  Todo: Wrap up TestBench
SKIP: todo/7c0e2b19  Todo: Consider to learn regular expressions? (already in target)
UNMATCHED: * [Todo: Wrap up TestBnch]()

mail -> todo (add), 3 pick entries: 1 added, 1 skipped, 0 failed, 1 unmatched
```

| Status | Meaning |
|---|---|
| `ADD` | Picked and not yet in the target, so copied. With `--dry-run`: `WOULD ADD`. |
| `SKIP` | Picked but already in the target, so left as is. `source differs` means the source entry has changed since it was copied (e.g. a newer mail was imported); the target copy is still kept. |
| `UNMATCHED` | Pick list line that matches no source entry (typo, stale line). |
| `FAIL` | Source entry whose heading doesn't hash to its folder name (e.g. a hand-edited heading). Not copied. |

* The exit status is **1** if any pick is `UNMATCHED` or `FAIL`. The picks
  that did work are still added.
* `<target>/index.md` is refreshed only when something was actually added.

## diff and the ignore list

`diff` prints every source entry that isn't in the target yet as a pick list
line on **stdout**, in index order. So its output can be used as a pick list
as is:

```sh
python3 domain_to_domain.py diff ~/my_site mail chime > pick.md
```

With `--ignore`, entries in the ignore list are left out too. The ignore
list has the same format as a pick list: entries are matched by the hash in
their link (so `…/chime.md` links work against `mail`), and headings, notes
and blank lines are ignored. Use it when what you want far outweighs what you
don't:

```markdown
# Brf Ekbladet
* [Todo: Brf Ekbladet - mail från Åke om beträda gräsmattor](31605a2f/chime.md)
```

The ignore list only filters what `diff` shows. `add` never reads it, so it
can never cause anything to be copied. Keep it wherever you like; the tool
never writes it.

Notes and a summary go to **stderr**, so they never end up in the pick list:

```
STALE IGNORE: * [Todo: Something long gone]()
mail -> chime (diff): 2942 pending, 262 ignored, 0 in target
```

| Note | Meaning |
|---|---|
| `IGNORED BUT IN TARGET` | Ignore list entry that is already in the target, so the two disagree. |
| `STALE IGNORE` | Ignore list line that matches no source entry (typo, entry gone). |
| `TARGET ONLY` | Target entry with no matching source entry. |
| `WILL FAIL` | Pending source entry whose heading doesn't hash to its folder name, so `add` would report `FAIL`. |

Notes don't affect the exit status.

## What a copy is

The whole entry folder (markdown, plain text, images, attachments) is copied
to `<repo>/<target>/<hash>/`, with:

* `<source>.md` / `<source>.txt` renamed to `<target>.md` / `<target>.txt`
* the `[...](<source>.txt)` link in the markdown rewritten to `<target>.txt`

The folder hash depends only on the heading, so an entry keeps the same hash
in every domain. The source domain is never modified, and folders in the target
that aren't entries (e.g. `notes/`) are left alone.

## Typical workflow

Run from this folder (`domain_to_domain/`):

```sh
# Once: set up a target repo and import your mails
mkdir -p ~/my_site && cp ../site_repo/init_new.py ../site_repo/update_index.py ~/my_site/
python3 ../eml_to_domain/emls_to_domain.py ~/Desktop/exported_mails ~/my_site --domain mail

# Once: create the target domain from a first pick list (preview first)
grep -i "testbench" ~/my_site/mail/index.md > ~/my_site/pick.md
python3 domain_to_domain.py init ~/my_site mail todo --pick ~/my_site/pick.md --dry-run
python3 domain_to_domain.py init ~/my_site mail todo --pick ~/my_site/pick.md
#   ...or start with an empty target domain instead
python3 domain_to_domain.py init ~/my_site mail todo

# Later: add more lines to the pick list and add them
$EDITOR ~/my_site/pick.md
python3 domain_to_domain.py add ~/my_site mail todo --pick ~/my_site/pick.md
```

Or, when you want most of the source and keep a list of what you don't:

```sh
python3 domain_to_domain.py init ~/my_site mail chime
python3 domain_to_domain.py diff ~/my_site mail chime --ignore ignore.md > pick.md
$EDITOR pick.md                    # optional: review; move unwanted lines to ignore.md
python3 domain_to_domain.py add  ~/my_site mail chime --pick pick.md --dry-run
python3 domain_to_domain.py add  ~/my_site mail chime --pick pick.md

# Any time later, e.g. after importing new mails: see what is left to decide on
python3 domain_to_domain.py diff ~/my_site mail chime --ignore ignore.md
```

`pick.md` is throwaway: `diff` recreates it from the domains whenever you need
it.

Edit entries in `~/my_site/todo/` as you like: later `add` runs skip them.

## Running the tests

The tests need `pytest`:

```sh
python3 -m pytest          # in domain_to_domain/
```
