"""Parse an exclude.md listing mails that must not become chimes.

exclude.md lives in the eml folder itself, so it filters the mail source:
a listed mail never becomes a chime, and an already existing chime for it
is removed. Its format is the one update_index.py writes to
chime/index.md, so entries can be cut-and-pasted (or grep'ed) straight
from any chime index:

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
class Entry:
    line: str
    heading: str
    target_hash: str | None
    # Set once the entry has matched a mail or an existing chime, so
    # entries that matched nothing (typos, stale lines) can be reported.
    hit: bool = False


@dataclass
class Exclusions:
    source: Path
    compute_hash: Callable[[str], str]
    entries: list[Entry] = field(default_factory=list)

    def __len__(self) -> int:
        return len(self.entries)

    def entry_matches(self, entry: Entry, subject: str) -> bool:
        return subject == entry.heading or self.compute_hash(subject) == entry.target_hash

    def matches(self, subject: str) -> bool:
        """True if any entry matches subject (marking those entries hit)."""
        matched = False
        for entry in self.entries:
            if self.entry_matches(entry, subject):
                entry.hit = True
                matched = True
        return matched

    def chime_hashes(self, entry: Entry) -> set[str]:
        """The chime folder hashes entry may refer to: its target's hash
        and the hash of its heading (the folder a mail with exactly that
        Subject would get)."""
        hashes = {self.compute_hash(entry.heading)} if entry.heading else set()
        if entry.target_hash is not None:
            hashes.add(entry.target_hash)
        return hashes

    def unmatched(self) -> list[Entry]:
        return [entry for entry in self.entries if not entry.hit]


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
        exclusions.entries.append(
            Entry(
                line=line.strip(),
                heading=match["heading"].strip(),
                target_hash=hash_from_target(match["target"], hash_length),
            )
        )
    return exclusions


def find_exclusions(eml_dir: Path, compute_hash: Callable[[str], str]) -> Exclusions | None:
    """Exclusions from eml_dir/exclude.md, or None if there is no such file."""
    exclude_path = eml_dir / EXCLUDE_FILE_NAME
    if not exclude_path.is_file():
        return None
    return load_exclusions(exclude_path, compute_hash)
