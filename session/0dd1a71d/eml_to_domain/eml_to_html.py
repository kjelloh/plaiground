#!/usr/bin/env python3
"""Extract the HTML body, inline/attached images, and other document
attachments from an .eml file.

Writes <name>.html plus every image and document part into an output
subfolder (default: "html"), rewriting the HTML's cid: references so the
images resolve as local files, and appending an "Attachments" section of
download links for non-image documents (Pages, PDF, docx, zip, ...).
"""

import argparse
import email
import mimetypes
import re
import sys
from email import policy
from html import escape as html_escape
from pathlib import Path


def sanitize(name: str) -> str:
    name = name.replace("/", "-").replace("\\", "-")
    name = re.sub(r'[:*?"<>|]', "", name)
    name = re.sub(r"\s+", " ", name).strip()
    return name or "unnamed"


def unique_name(filename: str, claimed: set[str]) -> str:
    """filename, or filename with a "-1", "-2", ... suffix if it's already
    claimed. claimed holds casefolded names: on a case-insensitive file
    system (macOS' default) "Mail.TXT" would overwrite "mail.txt"."""
    if filename.casefold() not in claimed:
        claimed.add(filename.casefold())
        return filename
    stem, dot, suffix = filename.rpartition(".")
    stem = stem or filename
    suffix = f".{suffix}" if dot else ""
    i = 1
    while f"{stem}-{i}{suffix}".casefold() in claimed:
        i += 1
    name = f"{stem}-{i}{suffix}"
    claimed.add(name.casefold())
    return name


class NoRenderablePartError(Exception):
    """Raised when the message has no text/html, text/plain, image or
    document part."""


def pick_html_part(msg):
    body = msg.get_body(preferencelist=("html",))
    if body is not None and body.get_content_type() == "text/html":
        return body
    for part in msg.walk():
        if part.get_content_type() == "text/html":
            return part
    return None


def pick_text_part(msg):
    body = msg.get_body(preferencelist=("plain",))
    if body is not None and body.get_content_type() == "text/plain":
        return body
    for part in msg.walk():
        if part.get_content_type() == "text/plain":
            return part
    return None


def plain_text_to_html(text: str) -> str:
    paragraphs = re.split(r"\n\s*\n", text.strip())
    paras_html = [
        f"<p>{html_escape(para).replace(chr(10), '<br>' + chr(10))}</p>"
        for para in paragraphs
        if para.strip()
    ]
    return "<html><body>\n" + "\n".join(paras_html) + "\n</body></html>\n"


def images_only_html(image_names: list[str]) -> str:
    imgs = "\n".join(f'<img src="{name}">' for name in image_names)
    return f"<html><body>\n{imgs}\n</body></html>\n"


def attachments_html(documents: list[tuple[str, str]]) -> str:
    items = "\n".join(
        f'<li><a href="{name}">{html_escape(label)}</a></li>' for name, label in documents
    )
    return f"<h2>Attachments</h2>\n<ul>\n{items}\n</ul>\n"


def append_attachments(html: str, documents: list[tuple[str, str]]) -> str:
    if not documents:
        return html
    section = attachments_html(documents)
    match = re.search(r"</body>", html, flags=re.IGNORECASE)
    if match:
        return html[: match.start()] + section + html[match.start() :]
    return html + section


def image_parts(msg):
    for part in msg.walk():
        if part.get_content_maintype() != "image":
            continue
        if part.get_content_disposition() in ("inline", "attachment") or part.get("Content-ID"):
            yield part


def document_parts(msg):
    """Non-image, non-body attachments (Pages/PDF/docx/zip/...) to be
    linked as downloads rather than embedded."""
    for part in msg.walk():
        if part.get_content_maintype() in ("multipart", "image"):
            continue
        if part.get_content_type() in ("text/plain", "text/html"):
            continue
        if part.get_content_disposition() in ("inline", "attachment") or part.get_filename():
            yield part


def write_part(part, idx: int, prefix: str, out_dir: Path, claimed: set[str]) -> str | None:
    """Write an email part's payload to a uniquely-named file in out_dir.
    Returns the local filename, or None if the part had no payload."""
    payload = part.get_payload(decode=True)
    if not payload:
        return None
    filename = part.get_filename()
    if not filename:
        ext = mimetypes.guess_extension(part.get_content_type()) or ".bin"
        filename = f"{prefix}-{idx}{ext}"
    name = unique_name(sanitize(filename), claimed)
    (out_dir / name).write_bytes(payload)
    return name


def insert_h1(html: str, subject: str) -> str:
    if not subject:
        return html
    heading = f"<h1>{html_escape(subject)}</h1>\n"
    match = re.search(r"<body\b[^>]*>", html, flags=re.IGNORECASE)
    if match:
        return html[: match.end()] + "\n" + heading + html[match.end() :]
    return heading + html


def extract(
    eml_path: Path, out_dir: Path, inject_h1: bool = True, reserved: tuple[str, ...] = ()
) -> None:
    """reserved: file names the caller will itself write next to the
    extracted files — attachments and images never take these names."""
    msg = email.message_from_bytes(eml_path.read_bytes(), policy=policy.default)

    out_dir.mkdir(parents=True, exist_ok=True)

    # Extract images and document attachments first: a fallback body (when
    # there's no text/html or text/plain part) needs their final local
    # filenames to embed/link them.
    cid_to_file: dict[str, str] = {}
    claimed: set[str] = {name.casefold() for name in reserved}

    image_names: list[str] = []
    for idx, part in enumerate(image_parts(msg), start=1):
        name = write_part(part, idx, "image", out_dir, claimed)
        if name is None:
            continue
        image_names.append(name)
        cid = part.get("Content-ID")
        if cid:
            cid_to_file[cid.strip().strip("<>").lower()] = name

    documents: list[tuple[str, str]] = []  # (local filename, display name)
    for idx, part in enumerate(document_parts(msg), start=1):
        display_name = part.get_filename() or f"attachment-{idx}"
        name = write_part(part, idx, "attachment", out_dir, claimed)
        if name is None:
            continue
        documents.append((name, display_name))
        cid = part.get("Content-ID")
        if cid:
            cid_to_file[cid.strip().strip("<>").lower()] = name

    html_part = pick_html_part(msg)
    if html_part is not None:
        html = html_part.get_content()
    else:
        text_part = pick_text_part(msg)
        if text_part is not None:
            html = plain_text_to_html(text_part.get_content())
        elif image_names:
            html = images_only_html(image_names)
        elif documents:
            html = "<html><body></body></html>\n"
        else:
            raise NoRenderablePartError(
                "No text/html, text/plain, image or document part found in the message."
            )

    html = append_attachments(html, documents)

    if inject_h1:
        subject = str(msg["subject"] or "").strip()
        html = insert_h1(html, subject)

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
    for name in image_names:
        print(f"image -> {out_dir / name}")
    for name, _ in documents:
        print(f"attachment -> {out_dir / name}")
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
    try:
        extract(eml_path, out_dir)
    except NoRenderablePartError as e:
        sys.exit(str(e))


if __name__ == "__main__":
    main()
