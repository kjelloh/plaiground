#!/usr/bin/env python3
"""
init_new: Create a typed folder with a hashed subfolder,
and inside it create a markdown file named after a sanitized heading.

The new markdown starts with the heading followed by a hash tag line:

    # <heading>

    #<full md5 of heading>

The folder is named after the first HASH_LENGTH characters of that same
hash, so the tag is a stable, collision-checked reference to the document.

Rules:
- Spaces -> '_'
- Invalid filename characters -> '?'
"""

import hashlib
import re
import sys
from pathlib import Path

HASH_LENGTH = 8

# A whole line holding just "#<32 hex digits>" (md5).
HASH_TAG_RE = re.compile(r"^#(?P<full_hash>[0-9a-f]{32})$")


class HashCollisionError(ValueError):
    """Raised when an existing entry in the target folder carries a hash
    tag for a different heading (same short hash, different full hash)."""


def compute_full_hash(text: str) -> str:
    return hashlib.md5(text.encode("utf-8")).hexdigest()


def compute_hash(text: str) -> str:
    return compute_full_hash(text)[:HASH_LENGTH]


def hash_tag(text: str) -> str:
    return f"#{compute_full_hash(text)}"


def read_hash_tag(md_file: Path) -> str | None:
    """The full hash from md_file's hash tag line, or None if it has none."""
    for line in md_file.read_text(encoding="utf-8").splitlines():
        match = HASH_TAG_RE.match(line.strip())
        if match is not None:
            return match["full_hash"]
    return None


def ensure_entry_folder(
    entry_type: str, heading: str, base_dir: Path = Path(".")
) -> tuple[Path, bool]:
    """Ensure <base_dir>/<entry_type>/<hash(heading)>/<entry_type>.md exists.

    Returns (file_path, created) where created is False if the file was
    already there (left untouched) and True if this call wrote it.
    Raises HashCollisionError if the existing file's hash tag belongs to a
    different heading.
    """
    if not entry_type.isidentifier():
        raise ValueError(f"Invalid type '{entry_type}'. Use a simple name like 'todo' or 'note'.")

    short_hash = compute_hash(heading)
    folder_path = base_dir / entry_type / short_hash
    folder_path.mkdir(parents=True, exist_ok=True)

    file_path = folder_path / f"{entry_type}.md"
    created = not file_path.exists()
    if created:
        file_path.write_text(f"# {heading}\n\n{hash_tag(heading)}\n\n", encoding="utf-8")
    else:
        existing_hash = read_hash_tag(file_path)
        if existing_hash is not None and existing_hash != compute_full_hash(heading):
            raise HashCollisionError(
                f"'{file_path}' is tagged #{existing_hash}, not "
                f"{hash_tag(heading)} — short hash collision for '{heading}'"
            )

    return file_path, created


def main():
    if len(sys.argv) < 3:
        print("Usage: ./init_new.py <type> 'Your heading text here'")
        sys.exit(1)

    entry_type = sys.argv[1].strip().lower()
    heading = " ".join(sys.argv[2:]).strip()

    try:
        file_path, created = ensure_entry_folder(entry_type, heading)
    except ValueError as e:
        print(e)
        sys.exit(1)

    if created:
        print(f"Created '{file_path}'")
    else:
        print(f"File '{file_path}' already exists")

    print(f"Directory structure ensured: '{file_path.parent}'")


if __name__ == "__main__":
    main()
