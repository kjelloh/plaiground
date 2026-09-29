#!/usr/bin/env python3
"""Transform a single .eml file into a chime folder (chime.md + local images).

Pipeline: eml_to_html (extract the HTML body + inline images) -> html_to_markdown
(convert to markdown, copy images) -> merge the result into chime.md scaffolded
by init_new.
"""

import argparse
import email
import shutil
import sys
from email import policy
from pathlib import Path

from eml_to_html import extract as eml_to_html_extract
from eml_to_html import sanitize
from html_to_markdown import convert as html_to_markdown_convert
from init_new import ensure_entry_folder

SESSION_DIR = Path(__file__).resolve().parent


class ChimeAlreadyExistsError(Exception):
    """Raised when a chime already exists for this eml file's name.

    The eml file name is used as the chime key, so this only fires when
    the same eml file has already been processed into a chime.
    """


def parse_date_line(eml_path: Path) -> str:
    msg = email.message_from_bytes(eml_path.read_bytes(), policy=policy.default)
    date = msg.get("date")
    if date is None:
        return "(no date)"
    try:
        return date.datetime.isoformat()
    except AttributeError:
        return str(date)


def eml_file_to_chime(eml_path: Path, base_dir: Path = SESSION_DIR) -> Path:
    chime_path, created = ensure_entry_folder("chime", eml_path.stem, base_dir=base_dir)
    if not created:
        raise ChimeAlreadyExistsError(
            f"chime already exists for eml file {eml_path.name!r} — "
            "already processed"
        )

    chime_dir = chime_path.parent
    scratch_dir = chime_dir / "_html_scratch"
    md_path = chime_dir / (sanitize(eml_path.stem) + ".md")

    try:
        eml_to_html_extract(eml_path, scratch_dir, inject_h1=False)
        html_to_markdown_convert(scratch_dir, chime_dir)

        markdown_body = md_path.read_text(encoding="utf-8")
        md_path.unlink()

        date_line = parse_date_line(eml_path)
        with chime_path.open("a", encoding="utf-8") as f:
            f.write(f"{date_line}\n\n")
            f.write(markdown_body)
    finally:
        if scratch_dir.is_dir():
            shutil.rmtree(scratch_dir)

    return chime_path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("eml", type=Path, help="path to the .eml file")
    args = parser.parse_args()

    if not args.eml.is_file():
        sys.exit(f"Not a file: {args.eml}")

    try:
        chime_path = eml_file_to_chime(args.eml)
    except ChimeAlreadyExistsError as e:
        sys.exit(str(e))

    print(f"chime -> {chime_path}")


if __name__ == "__main__":
    main()
