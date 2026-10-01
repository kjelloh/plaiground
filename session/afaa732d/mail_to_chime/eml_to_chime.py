#!/usr/bin/env python3
"""Transform a single .eml file into a chime folder (chime.md + local images).

Pipeline: eml_to_html (extract the HTML body + inline images) -> html_to_markdown
(convert to markdown, copy images) -> merge the result into chime.md scaffolded
by init_new.

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
from email import policy
from pathlib import Path

from eml_to_html import extract as eml_to_html_extract
from eml_to_html import sanitize
from html_to_markdown import convert as html_to_markdown_convert


class ChimeAlreadyExistsError(Exception):
    """Raised when a chime already exists for this eml file's name.

    The eml file name is used as the chime key, so this only fires when
    the same eml file has already been processed into a chime.
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


def parse_date_line(eml_path: Path) -> str:
    msg = email.message_from_bytes(eml_path.read_bytes(), policy=policy.default)
    date = msg.get("date")
    if date is None:
        return "(no date)"
    try:
        return date.datetime.isoformat()
    except AttributeError:
        return str(date)


def eml_file_to_chime(eml_path: Path, base_dir: Path) -> Path:
    """Turn eml_path into a chime folder under base_dir/chime/<hash>/."""
    ensure_entry_folder = load_ensure_entry_folder(base_dir)
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
    except (ChimeAlreadyExistsError, FileNotFoundError) as e:
        sys.exit(str(e))

    update_chime_index(args.out_dir)
    print(f"chime -> {chime_path}")


if __name__ == "__main__":
    main()
