#!/usr/bin/env python3
"""Transform a single .eml file into an entry of a caller-named domain:
<base_dir>/<domain>/<hash>/<domain>.md (+ local images and attachments).

Pipeline: eml_to_html (extract the HTML body + inline images) -> html_to_markdown
(convert to markdown, copy images) -> merge the result into <domain>.md
scaffolded by init_new. The mail's original text/plain body (if any) is also
kept verbatim as <domain>.txt (via eml_to_txt), linked from <domain>.md
right after the date line.

There is no default domain: the caller always names it (e.g. "mail"), so
the same mechanism can fill any namespace of the target repo.

This mechanism is repo-agnostic: it does not bundle its own init_new.py /
update_index.py, it uses whichever copies already live in the target repo
(base_dir) — the same general-purpose doc-scaffolding tools that repo uses
for its other namespaces (todo, note, ...). That's what makes the same
eml_to_domain folder usable, unmodified, against any of those repos.
"""

import argparse
import email
import importlib.util
import re
import shutil
import subprocess
import sys
from datetime import datetime
from email import policy
from pathlib import Path

from eml_to_html import extract as eml_to_html_extract
from eml_to_html import sanitize
from eml_to_txt import plain_text_of, write_plain_text
from html_to_markdown import convert as html_to_markdown_convert

# init_new.py's hash tag line ("#" + hex digest) between heading and date.
HASH_TAG_LINE_RE = re.compile(r"^#[0-9a-f]+$")


class EntrySupersededError(Exception):
    """Raised when an existing entry for this mail's Subject already
    reflects an equal-or-newer mail.

    Mails are keyed by Subject, not filename: Apple Mail's exporter
    appends a disambiguating " 2", " 3", ... to the *filename* alone when
    several exported mails share a subject (the real Subject: header is
    identical across them) — those are different versions of the same
    entry, and only the latest (by Date header) is kept.
    """


def entry_md_name(domain: str) -> str:
    return f"{domain}.md"


def plain_text_name(domain: str) -> str:
    return f"{domain}.txt"


def load_init_new(base_dir: Path):
    """Load base_dir/init_new.py — the target repo's own scaffolding tool,
    not a copy bundled with this mechanism."""
    init_new_path = base_dir / "init_new.py"
    if not init_new_path.is_file():
        raise FileNotFoundError(
            f"{init_new_path} not found — the target repo must have its "
            "own init_new.py (see the plaiground repo root, or site_repo/, "
            "for an example)."
        )
    spec = importlib.util.spec_from_file_location("_target_init_new", init_new_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def update_domain_index(base_dir: Path, domain: str) -> None:
    """Refresh base_dir/<domain>/index.md by running the target repo's own
    update_index.py — mirrors running it by hand from within that repo."""
    update_index_path = base_dir / "update_index.py"
    if not update_index_path.is_file():
        print(
            f"WARNING: {update_index_path} not found — {domain}/index.md not updated.",
            file=sys.stderr,
        )
        return
    subprocess.run(
        [sys.executable, str(update_index_path.resolve()), domain],
        cwd=base_dir,
        check=True,
    )


def subject_of(msg) -> str:
    return str(msg.get("subject") or "").strip() or "(no subject)"


def read_subject(eml_path: Path) -> str:
    msg = email.message_from_bytes(eml_path.read_bytes(), policy=policy.default)
    return subject_of(msg)


def parsed_date_of(msg):
    """The mail's Date header as a datetime, or None if absent/unparsable."""
    date = msg.get("date")
    if date is None:
        return None
    try:
        return date.datetime
    except AttributeError:
        return None


def date_line_of(msg) -> str:
    parsed = parsed_date_of(msg)
    if parsed is not None:
        return parsed.isoformat()
    date = msg.get("date")
    return str(date) if date is not None else "(no date)"


def is_newer(candidate_date, current_date) -> bool:
    """True if candidate_date should replace current_date as the entry's
    source mail. A missing date always loses to a present one; between two
    present dates the later one wins; dates that aren't directly comparable
    (e.g. one naive, one aware) keep the current one rather than crash the
    whole batch over one odd header."""
    if candidate_date is None:
        return False
    if current_date is None:
        return True
    try:
        return candidate_date > current_date
    except TypeError:
        return False


def read_entry_date(entry_path: Path):
    """Read back the date eml_file_to_entry wrote into an existing entry:
    after the "# heading" line, the first non-blank line that isn't
    init_new.py's "#<hash>" tag line. Returns None if there is no such line
    or it isn't a parseable ISO date (e.g. the "(no date)" fallback)."""
    lines = entry_path.read_text(encoding="utf-8").splitlines()[1:]
    for line in lines:
        line = line.strip()
        if not line or HASH_TAG_LINE_RE.match(line):
            continue
        try:
            return datetime.fromisoformat(line)
        except ValueError:
            return None
    return None


def eml_file_to_entry(eml_path: Path, base_dir: Path, domain: str) -> Path:
    """Turn eml_path into an entry folder under base_dir/<domain>/<hash>/,
    keyed by the mail's Subject so later revisions of the same mail replace
    earlier ones rather than piling up as separate entries."""
    init_new = load_init_new(base_dir)
    ensure_entry_folder = init_new.ensure_entry_folder
    msg = email.message_from_bytes(eml_path.read_bytes(), policy=policy.default)
    subject = subject_of(msg)
    candidate_date = parsed_date_of(msg)

    entry_path, created = ensure_entry_folder(domain, subject, base_dir=base_dir)
    entry_dir = entry_path.parent

    if not created:
        if not is_newer(candidate_date, read_entry_date(entry_path)):
            raise EntrySupersededError(
                f"{domain} entry for subject {subject!r} already reflects an "
                f"equal-or-newer mail — {eml_path.name!r} is superseded"
            )
        shutil.rmtree(entry_dir)
        entry_path, created = ensure_entry_folder(domain, subject, base_dir=base_dir)
        assert created, f"just removed {entry_dir}, recreating it should not clash"

    scratch_dir = entry_dir / "_html_scratch"
    md_path = entry_dir / (sanitize(eml_path.stem) + ".md")
    txt_name = plain_text_name(domain)

    try:
        eml_to_html_extract(eml_path, scratch_dir, inject_h1=False)
        html_to_markdown_convert(scratch_dir, entry_dir)

        markdown_body = md_path.read_text(encoding="utf-8")
        md_path.unlink()

        plain_text = plain_text_of(msg)
        if plain_text is not None:
            write_plain_text(entry_dir / txt_name, plain_text)

        date_line = date_line_of(msg)
        with entry_path.open("a", encoding="utf-8") as f:
            f.write(f"{date_line}\n\n")
            if plain_text is not None:
                f.write(f"[Plain text]({txt_name})\n\n")
            # Mail content routinely contains "{{" / "{%" (C++ brace-init,
            # JSON-ish notes, ...) which Jekyll/Liquid treats as template
            # syntax regardless of front matter settings — a malformed one
            # aborts the *entire* site build, not just this page. {% raw %}
            # is Liquid's own mechanism for "don't parse this", built to
            # survive exactly this case (verified: scans for the literal
            # {% endraw %} token rather than tokenizing the content).
            f.write("{% raw %}\n")
            f.write(markdown_body)
            f.write("\n{% endraw %}\n")
    finally:
        if scratch_dir.is_dir():
            shutil.rmtree(scratch_dir)

    return entry_path


def domain_name(text: str) -> str:
    """argparse type for --domain: the same rule init_new.py enforces, but
    reported before any mail is read."""
    domain = text.strip().lower()
    if not domain.isidentifier():
        raise argparse.ArgumentTypeError(
            f"invalid domain {text!r} — use a simple name like 'mail' or 'note'"
        )
    return domain


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("eml", type=Path, help="path to the .eml file")
    parser.add_argument(
        "--domain",
        type=domain_name,
        required=True,
        help="domain (namespace folder) to create the entry in, e.g. 'mail'",
    )
    parser.add_argument(
        "-o",
        "--out-dir",
        type=Path,
        default=Path("."),
        help="base directory of the target repo (default: cwd)",
    )
    args = parser.parse_args()

    if not args.eml.is_file():
        sys.exit(f"Not a file: {args.eml}")

    try:
        entry_path = eml_file_to_entry(args.eml, base_dir=args.out_dir, domain=args.domain)
    except (EntrySupersededError, FileNotFoundError, ValueError) as e:
        sys.exit(str(e))

    update_domain_index(args.out_dir, args.domain)
    print(f"{args.domain} -> {entry_path}")


if __name__ == "__main__":
    main()
