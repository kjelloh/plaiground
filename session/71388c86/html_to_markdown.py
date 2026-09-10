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

MD_ESCAPE = {ord(c): "\\" + c for c in "\\`*[]"}


def md_link(url: str) -> str:
    url = url.strip()
    if re.search(r"[ ()<>]", url):
        return "<" + url.replace("<", "%3C").replace(">", "%3E") + ">"
    return url


def is_local_ref(url: str) -> bool:
    parts = urlsplit(url)
    return not parts.scheme and not parts.netloc and not url.startswith("#")


class MarkdownConverter(HTMLParser):
    SKIP_TAGS = {"head", "style", "script", "title"}

    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.parts: list[str] = []
        self._nl_run = 2  # treat start-of-document as already blank
        self.line_has_content = False
        self.skip_depth = 0
        self.pre_depth = 0
        self.quote_depth = 0
        self.list_stack: list[list] = []  # [tag, ordinal]
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
            return
        if not self.line_has_content:
            prefix = self._prefix()
            if prefix:
                self.parts.append(prefix)
            self.line_has_content = True
        self.parts.append(ch)
        self._nl_run = 0

    def raw(self, text: str) -> None:
        for ch in text:
            self._put(ch)

    def text(self, data: str) -> None:
        if self.skip_depth:
            return
        if self.pre_depth:
            self.raw(data)
            return
        data = re.sub(r"\s+", " ", data.replace("\xa0", " "))
        if self.link_buf is None and not self.line_has_content:
            data = data.lstrip()
        if not data:
            return
        escaped = data.translate(MD_ESCAPE)
        if self.link_buf is not None:
            self.link_buf.append(escaped)
        else:
            self.raw(escaped)

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

        if tag == "div":
            self.block(1 if (self.quote_depth or self.list_stack) else 2)
        elif tag == "p":
            self.block(2)
        elif re.fullmatch(r"h[1-6]", tag):
            self.block(2)
            self.raw("#" * int(tag[1]) + " ")
        elif tag == "br":
            if self.line_has_content:
                self.parts.append("  ")
            self._put("\n")
        elif tag == "hr":
            self.block(2)
            self.raw("---")
            self.block(2)
        elif tag in ("strong", "b"):
            self.raw("**")
        elif tag in ("em", "i"):
            self.raw("*")
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

        if tag in ("div", "p"):
            self.block(1 if (tag == "div" and (self.quote_depth or self.list_stack)) else 2)
        elif re.fullmatch(r"h[1-6]", tag):
            self.block(2)
        elif tag in ("strong", "b"):
            self.raw("**")
        elif tag in ("em", "i"):
            self.raw("*")
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
            if is_local_ref(href):
                self.refs.append(href)
            label = inner.strip()
            if href and label == href and not re.search(r"[ <>]", href):
                self.raw(f"<{href}>")
            elif not label:
                self.raw(f"<{href}>" if href else inner)
            else:
                self.raw(f"[{inner}]({md_link(href)})")

    def handle_data(self, data):
        self.text(data)

    def result(self) -> str:
        md = "".join(self.parts)
        md = re.sub(r"[ \t]+\n", "\n", md)
        md = re.sub(r"\n{3,}", "\n\n", md)
        return md.strip() + "\n"


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
