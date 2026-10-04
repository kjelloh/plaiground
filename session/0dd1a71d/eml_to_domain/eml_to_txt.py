#!/usr/bin/env python3
"""Extract the plain text (text/plain) body of an .eml file.

Writes the decoded text verbatim (line endings normalised to \\n, UTF-8
with a BOM so browsers pick the encoding up) to <name>.txt. Mails without
a text/plain body part (e.g. HTML-only, or
images/attachments only) produce no file — there is no original plain
text to preserve, and deriving one from the HTML would not be the mail's
own text.
"""

import argparse
import email
import sys
from email import policy
from pathlib import Path

from eml_to_html import sanitize

# A UTF-8 byte order mark is the one in-file signal browsers obey for a
# plain .txt file (it outranks the server's Content-Type and their own
# guessing). Without it a server that sends "text/plain" with no charset
# — Python's http.server, many web hotels — leaves the browser guessing,
# typically Windows-1252, and å/ä/ö render as "Ã¥"/"Ã¤"/"Ã¶".
PLAIN_TEXT_ENCODING = "utf-8-sig"


def pick_text_body_part(msg):
    """The mail's text/plain body part, or None. Like
    eml_to_html.pick_text_part, but never picks an attached .txt file."""
    body = msg.get_body(preferencelist=("plain",))
    if body is not None and body.get_content_type() == "text/plain":
        return body
    for part in msg.walk():
        if part.get_content_type() == "text/plain" and part.get_content_disposition() != "attachment":
            return part
    return None


def plain_text_of(msg) -> str | None:
    part = pick_text_body_part(msg)
    if part is None:
        return None
    return part.get_content().replace("\r\n", "\n")


def write_plain_text(path: Path, text: str) -> None:
    path.write_text(text, encoding=PLAIN_TEXT_ENCODING)


def extract(eml_path: Path, out_path: Path) -> bool:
    """Write eml_path's plain text body to out_path. Returns False (and
    writes nothing) if the mail has no text/plain body part."""
    msg = email.message_from_bytes(eml_path.read_bytes(), policy=policy.default)
    text = plain_text_of(msg)
    if text is None:
        return False
    out_path.parent.mkdir(parents=True, exist_ok=True)
    write_plain_text(out_path, text)
    return True


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("eml", type=Path, help="path to the .eml file")
    parser.add_argument(
        "-o",
        "--out",
        type=Path,
        default=None,
        help="output .txt file (default: <eml name>.txt next to the .eml file)",
    )
    args = parser.parse_args()

    if not args.eml.is_file():
        sys.exit(f"Not a file: {args.eml}")

    out_path = args.out or (args.eml.parent / (sanitize(args.eml.stem) + ".txt"))
    if not extract(args.eml, out_path):
        sys.exit("No text/plain body part found in the message.")
    print(f"text -> {out_path}")


if __name__ == "__main__":
    main()
