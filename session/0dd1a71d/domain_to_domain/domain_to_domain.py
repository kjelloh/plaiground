#!/usr/bin/env python3
"""Add picked entries of a source domain to a target domain.

    domain_to_domain.py init <base_dir> <source> <target> [--pick <pick.md>] [--dry-run]
    domain_to_domain.py add  <base_dir> <source> <target>  --pick <pick.md>  [--dry-run]

Both domains are required; there are no defaults. An entry of a domain is
the folder <base_dir>/<domain>/<hash>/ holding <domain>.md, as created by
the target repo's own init_new.py.

The pick list uses the line format update_index.py writes to
<domain>/index.md, so lines can be cut-and-pasted straight from the source
index:

    * [Some heading](1afd4852/mail.md)

An entry is picked by the folder hash in its link target, or — for a
hand-written "* [Some heading]()" — by the hash of its heading. Lines that
aren't list-item links (headings, notes, blank lines) are ignored.

Modes (the required first argument):

- init: create the target domain. Fails if <base_dir>/<target>/ already
  exists — use add for that. Without --pick it creates an empty domain
  (just the folder and its index.md).
- add:  add to an existing target domain. Fails if <base_dir>/<target>/
  doesn't exist — use init for that. --pick is required.

Either way the target is only ever added to; nothing in it is replaced or
removed, so edits made in the target are safe:

- ADD: picked, not yet in the target -> copied
- SKIP: picked, already in the target -> left as is ("source differs"
  if the source entry no longer matches the target copy)
- FAIL: picked source entry that can't be copied (see below)
- UNMATCHED: pick list entry that refers to no source entry

The exit status is 1 if any pick FAILed or was UNMATCHED.

A copy is the whole entry folder (images, attachments, ...) with
<source>.md / <source>.txt renamed to <target>.md / <target>.txt and links
to <source>.txt rewritten. The hash is computed from the heading only, so
an entry keeps its folder hash across domains; a source entry whose heading
doesn't hash to its folder name (e.g. a hand-edited heading) is reported as
FAIL and not copied.
"""

import argparse
import filecmp
import importlib.util
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

# Keep the report and update_index.py's output (a subprocess) in print
# order when stdout is piped or redirected.
sys.stdout.reconfigure(line_buffering=True)

# Greedy heading up to the last "](" so a "]" inside the heading survives.
ENTRY_RE = re.compile(r"^\s*[*+-]\s+\[(?P<heading>.*)\]\((?P<target>[^()]*)\)\s*$")


@dataclass
class Pick:
    line: str
    heading: str
    target_hash: str | None
    hit: bool = False


@dataclass
class Report:
    added: list[tuple[str, str]] = field(default_factory=list)
    skipped: list[tuple[str, str, bool]] = field(default_factory=list)  # (key, heading, source differs)
    failed: list[tuple[str, str]] = field(default_factory=list)
    unmatched: list[Pick] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return not (self.failed or self.unmatched)


def load_init_new(base_dir: Path):
    """Load base_dir/init_new.py — the target repo's own scaffolding tool,
    whose hash function names the entry folders."""
    init_new_path = base_dir / "init_new.py"
    if not init_new_path.is_file():
        raise FileNotFoundError(
            f"{init_new_path} not found — the target repo must have its own init_new.py."
        )
    spec = importlib.util.spec_from_file_location("_target_init_new", init_new_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def update_domain_index(base_dir: Path, domain: str) -> None:
    """Refresh base_dir/<domain>/index.md with the target repo's own
    update_index.py."""
    update_index_path = base_dir / "update_index.py"
    if not update_index_path.is_file():
        print(
            f"WARNING: {update_index_path} not found — {domain}/index.md not updated.",
            file=sys.stderr,
        )
        return
    subprocess.run(
        [sys.executable, str(update_index_path.resolve()), domain],
        cwd=base_dir,
        check=True,
    )


def hash_from_target(target: str, hash_length: int) -> str | None:
    """The entry folder hash in a "<hash>/<domain>.md"-style link target.
    Only mined for a hex segment, never followed as a path."""
    for segment in reversed(Path(target.strip().strip("<>")).parts[:-1]):
        if re.fullmatch(rf"[0-9a-f]{{{hash_length}}}", segment):
            return segment
    return None


def load_picks(pick_path: Path, hash_length: int) -> list[Pick]:
    picks = []
    for line in pick_path.read_text(encoding="utf-8").splitlines():
        match = ENTRY_RE.match(line)
        if match is None:
            continue
        picks.append(
            Pick(
                line=line.strip(),
                heading=match["heading"].strip(),
                target_hash=hash_from_target(match["target"], hash_length),
            )
        )
    return picks


def entries_of(base_dir: Path, domain: str, hash_length: int) -> dict[str, Path]:
    """hash -> <domain>.md for every entry folder of a domain."""
    root = base_dir / domain
    if not root.is_dir():
        return {}
    entries = {}
    for folder in sorted(root.iterdir()):
        md = folder / f"{domain}.md"
        if folder.is_dir() and re.fullmatch(rf"[0-9a-f]{{{hash_length}}}", folder.name) and md.is_file():
            entries[folder.name] = md
    return entries


def heading_of(md: Path) -> str:
    with md.open(encoding="utf-8") as f:
        return f.readline().strip().lstrip("#").strip()


def picked_hashes(picks: list[Pick], source_entries: dict[str, Path], compute_hash) -> set[str]:
    """Source entry hashes the picks refer to (marking picks that hit)."""
    picked = set()
    for pick in picks:
        candidates = {pick.target_hash} if pick.target_hash else set()
        if pick.heading:
            candidates.add(compute_hash(pick.heading))
        for key in candidates & source_entries.keys():
            pick.hit = True
            picked.add(key)
    return picked


def check_folder_hash(md: Path, compute_hash) -> None:
    """Raise ValueError unless md's heading hashes to its folder name."""
    expected = compute_hash(heading_of(md))
    if expected != md.parent.name:
        raise ValueError(f"its heading hashes to {expected}, not to its folder name")


def build_copy(source_dir: Path, dest_dir: Path, source: str, target: str) -> None:
    """Copy a source entry folder to dest_dir as a target entry."""
    shutil.copytree(source_dir, dest_dir)
    for ext in ("md", "txt"):
        src_file = dest_dir / f"{source}.{ext}"
        if src_file.is_file():
            src_file.rename(dest_dir / f"{target}.{ext}")
    md = dest_dir / f"{target}.md"
    text = md.read_text(encoding="utf-8")
    md.write_text(text.replace(f"]({source}.txt)", f"]({target}.txt)"), encoding="utf-8")


def same_tree(a: Path, b: Path) -> bool:
    """True if folders a and b hold the same files with the same content."""
    cmp = filecmp.dircmp(a, b)
    if cmp.left_only or cmp.right_only or cmp.funny_files:
        return False
    _, mismatch, errors = filecmp.cmpfiles(a, b, cmp.common_files, shallow=False)
    if mismatch or errors:
        return False
    return all(same_tree(a / sub, b / sub) for sub in cmp.common_dirs)


def add(
    base_dir: Path, source: str, target: str, picks: list[Pick], dry_run: bool = False
) -> Report:
    init_new = load_init_new(base_dir)
    hash_length = len(init_new.compute_hash(""))
    source_entries = entries_of(base_dir, source, hash_length)
    target_entries = entries_of(base_dir, target, hash_length)
    wanted = picked_hashes(picks, source_entries, init_new.compute_hash)
    report = Report(unmatched=[pick for pick in picks if not pick.hit])
    target_root = base_dir / target

    for key in sorted(wanted):
        source_md = source_entries[key]
        heading = heading_of(source_md)
        try:
            check_folder_hash(source_md, init_new.compute_hash)
        except ValueError as e:
            report.failed.append((key, f"{heading} ({e})"))
            continue

        dest_dir = target_root / key
        staging = target_root / f".{key}.staging"
        if staging.exists():
            shutil.rmtree(staging)
        target_root.mkdir(parents=True, exist_ok=True)
        build_copy(source_md.parent, staging, source, target)

        if key in target_entries:
            report.skipped.append((key, heading, not same_tree(staging, dest_dir)))
            shutil.rmtree(staging)
            continue

        report.added.append((key, heading))
        if dry_run:
            shutil.rmtree(staging)
            continue
        staging.rename(dest_dir)

    if target_root.is_dir() and not any(target_root.iterdir()):
        target_root.rmdir()  # don't leave an empty folder behind a dry run or failed init

    return report


def report_lines(
    report: Report, source: str, target: str, mode: str, picks: int, dry_run: bool
) -> list[str]:
    would = "WOULD " if dry_run else ""
    lines = []
    for key, heading in report.added:
        lines.append(f"{would}ADD: {target}/{key}  {heading}")
    for key, heading, differs in report.skipped:
        note = "already in target; source differs" if differs else "already in target"
        lines.append(f"SKIP: {target}/{key}  {heading} ({note})")
    for key, detail in report.failed:
        lines.append(f"FAIL: {source}/{key}  {detail}")
    for pick in report.unmatched:
        lines.append(f"UNMATCHED: {pick.line}")
    lines.append("")
    prefix = "DRY RUN — nothing changed. " if dry_run else ""
    lines.append(
        f"{prefix}{source} -> {target} ({mode}), {picks} pick entries: "
        f"{len(report.added)} added, {len(report.skipped)} skipped, "
        f"{len(report.failed)} failed, {len(report.unmatched)} unmatched"
    )
    return lines


def check_target_for_mode(base_dir: Path, target: str, mode: str) -> None:
    """Exit unless the target domain's existence fits the mode."""
    target_root = base_dir / target
    if mode == "init" and target_root.exists():
        sys.exit(
            f"Target domain {target!r} already exists ({target_root}) — "
            f"use 'add' to add entries to it."
        )
    if mode == "add" and not target_root.is_dir():
        sys.exit(
            f"Target domain {target!r} does not exist ({target_root}) — "
            f"use 'init' to create it."
        )


def create_empty_domain(base_dir: Path, target: str, dry_run: bool = False) -> None:
    """init without a pick list: just the target folder and its index.md."""
    target_root = base_dir / target
    if dry_run:
        print(f"DRY RUN — nothing changed. WOULD CREATE empty target domain {target!r} ({target_root})")
        return
    target_root.mkdir(parents=True)
    print(f"Created empty target domain {target!r} ({target_root})")
    update_domain_index(base_dir, target)


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
        choices=("init", "add"),
        help="init: create the target domain; add: add to an existing one",
    )
    parser.add_argument("base_dir", type=Path, help="base directory of the target repo")
    parser.add_argument("source", type=domain_name, help="domain to pick entries from")
    parser.add_argument("target", type=domain_name, help="domain to add entries to")
    parser.add_argument(
        "--pick",
        type=Path,
        help="pick list (index.md line format); required for add, "
        "optional for init (without it, init creates an empty domain)",
    )
    parser.add_argument(
        "--dry-run", action="store_true", help="only report what would change; change nothing"
    )
    args = parser.parse_args()

    if args.source == args.target:
        sys.exit(f"source and target domain are both {args.source!r}")
    if args.pick is None and args.mode == "add":
        parser.error("add requires --pick")
    if args.pick is not None and not args.pick.is_file():
        sys.exit(f"Not a file: {args.pick}")
    if not (args.base_dir / args.source).is_dir():
        sys.exit(f"Source domain folder {args.base_dir / args.source} does not exist")
    check_target_for_mode(args.base_dir, args.target, args.mode)

    if args.pick is None:
        create_empty_domain(args.base_dir, args.target, dry_run=args.dry_run)
        return

    try:
        hash_length = len(load_init_new(args.base_dir).compute_hash(""))
        picks = load_picks(args.pick, hash_length)
        report = add(args.base_dir, args.source, args.target, picks, dry_run=args.dry_run)
    except FileNotFoundError as e:
        sys.exit(str(e))

    print(
        "\n".join(
            report_lines(report, args.source, args.target, args.mode, len(picks), args.dry_run)
        )
    )

    if report.added and not args.dry_run:
        update_domain_index(args.base_dir, args.target)
    if not report.ok:
        sys.exit(1)

if __name__ == "__main__":
    main()
