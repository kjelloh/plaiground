# update_domain — user manual

Update the entries of **one** *domain* of a target repo in place (e.g. the
`chime/` domain filled by [domain_to_domain](../domain_to_domain/README.md)).

```
<repo>/<domain>/<hash>/<domain>.md ──update_domain.py──▶ same files, updated
```

## Requirements

* Python 3.10 or newer. Standard library only, nothing to install.

## Usage

```sh
python3 update_domain.py intralink <repo> <domain> [--dry-run]
```

| Argument | Meaning |
|---|---|
| `intralink` | **Required first argument**, the mode (the only one so far). |
| `repo` | Base folder of the target repo. |
| `domain` | The domain to update. **Required.** Must exist. |
| `--dry-run` | Report what would happen; change nothing. |

## intralink — link "Also see" references between entries

Turns references to other entries of the same domain into markdown links,
so you can click from one entry to the other on the site.

A reference is a line where an arrow `==>` is followed by a cue and a
quoted title:

```markdown
==> Also see "Todo: Some other entry?"
==> Se också ”Todo: Någon annan post?”
```

* Arrow: `==>`, also as written in mail-imported text (`==&gt;`) and the
  typos `== >` and `=>=`. Blockquote / emphasis markers (`>`, `*`) before it
  are fine.
* Cue: `Also see`, `See also`, `Se också` or `Se även`, any case, with or
  without a `:`.
* Title: quoted with any quote characters (`"…"`, `“…”`, `”…”`, mixed, or
  even unclosed). The title may hold quotes itself, and a note may follow it.

Anything else is left alone: a cue without an arrow, a cue followed by more
words before the quote, URLs and paths.

Only the title inside the quotes changes, into a link to the entry. The
text stays as you wrote it, typos included:

```markdown
==> Also see "[Todo: Some other entry?](../1a2b3c4d/chime.md)"
```

### How a title is matched

The first match wins:

| Match | The quoted title is … |
|---|---|
| exact | the very same text as an entry's `# heading`. |
| normalized | the same after unescaping HTML (`&lt;` → `<`), unifying quote characters, collapsing whitespace, ignoring case and surrounding quotes / `?` / `.`, and reading `<url>` and `[text](url)` as plain text. |
| loose | the same after also ignoring a leading `Todo:` (or a cut `odo:`). |
| fuzzy | at least 92 % similar, clearly more so than the runner-up, and with the **very same numbers**, so `gcc14` never links to `gcc13`, nor `2016` to `2017`. |

### Safe to re-run

A title already turned into a link is left as is, so running intralink again
only links what is new: references to entries added since the last run, or
ones you fixed by hand. Nothing else in an entry is touched, and line endings
are kept.

The link goes by the target entry's folder, so it keeps working if you later
edit that entry's heading.

### The report

What deserves a look is listed first, each with the file and line to fix by
hand, then a summary line:

| Note | Meaning |
|---|---|
| `FUZZY` | Linked by a fuzzy match. Check it is the right entry. |
| `UNRESOLVED` | No entry matches. Left as is. |
| `AMBIGUOUS` | Several entries match equally well. Left as is. |
| `SELF` | The reference names the entry it is in. Left as is. |
| `BROKEN` | An existing link whose target entry is missing. |

```
chime (intralink): 1462 references: 1386 linked (1317 exact, 60 normalized, 5 loose, 4 fuzzy), 0 already linked, 70 unresolved, 6 self; 497 entries changed.
```

The tool doesn't back anything up. If the domain folder isn't under version
control, copy it first, or start with `--dry-run`.

## Tests

```sh
python3 -m pytest
```
