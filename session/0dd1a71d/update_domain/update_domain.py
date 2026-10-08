#!/usr/bin/env python3
"""Update the entries of a single domain in place.

    update_domain.py intralink <base_dir> <domain> [--dry-run]

An entry of a domain is the folder <base_dir>/<domain>/<hash>/ holding
<domain>.md, whose first line is its "# <title>" heading.

Modes (the required first argument):

- intralink: turn references to other entries of the same domain into
  markdown links. A reference is a line of the form

      ==> Also see "Todo: Some other entry?"
      ==> Se också ”Todo: Någon annan post?”

  i.e. an arrow "==>" (also HTML-escaped as "==&gt;", and the typos
  "== >" and "=>="), optionally after blockquote / emphasis markers, then a
  cue ("Also see", "See also", "Se också", "Se även"; any case, optional
  ":"), then a quoted title. Only the title inside the quotes is changed —
  into a link to the entry it names; the text stays as written:

      ==> Also see "[Todo: Some other entry?](../1a2b3c4d/chime.md)"

  The quoted title is matched against the domain's entry titles, first
  match wins:

  - exact:      the very same text
  - normalized: same after unescaping HTML (&lt; &gt; &amp; ...), unifying
                quote characters, collapsing whitespace, ignoring case and
                surrounding quotes / "?" / "." and <...> around URLs, and
                reading a markdown link "[text](url)" as its text
  - loose:      same after also ignoring a leading "Todo:" (or "odo:")
  - fuzzy:      at least FUZZY_MIN similar, at least FUZZY_MARGIN more so
                than the runner-up; only titles with the very same numbers
                count (so "gcc14" never links to "gcc13", nor "2016" to
                "2017")

  A title already turned into a link is left as is (so re-running is
  safe); its link is only checked. Nothing else in an entry is touched.

The report on stdout lists what deserves a look, then a summary line:

- FUZZY: linked by a fuzzy match (check that it is the right entry)
- UNRESOLVED: no entry matches (left as is)
- AMBIGUOUS: several entries match equally well (left as is)
- SELF: the reference names the entry it is in (left as is)
- BROKEN: an existing link whose target entry is missing

With --dry-run the same report is printed but no file is changed.
"""

import argparse
import difflib
import html
import re
import sys
import unicodedata
from dataclasses import dataclass, field
from pathlib import Path

CUES = ("also see", "see also", "se också", "se även")
QUOTES = "\"“”„«»'‘’"
ARROW = r"=+\s*(?:&gt;|>)=?"

REFERENCE_RE = re.compile(
    rf"^(?P<lead>[\s>*_]*{ARROW}\s*(?:{'|'.join(CUES)})\s*:?\s*)"
    rf"(?P<open>[{QUOTES}])(?P<rest>.*)$",
    re.IGNORECASE,
)
# A link this tool wrote: "]" in its text is escaped, whatever follows it.
LINKED_RE = re.compile(r"^\s*\[(?:\\.|[^\]\\])*\]\((?P<target>[^()\s]*)\)")
QUOTES_RE = re.compile(f"[{QUOTES}]")
AUTOLINK_RE = re.compile(r"<((?:https?|ftp)://[^>\s]+)>")
MD_LINK_RE = re.compile(r"\[([^\]]*)\]\([^()\s]*\)")
TODO_RE = re.compile(r"^t?odo\s*:\s*")

FUZZY_MIN = 0.92
FUZZY_MARGIN = 0.05


def normalized(title: str) -> str:
    text = unicodedata.normalize("NFC", html.unescape(title))
    text = MD_LINK_RE.sub(r"\1", AUTOLINK_RE.sub(r"\1", text))
    text = QUOTES_RE.sub('"', text)
    text = re.sub(r"\s+", " ", text)
    return text.casefold().strip(' "?.')


def loose(title: str) -> str:
    return TODO_RE.sub("", normalized(title))


def numbers(key: str) -> list[str]:
    return re.findall(r"\d+", key)


@dataclass
class TitleIndex:
    """Entry titles of a domain, looked up by ever looser keys."""

    exact: dict[str, set[str]] = field(default_factory=dict)
    normalized: dict[str, set[str]] = field(default_factory=dict)
    loose: dict[str, set[str]] = field(default_factory=dict)
    titles: dict[str, str] = field(default_factory=dict)  # hash -> title

    def add(self, key: str, title: str) -> None:
        self.titles[key] = title
        self.exact.setdefault(title, set()).add(key)
        self.normalized.setdefault(normalized(title), set()).add(key)
        self.loose.setdefault(loose(title), set()).add(key)

    def match(self, text: str) -> tuple[str, set[str], float]:
        """(how, matching entry hashes, similarity) for a referenced title;
        how is "unresolved" with no hashes if nothing matches."""
        for how, table, key in (
            ("exact", self.exact, text.strip()),
            ("normalized", self.normalized, normalized(text)),
            ("loose", self.loose, loose(text)),
        ):
            if key in table:
                return how, table[key], 1.0
        return self.fuzzy(loose(text))

    def fuzzy(self, key: str) -> tuple[str, set[str], float]:
        matcher = difflib.SequenceMatcher(autojunk=False)
        matcher.set_seq2(key)
        scored = []
        for candidate, hashes in self.loose.items():
            if numbers(candidate) != numbers(key):
                continue
            matcher.set_seq1(candidate)
            if matcher.real_quick_ratio() < FUZZY_MIN or matcher.quick_ratio() < FUZZY_MIN:
                continue
            scored.append((matcher.ratio(), candidate, hashes))
        scored.sort(key=lambda s: s[0], reverse=True)
        if not scored or scored[0][0] < FUZZY_MIN:
            return "unresolved", set(), 0.0
        ratio, _, hashes = scored[0]
        if len(scored) > 1 and ratio - scored[1][0] < FUZZY_MARGIN:
            return "ambiguous", hashes | scored[1][2], ratio
        return "fuzzy", hashes, ratio


@dataclass
class Reference:
    where: str  # "<domain>/<hash>/<domain>.md:<line>"
    text: str  # the quoted title as written
    how: str
    target: str | None = None
    ratio: float = 1.0


@dataclass
class Report:
    linked: list[Reference] = field(default_factory=list)
    already_linked: int = 0
    unresolved: list[Reference] = field(default_factory=list)
    ambiguous: list[Reference] = field(default_factory=list)
    self_refs: list[Reference] = field(default_factory=list)
    broken: list[Reference] = field(default_factory=list)
    files_changed: int = 0

    def count(self, how: str) -> int:
        return sum(1 for ref in self.linked if ref.how == how)

    @property
    def references(self) -> int:
        return (
            len(self.linked) + self.already_linked + len(self.unresolved)
            + len(self.ambiguous) + len(self.self_refs) + len(self.broken)
        )


def entries_of(base_dir: Path, domain: str) -> dict[str, Path]:
    """hash -> <domain>.md for every entry folder of a domain."""
    entries = {}
    for folder in sorted((base_dir / domain).iterdir()):
        md = folder / f"{domain}.md"
        if folder.is_dir() and md.is_file():
            entries[folder.name] = md
    return entries


def title_of(md: Path) -> str:
    with md.open(encoding="utf-8") as f:
        line = f.readline().strip()
    return line[2:].strip() if line.startswith("# ") else ""


def split_quoted(rest: str) -> list[tuple[str, str]]:
    """Ways to split what follows the opening quote into (title, closing
    quote + tail), most likely first: up to the last quote on the line, then
    up to each earlier one (titles may hold quotes themselves, and a note
    may follow the title), then the whole rest of the line (an unclosed
    quote)."""
    stripped = rest.rstrip()
    ways = [(rest[:i], rest[i:]) for i in range(len(rest) - 1, 0, -1) if rest[i] in QUOTES]
    ways.append((stripped, rest[len(stripped):]))
    return ways


def link_text(title: str) -> str:
    return title.replace("\\", "\\\\").replace("[", "\\[").replace("]", "\\]")


def intralink_line(
    line: str, where: str, own: str, md: Path, domain: str, index: TitleIndex, report: Report
) -> str:
    match = REFERENCE_RE.match(line)
    if match is None:
        return line
    rest = match["rest"]
    if not rest.strip(QUOTES + " \t"):
        return line

    linked = LINKED_RE.match(rest)
    if linked:
        if (md.parent / linked["target"]).resolve().is_file():
            report.already_linked += 1
        else:
            report.broken.append(Reference(where, linked[0].strip(), "broken", linked["target"]))
        return line

    first_miss = None
    for title, tail in split_quoted(rest):
        text = title.strip()
        how, hashes, ratio = index.match(text)
        ref = Reference(where, text, how, ratio=ratio)
        if how in ("unresolved", "ambiguous"):
            if how == "ambiguous":
                ref.target = ", ".join(sorted(hashes))
            first_miss = first_miss or ref
            continue
        if len(hashes) > 1:
            ref.how, ref.target = "ambiguous", ", ".join(sorted(hashes))
            first_miss = first_miss or ref
            continue
        (target,) = hashes
        ref.target = target
        if target == own:
            report.self_refs.append(ref)
            return line
        report.linked.append(ref)
        lead_ws = title[: len(title) - len(title.lstrip())]
        trail_ws = title[len(title.rstrip()):]
        link = f"[{link_text(text)}](../{target}/{domain}.md)"
        return f"{match['lead']}{match['open']}{lead_ws}{link}{trail_ws}{tail}"

    if first_miss.how == "ambiguous":
        report.ambiguous.append(first_miss)
    else:
        report.unresolved.append(first_miss)
    return line


def intralink(base_dir: Path, domain: str, dry_run: bool = False) -> tuple[Report, TitleIndex]:
    entries = entries_of(base_dir, domain)
    index = TitleIndex()
    for key, md in entries.items():
        index.add(key, title_of(md))

    report = Report()
    for key, md in entries.items():
        with md.open(encoding="utf-8", newline="") as f:
            text = f.read()
        lines = text.split("\n")
        for i, line in enumerate(lines):
            cr = "\r" if line.endswith("\r") else ""
            body = line[: len(line) - len(cr)]
            where = f"{domain}/{key}/{md.name}:{i + 1}"
            lines[i] = intralink_line(body, where, key, md, domain, index, report) + cr
        updated = "\n".join(lines)
        if updated != text:
            report.files_changed += 1
            if not dry_run:
                with md.open("w", encoding="utf-8", newline="") as f:
                    f.write(updated)
    return report, index


def report_lines(report: Report, index: TitleIndex, domain: str, dry_run: bool) -> list[str]:
    lines = []
    for ref in report.linked:
        if ref.how == "fuzzy":
            lines.append(
                f"FUZZY ({ref.ratio:.2f}): {ref.where}  \"{ref.text}\" -> "
                f"{ref.target}  {index.titles[ref.target]}"
            )
    for ref in report.unresolved:
        lines.append(f"UNRESOLVED: {ref.where}  \"{ref.text}\"")
    for ref in report.ambiguous:
        lines.append(f"AMBIGUOUS: {ref.where}  \"{ref.text}\" -> {ref.target}")
    for ref in report.self_refs:
        lines.append(f"SELF: {ref.where}  \"{ref.text}\"")
    for ref in report.broken:
        lines.append(f"BROKEN: {ref.where}  {ref.text}")
    if lines:
        lines.append("")
    prefix = "DRY RUN — nothing changed. " if dry_run else ""
    would = "would change" if dry_run else "changed"
    summary = (
        f"{prefix}{domain} (intralink): {report.references} references: "
        f"{len(report.linked)} linked ({report.count('exact')} exact, "
        f"{report.count('normalized')} normalized, {report.count('loose')} loose, "
        f"{report.count('fuzzy')} fuzzy), {report.already_linked} already linked, "
        f"{len(report.unresolved)} unresolved"
    )
    for label, items in (("ambiguous", report.ambiguous), ("self", report.self_refs), ("broken", report.broken)):
        if items:
            summary += f", {len(items)} {label}"
    lines.append(f"{summary}; {report.files_changed} entries {would}.")
    return lines


def domain_name(text: str) -> str:
    domain = text.strip().lower()
    if not domain.isidentifier():
        raise argparse.ArgumentTypeError(
            f"invalid domain {text!r} — use a simple name like 'mail' or 'note'"
        )
    return domain


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "mode",
        choices=("intralink",),
        help="intralink: link 'Also see' references between entries of the domain",
    )
    parser.add_argument("base_dir", type=Path, help="base directory of the target repo")
    parser.add_argument("domain", type=domain_name, help="the domain to update")
    parser.add_argument("--dry-run", action="store_true", help="report only; change nothing")
    args = parser.parse_args()

    domain_dir = args.base_dir / args.domain
    if not domain_dir.is_dir():
        sys.exit(f"Domain {args.domain!r} does not exist ({domain_dir}).")

    report, index = intralink(args.base_dir, args.domain, dry_run=args.dry_run)
    print("\n".join(report_lines(report, index, args.domain, args.dry_run)))


if __name__ == "__main__":
    main()
