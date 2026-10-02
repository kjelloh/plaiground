#!/usr/bin/env python3
"""Transform a single .eml file into a chime folder (chime.md + local images).

Pipeline: eml_to_html (extract the HTML body + inline images) -> html_to_markdown
(convert to markdown, copy images) -> merge the result into chime.md scaffolded
by init_new. The mail's original text/plain body (if any) is also kept
verbatim as chime.txt (via eml_to_txt), linked from the end of chime.md.

This mechanism is repo-agnostic: it does not bundle its own init_new.py /
update_index.py, it uses whichever copies already live in the target repo
(base_dir) — the same general-purpose doc-scaffolding tools that repo uses
for its other namespaces (todo, note, ...). That's what makes the same
mail_to_chime folder usable, unmodified, against any of those repos.
"""

import argparse
import email
import importlib.util
import shutil
import subprocess
import sys
from datetime import datetime
from email import policy
from pathlib import Path

from eml_to_html import extract as eml_to_html_extract
from eml_to_html import sanitize
from eml_to_txt import plain_text_of
from html_to_markdown import convert as html_to_markdown_convert

PLAIN_TEXT_NAME = "chime.txt"


class ChimeSupersededError(Exception):
    """Raised when an existing chime for this mail's Subject already
    reflects an equal-or-newer mail.

    Mails are keyed by Subject, not filename: Apple Mail's exporter
    appends a disambiguating " 2", " 3", ... to the *filename* alone when
    several exported mails share a subject (the real Subject: header is
    identical across them) — those are different versions of the same
    chime, and only the latest (by Date header) is kept.
    """


def load_ensure_entry_folder(base_dir: Path):
    """Load ensure_entry_folder from base_dir/init_new.py — the target
    repo's own scaffolding tool, not a copy bundled with this mechanism."""
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
    return module.ensure_entry_folder


def update_chime_index(base_dir: Path) -> None:
    """Refresh base_dir/chime/index.md by running the target repo's own
    update_index.py — mirrors running it by hand from within that repo."""
    update_index_path = base_dir / "update_index.py"
    if not update_index_path.is_file():
        print(
            f"WARNING: {update_index_path} not found — chime/index.md not updated.",
            file=sys.stderr,
        )
        return
    subprocess.run(
        [sys.executable, str(update_index_path.resolve()), "chime"],
        cwd=base_dir,
        check=True,
    )


def subject_of(msg) -> str:
    return str(msg.get("subject") or "").strip() or "(no subject)"


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
    """True if candidate_date should replace current_date as the chime's
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


def read_chime_date(chime_path: Path):
    """Read back the date eml_file_to_chime wrote into an existing chime:
    line 1 is "# heading", line 2 blank, line 3 the date line — the exact
    layout init_new.py + eml_file_to_chime always produce together. Returns
    None if that line is missing or isn't a parseable ISO date (e.g. the
    "(no date)" fallback)."""
    lines = chime_path.read_text(encoding="utf-8").split("\n", 3)
    if len(lines) < 3:
        return None
    try:
        return datetime.fromisoformat(lines[2])
    except ValueError:
        return None


def eml_file_to_chime(eml_path: Path, base_dir: Path) -> Path:
    """Turn eml_path into a chime folder under base_dir/chime/<hash>/,
    keyed by the mail's Subject so later revisions of the same todo replace
    earlier ones rather than piling up as separate chimes."""
    ensure_entry_folder = load_ensure_entry_folder(base_dir)
    msg = email.message_from_bytes(eml_path.read_bytes(), policy=policy.default)
    subject = subject_of(msg)
    candidate_date = parsed_date_of(msg)

    chime_path, created = ensure_entry_folder("chime", subject, base_dir=base_dir)
    chime_dir = chime_path.parent

    if not created:
        if not is_newer(candidate_date, read_chime_date(chime_path)):
            raise ChimeSupersededError(
                f"chime for subject {subject!r} already reflects an "
                f"equal-or-newer mail — {eml_path.name!r} is superseded"
            )
        shutil.rmtree(chime_dir)
        chime_path, created = ensure_entry_folder("chime", subject, base_dir=base_dir)
        assert created, f"just removed {chime_dir}, recreating it should not clash"

    scratch_dir = chime_dir / "_html_scratch"
    md_path = chime_dir / (sanitize(eml_path.stem) + ".md")

    try:
        eml_to_html_extract(eml_path, scratch_dir, inject_h1=False)
        html_to_markdown_convert(scratch_dir, chime_dir)

        markdown_body = md_path.read_text(encoding="utf-8")
        md_path.unlink()

        plain_text = plain_text_of(msg)
        if plain_text is not None:
            (chime_dir / PLAIN_TEXT_NAME).write_text(plain_text, encoding="utf-8")

        date_line = date_line_of(msg)
        with chime_path.open("a", encoding="utf-8") as f:
            f.write(f"{date_line}\n\n")
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
            if plain_text is not None:
                f.write(f"\n[Plain text]({PLAIN_TEXT_NAME})\n")
    finally:
        if scratch_dir.is_dir():
            shutil.rmtree(scratch_dir)

    return chime_path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("eml", type=Path, help="path to the .eml file")
    parser.add_argument(
        "-o",
        "--out-dir",
        type=Path,
        default=Path("."),
        help="base directory to create the chime/ tree under (default: cwd)",
    )
    args = parser.parse_args()

    if not args.eml.is_file():
        sys.exit(f"Not a file: {args.eml}")

    try:
        chime_path = eml_file_to_chime(args.eml, base_dir=args.out_dir)
    except (ChimeSupersededError, FileNotFoundError) as e:
        sys.exit(str(e))

    update_chime_index(args.out_dir)
    print(f"chime -> {chime_path}")


if __name__ == "__main__":
    main()
