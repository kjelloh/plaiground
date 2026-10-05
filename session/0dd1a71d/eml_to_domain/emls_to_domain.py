#!/usr/bin/env python3
"""Batch-run eml_to_domain over every .eml file in a folder.

ALL .eml files are processed. Runs are incremental: a mail whose Subject
already has an entry in the domain replaces it only if the mail is newer
(by Date header), otherwise it is skipped as superseded. Failures are
logged and skipped so one bad eml doesn't stop the run.
"""

import argparse
import sys
from pathlib import Path

from eml_to_domain import (
    EntrySupersededError,
    domain_name,
    eml_file_to_entry,
    entry_md_name,
    load_init_new,
    read_subject,
    update_domain_index,
)

# When stdout and stderr are both redirected to the same file (e.g.
# `> output.log 2>&1`), stdout is block-buffered while stderr isn't, so
# FAIL lines (stderr) can land mid-way through an unflushed stdout line.
# Force line buffering so writes interleave in the order they're printed.
sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)


def to_dir_path(path_str: str) -> Path:
    dir_path = Path(path_str)
    if not dir_path.is_dir():
        raise NotADirectoryError(f"Not a directory: {dir_path}")

    return dir_path


def emls_to_domain(eml_paths: list[Path], base_dir: Path, domain: str) -> dict[str, int]:
    """Import eml_paths into base_dir/<domain>/, printing one line per mail.
    Returns the counts: added (new entry), updated (newer mail replaced an
    entry), skipped (superseded) and failed."""
    compute_hash = load_init_new(base_dir).compute_hash
    counts = {"added": 0, "updated": 0, "skipped": 0, "failed": 0}

    for eml_path in eml_paths:
        try:
            subject = read_subject(eml_path)
            existing = base_dir / domain / compute_hash(subject) / entry_md_name(domain)
            existed = existing.is_file()
            eml_file_to_entry(eml_path, base_dir=base_dir, domain=domain)
        except EntrySupersededError:
            counts["skipped"] += 1
            print(f"SKIP: {eml_path.name} (superseded by an equal-or-newer mail with the same subject)")
        except Exception as e:
            counts["failed"] += 1
            print(f"FAIL: {eml_path.name}", file=sys.stderr)
            print(f"    └── {type(e).__name__}: {e}", file=sys.stderr)
        else:
            if existed:
                counts["updated"] += 1
                print(f"UPDATE: {eml_path.name}")
            else:
                counts["added"] += 1
                print(f"ADD: {eml_path.name}")

    return counts


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("eml_dir", help="path to the folder of .eml files")
    parser.add_argument("out_dir", type=Path, help="base directory of the target repo")
    parser.add_argument(
        "--domain",
        type=domain_name,
        required=True,
        help="domain (namespace folder) to import into, e.g. 'mail'",
    )
    args = parser.parse_args()

    try:
        eml_dir = to_dir_path(args.eml_dir)
    except Exception as e:
        sys.exit(f"Exception: {e}")

    eml_paths = sorted(eml_dir.glob("*.eml"))
    if not eml_paths:
        sys.exit(f"No .eml files found in {eml_dir}")

    try:
        counts = emls_to_domain(eml_paths, args.out_dir, args.domain)
    except FileNotFoundError as e:  # no init_new.py in the target repo
        sys.exit(str(e))

    print(
        f"\n{len(eml_paths)} mail files -> {args.domain}: {counts['added']} added, "
        f"{counts['updated']} updated, {counts['skipped']} skipped (superseded), "
        f"{counts['failed']} failed"
    )

    if counts["added"] or counts["updated"]:
        update_domain_index(args.out_dir, args.domain)


if __name__ == "__main__":
    main()
