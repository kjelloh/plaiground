#!/usr/bin/env python3
"""Extract the HTML body and inline/attached images from an .eml file.

First version: writes <name>.html plus every image part into an output
subfolder (default: "html"), rewriting the HTML's cid: references so the
images resolve as local files.
"""

import argparse
import email
import mimetypes
import re
import sys
from email import policy
from pathlib import Path


def sanitize(name: str) -> str:
    name = name.replace("/", "-").replace("\\", "-")
    name = re.sub(r'[:*?"<>|]', "", name)
    name = re.sub(r"\s+", " ", name).strip()
    return name or "unnamed"


def unique_path(directory: Path, filename: str) -> Path:
    candidate = directory / filename
    if not candidate.exists():
        return candidate
    stem, suffix = candidate.stem, candidate.suffix
    i = 1
    while True:
        candidate = directory / f"{stem}-{i}{suffix}"
        if not candidate.exists():
            return candidate
        i += 1


def pick_html_part(msg):
    body = msg.get_body(preferencelist=("html",))
    if body is not None and body.get_content_type() == "text/html":
        return body
    for part in msg.walk():
        if part.get_content_type() == "text/html":
            return part
    return None


def image_parts(msg):
    for part in msg.walk():
        if part.get_content_maintype() != "image":
            continue
        if part.get_content_disposition() in ("inline", "attachment") or part.get("Content-ID"):
            yield part


def extract(eml_path: Path, out_dir: Path) -> None:
    msg = email.message_from_bytes(eml_path.read_bytes(), policy=policy.default)

    html_part = pick_html_part(msg)
    if html_part is None:
        sys.exit("No text/html part found in the message.")
    html = html_part.get_content()

    out_dir.mkdir(parents=True, exist_ok=True)

    cid_to_file: dict[str, str] = {}
    written: list[str] = []
    for idx, part in enumerate(image_parts(msg), start=1):
        payload = part.get_payload(decode=True)
        if not payload:
            continue
        filename = part.get_filename()
        if not filename:
            ext = mimetypes.guess_extension(part.get_content_type()) or ".bin"
            filename = f"image-{idx}{ext}"
        target = unique_path(out_dir, sanitize(filename))
        target.write_bytes(payload)
        written.append(target.name)

        cid = part.get("Content-ID")
        if cid:
            cid_to_file[cid.strip().strip("<>").lower()] = target.name

    def replace_cid(match: re.Match) -> str:
        key = match.group("cid").strip().strip("<>").lower()
        local = cid_to_file.get(key)
        return local if local else match.group(0)

    html = re.sub(
        r'cid:(?P<cid>[^"\'>\s)]+)',
        replace_cid,
        html,
        flags=re.IGNORECASE,
    )

    html_name = sanitize(eml_path.stem) + ".html"
    html_target = out_dir / html_name
    html_target.write_text(html, encoding="utf-8")

    print(f"HTML  -> {html_target}")
    for name in written:
        print(f"image -> {out_dir / name}")
    unresolved = re.findall(r'cid:[^"\'>\s)]+', html)
    if unresolved:
        print(f"WARNING: {len(unresolved)} unresolved cid reference(s): {unresolved}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("eml", type=Path, nargs="?", help="path to the .eml file")
    parser.add_argument(
        "-o",
        "--out",
        type=Path,
        default=None,
        help="output directory (default: 'html' next to the .eml file)",
    )
    args = parser.parse_args()

    eml_path = args.eml
    if eml_path is None:
        emls = sorted(Path(__file__).parent.glob("*.eml"))
        if len(emls) != 1:
            sys.exit("Specify the .eml path (found %d in script folder)." % len(emls))
        eml_path = emls[0]
    if not eml_path.is_file():
        sys.exit(f"Not a file: {eml_path}")

    out_dir = args.out or (eml_path.parent / "html")
    extract(eml_path, out_dir)


if __name__ == "__main__":
    main()
