#!/usr/bin/env python3
"""Batch-run eml_to_chime over every .eml file in a folder.

Failures are logged and skipped so one bad eml doesn't stop the run.
If the folder has an exclude.md (same format as chime/index.md), mails
listed in it are skipped and any existing chime it lists is removed first;
--dry-run only reports what would be excluded and removed.
"""

import argparse
import sys
from pathlib import Path

from exclude import Exclusions
from eml_to_chime import (
    ChimeExcludedError,
    ChimeSupersededError,
    eml_file_to_chime,
    find_exclusions_for,
    read_subject,
    remove_excluded_chimes,
    update_chime_index,
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


def exclude_report(
    exclusions: Exclusions,
    excluded_files: dict[str, list[str]],
    removed: list[tuple[Path, str]],
    dry_run: bool = False,
) -> list[str]:
    """The exclude.md part of the final report. Mail files and chimes are
    counted separately: several exported versions of one Subject (Apple
    Mail's "x.eml", "x 2.eml", ...) are several excluded mail files but
    only one chime. excluded_files maps chime hash -> excluded mail files."""
    would = "would be " if dry_run else ""
    removed_keys = {chime_dir.name for chime_dir, _ in removed}
    file_count = sum(len(names) for names in excluded_files.values())
    no_chime = [key for key in excluded_files if key not in removed_keys]
    no_mail = [key for key in removed_keys if key not in excluded_files]
    unmatched = exclusions.unmatched()

    lines = [
        f"exclude.md ({exclusions.source}), {len(exclusions)} entries:",
        f"  {file_count} mail files {would}excluded, covering {len(excluded_files)} subjects",
        f"  {len(removed)} existing chimes {would}removed",
    ]
    if no_chime:
        lines.append(f"  {len(no_chime)} excluded subjects had no existing chime")
    if no_mail:
        lines.append(f"  {len(no_mail)} chimes {would}removed whose mail is not in this folder")
    lines.append(f"  {len(unmatched)} entries matched nothing")

    if removed:
        verb = "Would remove" if dry_run else "Removed"
        lines += ["", f"{verb} {len(removed)} chime(s):"]
        for chime_dir, heading in removed:
            count = len(excluded_files.get(chime_dir.name, []))
            note = f"{count} mail files excluded" if count else "mail not in this folder"
            lines.append(f"  {chime_dir}  {heading}   ({note})")
    if unmatched:
        lines += ["", f"UNMATCHED: {len(unmatched)} entry(ies) matched no mail and no existing chime:"]
        lines += [f"  {entry.line}" for entry in unmatched]
    return lines


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("eml_dir", help="path to the folder of .eml files")
    parser.add_argument("out_dir", type=Path, help="base directory of the target repo")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="only report what exclude.md would exclude and remove; create and remove nothing",
    )
    args = parser.parse_args()

    try:
        eml_dir = to_dir_path(args.eml_dir)
    except Exception as e:
        sys.exit(f"Exception: {e}")

    out_dir = args.out_dir
    dry_run = args.dry_run

    eml_paths = sorted(eml_dir.glob("*.eml"))
    if not eml_paths:
        sys.exit(f"No .eml files found in {eml_dir}")

    try:
        exclusions = find_exclusions_for(eml_dir, out_dir)
    except Exception as e:
        sys.exit(f"Exception: {e}")

    removed: list[tuple[Path, str]] = []
    if exclusions is not None:
        print(f"Applying {exclusions.source} ({len(exclusions)} entries)")
        # Before any mail is processed: an excluded chime goes whether or
        # not its mail is still in eml_dir.
        removed = remove_excluded_chimes(out_dir, exclusions, dry_run=dry_run)
        for chime_dir, heading in removed:
            print(f"{'WOULD REMOVE' if dry_run else 'REMOVE'}: {chime_dir}  {heading}")

    ok_count = 0
    skipped_count = 0
    excluded_count = 0
    fail_count = 0
    excluded_files: dict[str, list[str]] = {}  # chime hash -> mail files

    for eml_path in eml_paths:
        try:
            if dry_run:
                # Only report exclusions; converting would write chimes.
                subject = read_subject(eml_path)
                if exclusions is not None and exclusions.matches(subject):
                    raise ChimeExcludedError("excluded", subject=subject)
                continue
            eml_file_to_chime(eml_path, base_dir=out_dir, exclusions=exclusions)
            print(f"OK: {eml_path.name}")
        except ChimeExcludedError as e:
            excluded_count += 1
            key = exclusions.compute_hash(e.subject)
            excluded_files.setdefault(key, []).append(eml_path.name)
            print(f"EXCLUDE: {eml_path.name}")
        except ChimeSupersededError:
            skipped_count += 1
            print(f"SKIP: {eml_path.name} (superseded by a newer mail with the same subject)")
        except Exception as e:
            fail_count += 1
            print(f"FAIL: {eml_path.name}", file=sys.stderr)
            print(f"    └── {type(e).__name__}: {e}", file=sys.stderr)
        else:
            ok_count += 1

    if dry_run:
        print(
            f"\nDRY RUN — nothing created or removed. {len(eml_paths)} mail files: "
            f"{excluded_count} would be excluded, {fail_count} failed"
        )
    else:
        print(
            f"\n{len(eml_paths)} mail files: {ok_count} ok, "
            f"{skipped_count} skipped (superseded), {excluded_count} excluded, "
            f"{fail_count} failed"
        )

    if exclusions is not None:
        print()
        print("\n".join(exclude_report(exclusions, excluded_files, removed, dry_run)))

    if not dry_run and (ok_count or removed):
        update_chime_index(out_dir)


if __name__ == "__main__":
    main()
