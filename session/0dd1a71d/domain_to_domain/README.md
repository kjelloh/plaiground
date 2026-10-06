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
```

| Argument | Meaning |
|---|---|
| `init` \| `add` | **Required first argument**, the mode (see below). Anything else is an error. |
| `repo` | Base folder of the target repo. |
| `source` | Domain to pick from. **Required.** Must exist. |
| `target` | Domain to add to. **Required.** Must differ from `source`. |
| `--pick` | The pick list (see below). **Required for `add`**, optional for `init`. |
| `--dry-run` | Only report what would happen; change nothing. |

### Modes

| Mode | Use it when | Fails if |
|---|---|---|
| `init` | Creating the target domain for the first time, with or without a pick list. | `<repo>/<target>/` already exists, and suggests `add`. |
| `add` | Adding more entries to an existing target domain. | `<repo>/<target>/` doesn't exist, and suggests `init`. |

Apart from that check, both modes do the same thing: copy the picked entries
that aren't in the target yet and leave everything else alone.

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

Edit entries in `~/my_site/todo/` as you like: later `add` runs skip them.

## Running the tests

The tests need `pytest`:

```sh
python3 -m pytest          # in domain_to_domain/
```
