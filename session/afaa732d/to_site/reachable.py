#!/usr/bin/env python3
"""
reachable.py: Walk a site's markdown link graph starting at a root index.md,
producing an adjacency list of every reachable file to the files it links to.
"""

import re
import sys
from pathlib import Path
from collections import deque

# Matches both [text](target) and ![alt](target), capturing the target.
LINK_RE = re.compile(r'!?\[[^\]]*\]\(([^)]+)\)')

SCHEME_RE = re.compile(r'^[a-zA-Z][a-zA-Z0-9+.-]*:')  # http:, mailto:, etc.


def is_external(target: str) -> bool:
    return bool(SCHEME_RE.match(target)) or target.startswith('//')


class ReachabilityGraph:
    """Builds an adjacency list of files reachable from a root markdown file,
    by following markdown links and image references (BFS)."""

    def __init__(self, site_root: Path):
        self.site_root = site_root.resolve()
        self.adjacency: dict[Path, list[Path]] = {}

    def build(self, root_md: Path) -> dict[Path, list[Path]]:
        """BFS from root_md. Returns {file: [linked files]} for every
        reachable file. Non-markdown files are leaves (empty link list)."""
        root_md = root_md.resolve()
        queue = deque([root_md])
        visited = {root_md}

        while queue:
            current = queue.popleft()
            targets = self._extract_links(current) if current.suffix.lower() == '.md' else []
            self.adjacency[current] = targets
            for target in targets:
                if target not in visited:
                    visited.add(target)
                    queue.append(target)

        return self.adjacency

    def _extract_links(self, md_file: Path) -> list[Path]:
        text = md_file.read_text(encoding='utf-8')
        targets: dict[Path, None] = {}  # insertion-ordered set
        for raw in LINK_RE.findall(text):
            target = self._parse_target(raw)
            target = target.split('#', 1)[0]  # drop in-page anchors
            if not target or is_external(target):
                continue
            resolved = self._resolve(target, md_file)
            # Only files are published: links to folders (and to missing
            # files) are left as they are, not followed.
            if resolved is not None and resolved.is_file():
                targets[resolved] = None
        return list(targets)

    def _parse_target(self, raw: str) -> str:
        """A link destination is either a bare token, or - per CommonMark,
        needed for a target containing a space (e.g. an attachment named
        "En timme.pages") - wrapped in <...>. Only a bare token may carry a
        trailing " title" to drop; a bracketed target's content, spaces
        included, is the target verbatim."""
        raw = raw.strip()
        if raw.startswith('<'):
            end = raw.find('>')
            return raw[1:end] if end != -1 else raw[1:]
        return raw.split()[0]

    def _resolve(self, target: str, from_file: Path) -> Path | None:
        base = self.site_root / target.lstrip('/') if target.startswith('/') else from_file.parent / target
        try:
            return base.resolve()
        except OSError:
            return None


def main():
    site_root = (Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()).resolve()
    root_md = site_root / "index.md"

    graph = ReachabilityGraph(site_root)
    adjacency = graph.build(root_md)

    for src, targets in adjacency.items():
        print(f"{src.relative_to(site_root)}:")
        for t in targets:
            print(f"  -> {t.relative_to(site_root)}")


if __name__ == "__main__":
    main()
