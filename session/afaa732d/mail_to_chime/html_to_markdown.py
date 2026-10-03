#!/usr/bin/env python3
"""Transform an html/ folder (one .html document + image assets) into a
markdown document plus local copies of the images.

- Output markdown file has the same stem as the .html file, with a .md suffix.
- Every non-.html file in the source folder is copied into the output folder.
- Image and link references are rewritten to plain local filenames.

Standard library only, so it runs the same on Linux, macOS and Windows.
"""

import argparse
import re
import shutil
import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlsplit

MD_ESCAPE = {ord(c): "\\" + c for c in "\\`*"}
# Square brackets get HTML entities, not backslash escapes: "\[" / "\]" is
# valid CommonMark escaping, but it's also the LaTeX display-math delimiter
# many renderers (e.g. VS Code's Markdown preview) recognize, so backslash-
# escaping a bracket can turn plain text into a bogus, sometimes-broken math
# block. Entities can't be mistaken for either link syntax or math syntax.
MD_ESCAPE[ord("[")] = "&#91;"
MD_ESCAPE[ord("]")] = "&#93;"
# Angle brackets, too: raw "<...>" in markdown source is valid inline HTML
# per CommonMark, so a renderer's HTML parser (e.g. Jekyll's Kramdown) will
# try to parse it as a tag. A mail full of literal "<...>" references (log
# pastes, "<Settings> menu" style text) can make that parser recurse once
# per occurrence and blow its stack on a large-enough file — seen for real
# on a ~1000-line TestBench log mail with ~4300 "<...>" sequences. Escaping
# as entities (not backslash — backslash-escaping "<"/">" isn't even valid
# CommonMark) keeps it plain text no HTML parser will ever touch.
MD_ESCAPE[ord("<")] = "&lt;"
MD_ESCAPE[ord(">")] = "&gt;"


# A <br> becomes a CommonMark hard line break: two trailing spaces before
# the newline (the form every renderer supports — Kramdown, markdown-it,
# GitHub). Those spaces are indistinguishable from stray trailing
# whitespace, which result() strips, so a placeholder marks the break
# until after that cleanup.
HARD_BREAK = "\x00"


def md_link(url: str) -> str:
    url = url.strip()
    if re.search(r"[ ()<>]", url):
        return "<" + url.replace("<", "%3C").replace(">", "%3E") + ">"
    return url


def url_parts(url: str):
    """urlsplit(), tolerating malformed input (e.g. mail client "data
    detector" mistakes, like text starting with "//[" that urlsplit reads
    as a broken IPv6 host and rejects) by treating it as unparsable."""
    try:
        return urlsplit(url)
    except ValueError:
        return None


def is_local_ref(url: str) -> bool:
    parts = url_parts(url)
    if parts is None:
        return False
    return not parts.scheme and not parts.netloc and not url.startswith("#")


def is_autolinkable(url: str) -> bool:
    """True if url is well-formed enough to stand alone as a CommonMark
    autolink (<url>), which requires an absolute URI with a scheme."""
    parts = url_parts(url)
    return bool(parts and parts.scheme) and not re.search(r"[ <>]", url)


class MarkdownConverter(HTMLParser):
    SKIP_TAGS = {"head", "style", "script", "title"}
    VOID_TAGS = {"br", "img", "hr", "input", "meta", "link", "wbr", "col", "area", "source"}

    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.parts: list[str] = []
        self._nl_run = 2  # treat start-of-document as already blank
        self.line_has_content = False
        self.line_has_text = False  # beyond prefix / list marker
        self.skip_depth = 0
        self.pre_depth = 0
        self.quote_depth = 0
        self.list_stack: list[list] = []  # [tag, ordinal]
        # Open <b>/<i> markers as [marker, written]: a marker is written
        # only once visible text follows, so empty emphasis
        # (<i><br></i>) leaves no stray "*" lines and "<b> x</b>" becomes
        # " **x**" (CommonMark won't open emphasis before a space).
        self.mark_stack: list[list] = []
        # [tag, keeps_newlines] for elements with a CSS white-space style:
        # pre / pre-wrap / pre-line text keeps its newlines as line breaks.
        self.ws_stack: list[list] = []
        self.link_href = ""
        self.link_buf: list[str] | None = None
        self.refs: list[str] = []

    # -- low level output -------------------------------------------------
    def _prefix(self) -> str:
        return "> " * self.quote_depth + "  " * len(self.list_stack)

    def _put(self, ch: str) -> None:
        if ch == "\n":
            self.parts.append("\n")
            self._nl_run += 1
            self.line_has_content = False
            self.line_has_text = False
            return
        if not self.line_has_content:
            prefix = self._prefix()
            if prefix:
                self.parts.append(prefix)
            self.line_has_content = True
        self.parts.append(ch)
        self._nl_run = 0
        self.line_has_text = True

    def raw(self, text: str) -> None:
        for ch in text:
            self._put(ch)

    def emit(self, chunk: str) -> None:
        if self.link_buf is not None:
            self.link_buf.append(chunk)
        else:
            self.raw(chunk)

    def open_mark(self, marker: str) -> None:
        self.mark_stack.append([marker, False])

    def close_mark(self, marker: str) -> None:
        if not self.mark_stack or self.mark_stack[-1][0] != marker:
            return  # stray end tag
        _, written = self.mark_stack.pop()
        if written:
            self.emit(marker)

    def keeps_newlines(self) -> bool:
        return bool(self.ws_stack) and self.ws_stack[-1][1]

    def text(self, data: str) -> None:
        if self.skip_depth:
            return
        if self.pre_depth:
            self.raw(data)
            return
        if self.keeps_newlines():
            for i, segment in enumerate(data.split("\n")):
                if i:
                    self.text_newline()
                self.inline_text(segment)
            return
        self.inline_text(data)

    def text_newline(self) -> None:
        if self.link_buf is not None:
            self.link_buf.append(" ")
        elif self.line_has_text:
            self.line_break()
        else:
            self._put("\n")

    def inline_text(self, data: str) -> None:
        data = re.sub(r"\s+", " ", data.replace("\xa0", " "))
        if self.link_buf is None and not self.line_has_content:
            data = data.lstrip()
        if not data:
            return
        pending = [m for m in self.mark_stack if not m[1]]
        if pending and data.strip():
            # Leading space goes before the opening marker(s).
            stripped = data.lstrip()
            if len(stripped) < len(data):
                self.emit(" ")
            for m in pending:
                self.emit(m[0])
                m[1] = True
            data = stripped
        self.emit(data.translate(MD_ESCAPE))

    def line_break(self) -> None:
        """End the current line as a hard break: a single newline alone is
        a soft break, which renderers join into one line with the next."""
        if self.line_has_text:
            self.parts.append(HARD_BREAK)
            self._put("\n")

    def div_boundary(self) -> None:
        # Inside a quote or list a blank line would end the quote/list
        # item, so div lines are kept apart by a hard break instead.
        if self.quote_depth or self.list_stack:
            self.line_break()
        else:
            self.block(2)

    def block(self, n: int = 2) -> None:
        while self._nl_run < n:
            self.parts.append("\n")
            self._nl_run += 1
        self.line_has_content = False

    # -- tag handling --------------------------------------------------------
    def handle_starttag(self, tag, attrs):
        if tag in self.SKIP_TAGS:
            self.skip_depth += 1
            return
        if self.skip_depth:
            return
        a = dict(attrs)

        ws = re.search(r"white-space\s*:\s*([\w-]+)", a.get("style") or "")
        if ws and tag not in self.VOID_TAGS:
            self.ws_stack.append([tag, ws.group(1).lower() in ("pre", "pre-wrap", "pre-line", "break-spaces")])

        if tag == "div":
            self.div_boundary()
        elif tag == "p":
            self.block(2)
        elif re.fullmatch(r"h[1-6]", tag):
            self.block(2)
            self.raw("#" * int(tag[1]) + " ")
        elif tag == "br":
            if self.line_has_content:
                self.line_break()
            else:
                self._put("\n")
        elif tag == "hr":
            self.block(2)
            self.raw("---")
            self.block(2)
        elif tag in ("strong", "b"):
            self.open_mark("**")
        elif tag in ("em", "i"):
            self.open_mark("*")
        elif tag == "code" and not self.pre_depth:
            self.raw("`")
        elif tag == "pre":
            self.block(2)
            self.raw("```")
            self._put("\n")
            self.pre_depth += 1
        elif tag == "blockquote":
            self.block(2)
            self.quote_depth += 1
        elif tag in ("ul", "ol"):
            self.block(1)
            self.list_stack.append([tag, 0])
        elif tag == "li":
            self.block(1)
            if self.list_stack:
                entry = self.list_stack[-1]
                indent = "  " * (len(self.list_stack) - 1)
                if entry[0] == "ol":
                    entry[1] += 1
                    marker = f"{entry[1]}. "
                else:
                    marker = "- "
                self.parts.append(indent + marker)
                self.line_has_content = True
                self._nl_run = 0
            else:
                self.raw("- ")
        elif tag == "a":
            self.link_href = a.get("href", "")
            self.link_buf = []
        elif tag == "img":
            src = a.get("src", "")
            alt = re.sub(r"\s+", " ", (a.get("alt") or "").strip())
            if is_local_ref(src):
                self.refs.append(src)
            chunk = f"![{alt}]({md_link(src)})"
            if self.link_buf is not None:
                self.link_buf.append(chunk)
            else:
                self.raw(chunk)

    def handle_startendtag(self, tag, attrs):
        self.handle_starttag(tag, attrs)

    def handle_endtag(self, tag):
        if tag in self.SKIP_TAGS:
            self.skip_depth = max(0, self.skip_depth - 1)
            return
        if self.skip_depth:
            return

        if self.ws_stack and self.ws_stack[-1][0] == tag:
            self.ws_stack.pop()

        if tag == "div":
            self.div_boundary()
        elif tag == "p":
            self.block(2)
        elif re.fullmatch(r"h[1-6]", tag):
            self.block(2)
        elif tag in ("strong", "b"):
            self.close_mark("**")
        elif tag in ("em", "i"):
            self.close_mark("*")
        elif tag == "code" and not self.pre_depth:
            self.raw("`")
        elif tag == "pre":
            self.pre_depth = max(0, self.pre_depth - 1)
            self._put("\n")
            self.raw("```")
            self.block(2)
        elif tag == "blockquote":
            self.quote_depth = max(0, self.quote_depth - 1)
            self.block(2)
        elif tag in ("ul", "ol"):
            if self.list_stack:
                self.list_stack.pop()
            self.block(2)
        elif tag == "li":
            self.block(1)
        elif tag == "a":
            href = self.link_href
            inner = "".join(self.link_buf or [])
            self.link_buf = None
            self.link_href = ""
            if href and url_parts(href) is None:
                # Unparsable href (e.g. a mail client's data detector
                # mis-reading plain text as a link) has no usable link
                # target — drop it and keep just the visible text.
                self.raw(inner)
                return
            if is_local_ref(href):
                self.refs.append(href)
            label = inner.strip()
            if href and label == href and is_autolinkable(href):
                self.raw(f"<{href}>")
            elif not label:
                self.raw(f"<{href}>" if href else inner)
            else:
                self.raw(f"[{inner}]({md_link(href)})")

    def handle_data(self, data):
        self.text(data)

    def result(self) -> str:
        md = "".join(self.parts)
        md = re.sub(r"[ \t]*" + HARD_BREAK + r"[ \t]*\n", HARD_BREAK + "\n", md)
        md = re.sub(r"[ \t]+\n", "\n", md)
        md = re.sub(r"\n{3,}", "\n\n", md)
        md = md.strip()
        # A hard break ending a paragraph, a list item (next line starts a
        # new item) or the document breaks nothing.
        md = re.sub(HARD_BREAK + r"(?=\n\n|\n *(?:[-*+]|\d+\.) |\Z)", "", md)
        md = md.replace(HARD_BREAK, "  ")
        return md + "\n"


def html_to_markdown(html: str) -> tuple[str, list[str]]:
    conv = MarkdownConverter()
    conv.feed(html)
    conv.close()
    return conv.result(), conv.refs


def convert(html_dir: Path, out_dir: Path) -> None:
    html_files = sorted(html_dir.glob("*.html"))
    if not html_files:
        sys.exit(f"No .html file found in {html_dir}")

    out_dir.mkdir(parents=True, exist_ok=True)

    copied: list[str] = []
    for item in sorted(html_dir.iterdir()):
        if not item.is_file() or item.suffix.lower() == ".html":
            continue
        dest = out_dir / item.name
        if dest.resolve() == item.resolve():
            continue
        shutil.copy2(item, dest)
        copied.append(item.name)

    for html_file in html_files:
        markdown, refs = html_to_markdown(html_file.read_text(encoding="utf-8"))
        md_path = out_dir / (html_file.stem + ".md")
        md_path.write_text(markdown, encoding="utf-8", newline="\n")
        print(f"markdown -> {md_path}")
        missing = sorted(
            {r for r in refs if is_local_ref(r) and not (out_dir / r).is_file()}
        )
        if missing:
            print(f"  WARNING: {len(missing)} local reference(s) not found: {missing}")

    for name in copied:
        print(f"asset    -> {out_dir / name}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "html_dir",
        type=Path,
        nargs="?",
        help="folder with the .html document and its images "
        "(default: 'html' next to this script)",
    )
    parser.add_argument(
        "-o",
        "--out",
        type=Path,
        default=None,
        help="output folder (default: 'markdown' next to the html folder)",
    )
    args = parser.parse_args()

    html_dir = args.html_dir or (Path(__file__).parent / "html")
    if not html_dir.is_dir():
        sys.exit(f"Not a folder: {html_dir}")

    out_dir = args.out or (html_dir.parent / "markdown")
    convert(html_dir, out_dir)


if __name__ == "__main__":
    main()
