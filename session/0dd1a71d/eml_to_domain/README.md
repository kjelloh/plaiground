# eml_to_domain — user manual

Turn exported mail files (`.eml`) into markdown documents in a *domain* of
your choice, e.g. `mail/`.

```
folder of .eml files ──emls_to_domain.py──▶ <repo>/<domain>/<hash>/<domain>.md
```

## Requirements

* Python 3.10 or newer. Standard library only, nothing to install.
* A **target repo** folder holding the two scaffolding scripts the mail
  scripts call. Copy them from [site_repo/](../site_repo/):
  * `init_new.py` creates each entry folder (named after a hash of the
    heading) and its markdown file.
  * `update_index.py` writes `<domain>/index.md`. If it is missing, entries are
    still created, but you get a warning and no index.

## Quick start

```sh
# From this folder, import a whole folder of mails into the domain "mail"
python3 emls_to_domain.py ~/Desktop/exported_mails ../site_repo --domain mail
```

Result:

```
../site_repo/
├── init_new.py
├── update_index.py
└── mail/
    ├── index.md              ← one link per entry
    ├── 1afd4852/
    │   ├── mail.md           ← the mail as markdown
    │   ├── mail.txt          ← the mail's own plain text (if it had one)
    │   └── En timme från Stockholm med bil.pages   ← attachments / images
    └── 5135dc9a/
        └── ...
```

## The scripts

### emls_to_domain.py: import a folder of mails

```
python3 emls_to_domain.py <eml_dir> <repo> --domain <name>
```

| Argument | Meaning |
|---|---|
| `eml_dir` | Folder of `.eml` files. Every `*.eml` in it is processed; other files are ignored. Subfolders are not searched. |
| `repo` | Base folder of the target repo (holds `init_new.py` and `update_index.py`). |
| `--domain` | **Required.** Domain (folder) to import into, e.g. `mail`. Must be a simple name: letters, digits, `_`; no spaces or `/`. It is lower-cased. There is no default. |

The script prints one line per mail, then a summary:

```
ADD: TODO_ Wrap up TestBench.eml
UPDATE: Todo_ KoH-Innovation - ... 2016? 4.eml
SKIP: Todo_ KoH-Innovation - ... 2016? 2.eml (superseded by an equal-or-newer mail with the same subject)
FAIL: broken.eml
    └── NoRenderablePartError: ...

9 mail files -> mail: 7 added, 1 updated, 1 skipped (superseded), 1 failed
Updated 'mail/index.md' with 8 entries.
```

| Status | Meaning |
|---|---|
| `ADD` | First mail with this Subject, so a new entry was created. |
| `UPDATE` | An entry for this Subject existed, and this mail's `Date:` header is **later** than the entry's `*As of ...*` date, so the entry was replaced. |
| `SKIP` | An entry for this Subject already has an `*As of ...*` date as late as this mail's, or later (from a newer mail or from your own edit). Nothing changed. |
| `FAIL` | The mail could not be converted (e.g. it has no text, HTML or image content). The run continues with the next mail. FAIL lines go to stderr. |

`index.md` is only rewritten when something was added or updated, so a
run with no changes leaves it untouched.

### eml_to_domain.py: import a single mail

```
python3 eml_to_domain.py <file.eml> --domain <name> [-o <repo>]
```

| Argument | Meaning |
|---|---|
| `file.eml` | The mail to import. |
| `--domain` | **Required.** Same rules as above. |
| `-o`, `--out-dir` | Base folder of the target repo. Default: the current folder. |

It prints `<domain> -> <path to the new .md>` on success. If the mail is
superseded (see `SKIP` above) it exits with an error message and changes
nothing.

### Helper scripts

`eml_to_html.py`, `html_to_markdown.py` and `eml_to_txt.py` are the
conversion stages `eml_to_domain.py` uses. They can also be run on their own
for debugging (`python3 <script> --help`). You don't need them for normal use.

## How mails become entries

* **Keyed by Subject.** The entry folder is named after a hash of the mail's
  `Subject:` header, not of the file name. So `todo.eml`, `todo 2.eml`,
  `todo 3.eml` (Apple Mail's names for several exported mails with the same
  subject) all map to **one** entry. A mail without a subject gets the
  heading `(no subject)`.
* **Newest wins.** Of several mails with the same Subject, the one with the
  latest `Date:` header is kept, whatever order the files are processed in
  and across runs. A mail without a parsable date never replaces one that
  has one. The entry's date is read back from its `*As of ...*` line (see
  [Entry layout](#entry-layout)), to the minute: a mail from the same minute
  is not newer.
* **Hand edits are protected by their date.** If you edit an entry, set its
  `*As of ...*` line to the time of your edit. Only a mail dated later than
  that replaces the entry (and your edit).
* **Incremental.** Re-running on the same folder, or on a folder with a few
  new exports added, only adds or updates what changed. You never need to
  clear the domain first.
* **Nothing is ever removed.** Deleting an `.eml` file does not delete its
  entry. To remove an entry, delete its `<domain>/<hash>/` folder and re-run
  `update_index.py <domain>` from the repo folder (or just run another import
  that adds something).
* **One domain at a time.** Importing the same mails into two domains gives
  two independent copies with the **same** hash folder names. The hash
  depends only on the Subject.

## Entry layout

`<domain>/<hash>/<domain>.md` looks like this:

```
# Todo: Build House - Consider to use google maps ...

*As of 2019-07-20 14:31*

[mail plain/text](mail.txt)

(Liquid "raw" tag)
...the mail body as markdown, image links pointing to files in the folder...
## Attachments
* [En timme från Stockholm med bil.pages](En timme från Stockholm med bil.pages)
(Liquid "endraw" tag)
```

| Line | Content |
|---|---|
| 1 | `# ` + the mail's Subject (`update_index.py` uses it as the link text; the folder name is the first 8 characters of its md5 hash). Don't edit it: the folder is found by hashing it. |
| 3 | `*As of yyyy-mm-dd hh:mm*`: the date of the entry's state, shown in italics on the page. On import it is the mail's `Date:` header, the time as written in the mail, without seconds or timezone. Used to decide whether a later mail is newer, so update it when you edit the entry. Mails without a parsable `Date:` header get no such line. |
| 5 | `[mail plain/text](mail.txt)`, only present when the mail had a plain-text body. |
| rest | The body, wrapped in Liquid `raw` … `endraw` tags so that brace sequences in mail text (C++ code, templates, ...) can't break a Jekyll site build. |

`mail.txt` is the mail's own `text/plain` part, kept verbatim (UTF-8
with BOM so browsers show non-ASCII characters correctly). It is always
named `mail.txt`, whatever the domain, so it is just another file linked
from the entry, and other mechanisms (e.g.
[domain_to_domain](../domain_to_domain/README.md)) need not know it came
from a mail. HTML-only mails have no `mail.txt`. Inline images and
attachments are saved next to the `.md`; an attachment that would clash
with `mail.txt` or `<domain>.md` (case-insensitively) gets a `-1`, `-2`, ...
suffix instead, e.g. `mail-1.txt`.

> **Upgrading entries from earlier layouts** (an invisible
> `<!-- mail-date: ... -->` comment, or an even older bare date line, instead
> of the `*As of ...*` line): no action needed. Such entries read as undated,
> so the next import of their mails rewrites each one once in the new layout
> (expect every entry to be reported as `UPDATE` that one time). The newest
> mail per Subject still wins.

## Typical workflow

```sh
# Once: set up a target repo
mkdir -p ~/my_site && cp ../site_repo/init_new.py ../site_repo/update_index.py ~/my_site/

# Whenever you have exported new mails
python3 emls_to_domain.py ~/Desktop/exported_mails ~/my_site --domain mail
```

## Next step

To pick entries from the imported domain into another domain, see
[domain_to_domain](../domain_to_domain/README.md).

## Running the tests

The tests need `pytest` (e.g. via the repo's `init_python_tool_chain.py`):

```sh
python3 -m pytest          # in eml_to_domain/ and site_repo/
```

One test reads the sample mails in `../../afaa732d/example_eml`.
