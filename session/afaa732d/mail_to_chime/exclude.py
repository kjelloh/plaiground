"""Parse an exclude.md listing mails that must not become chimes.

exclude.md lives in the eml folder itself, so it filters the mail source:
a listed mail never becomes a chime. Its format is the one update_index.py
writes to chime/index.md, so entries can be cut-and-pasted (or grep'ed)
straight from any chime index:

    * [TODO: Wrap up TestBench](ad2d0789/chime.md)

A mail is excluded if its Subject either hashes to the chime folder hash
in an entry's link target, or equals an entry's heading text. Matching
either keeps hand-edited entries working: an annotated heading still
matches by hash, a hand-written "* [Some subject]()" matches by heading.
Lines that aren't list-item links (headings, notes, blank lines) are
ignored.
"""

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable

EXCLUDE_FILE_NAME = "exclude.md"

# Greedy heading up to the last "](" so a "]" inside the subject survives.
ENTRY_RE = re.compile(r"^\s*[*+-]\s+\[(?P<heading>.*)\]\((?P<target>[^()]*)\)\s*$")


@dataclass
class Exclusions:
    source: Path
    compute_hash: Callable[[str], str]
    hashes: set[str] = field(default_factory=set)
    headings: set[str] = field(default_factory=set)

    def __len__(self) -> int:
        return len(self.hashes | {self.compute_hash(h) for h in self.headings})

    def matches(self, subject: str) -> bool:
        return subject in self.headings or self.compute_hash(subject) in self.hashes


def hash_from_target(target: str, hash_length: int) -> str | None:
    """The chime folder hash in a "<hash>/chime.md"-style target, if any."""
    for segment in reversed(Path(target.strip().strip("<>")).parts[:-1]):
        if re.fullmatch(rf"[0-9a-f]{{{hash_length}}}", segment):
            return segment
    return None


def load_exclusions(exclude_path: Path, compute_hash: Callable[[str], str]) -> Exclusions:
    hash_length = len(compute_hash(""))
    exclusions = Exclusions(source=exclude_path, compute_hash=compute_hash)
    for line in exclude_path.read_text(encoding="utf-8").splitlines():
        match = ENTRY_RE.match(line)
        if match is None:
            continue
        heading = match["heading"].strip()
        if heading:
            exclusions.headings.add(heading)
        key = hash_from_target(match["target"], hash_length)
        if key is not None:
            exclusions.hashes.add(key)
    return exclusions


def find_exclusions(eml_dir: Path, compute_hash: Callable[[str], str]) -> Exclusions | None:
    """Exclusions from eml_dir/exclude.md, or None if there is no such file."""
    exclude_path = eml_dir / EXCLUDE_FILE_NAME
    if not exclude_path.is_file():
        return None
    return load_exclusions(exclude_path, compute_hash)
