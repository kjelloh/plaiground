#!/usr/bin/env python3

import sys
from pathlib import Path

from eml_to_markdown import eml_file_to_markdown, to_path


def to_dir_path(path_str: str) -> Path:
    dir_path = Path(path_str)
    if not dir_path.is_dir():
        raise NotADirectoryError(f"Not a directory: {dir_path}")

    return dir_path


def main() -> None:

    if len(sys.argv) != 2:
        sys.exit(f"Usage: {sys.argv[0]} <path-to-eml-folder>")

    try:
        eml_dir = to_dir_path(sys.argv[1])
    except Exception as e:
        sys.exit(f"Exception: {e}")

    eml_paths = sorted(eml_dir.glob("*.eml"))
    if not eml_paths:
        sys.exit(f"No .eml files found in {eml_dir}")

    ok_count = 0
    fail_count = 0

    for eml_path in eml_paths:
        try:
            eml_file_to_markdown(to_path(str(eml_path)))
        except Exception as e:
            fail_count += 1
            print(f"FAIL: {eml_path.name}", file=sys.stderr)
            print(f"    └── {type(e).__name__}: {e}", file=sys.stderr)
        else:
            ok_count += 1

    print(f"\n{ok_count} ok, {fail_count} failed, {len(eml_paths)} total")


if __name__ == "__main__":
    main()
