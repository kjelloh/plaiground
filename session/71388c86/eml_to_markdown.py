#!/usr/bin/env python3

import email
from operator import index
import sys
from email import policy
from email.utils import parsedate_to_datetime
from email.message import EmailMessage
from pathlib import Path

from init_new import ensure_entry_folder

SESSION_DIR = Path(__file__).resolve().parent


class EmailParseError(Exception):
    """Base exception for email parsing failures."""


class EmailDefectsError(EmailParseError):
    """Rais when the parsed email has structural/MIME defects."""


class EmailEmptyError(EmailParseError):
    """Rais when the email has no usable headers or content."""


class EmailMissingSubjectError(EmailParseError):
    """Rais when the email has no Subject header — a chime cannot be named without one."""


class UnsupportedContentTypeError(Exception):
    """Rais when the email has parts with not-yet-supported content type"""


class ChimeVariantsNotSupportedError(Exception):
    """Rais when a chime already exists for this subject.

    Multiple mails sharing a subject should eventually become versions of
    the same chime; that isn't implemented yet, so this is raised instead
    of silently reusing or overwriting the existing chime.md.
    """


def to_path(path_str: str) -> Path:
    eml_path = Path(path_str)
    if not eml_path.is_file():
        raise FileNotFoundError(f"Not a file: {eml_path}")
    return eml_path


def to_email_msg(eml_path: Path) -> "email.message.EmailMessage":
    msg = email.message_from_bytes(eml_path.read_bytes(), policy=policy.default)
    return msg

# Part of 'print_email_part_tree'
def to_child_prefix_string(prefix: str, parent_is_last: bool) -> str:
    return prefix + ("    " if parent_is_last else "│   ")

def to_part_descriptor_string(part: EmailMessage) -> str:
    content_type = part.get_content_type()
    disposition = part.get_content_disposition()  # 'attachment' | 'inline' | None
    content_id = part.get("Content-ID")
    filename = part.get_filename()

    details = []
    if disposition:
        details.append(disposition)
    if filename:
        details.append(f"filename={filename!r}")
    if content_id:
        details.append(f"cid={content_id}")
    if not part.is_multipart():
        try:
            size = len(part.get_content())
            details.append(f"{size} chars/bytes")
        except Exception:
            pass

    suffix = f" ({', '.join(details)})" if details else ""
    return f"{content_type}{suffix}"

# Part of 'to_email_part_tree_string'
def to_part_node_string(part: EmailMessage, depth: int, prefix: str) -> str:
    """Build a single node's label line, then recurse into its children if any."""
    lines = [to_part_descriptor_string(part)]
    if part.is_multipart():
        children = part.get_payload()
        for i, child in enumerate(children):
            is_last = i == len(children) - 1
            connector = "└── " if is_last else "├── "
            child_prefix = to_child_prefix_string(prefix, is_last)
            child_string = to_part_node_string(child, depth + 1, child_prefix)
            lines.append(f"{prefix}{connector}{child_string}")
    return "\n".join(lines)

# builds the email structure as a Unix 'tree'-style string
def to_email_part_tree_string(email_msg: EmailMessage, _depth: int = 0, _prefix: str = "") -> str:
    """Build the MIME structure of an email as a string, similar to the Unix `tree` command.

    Recurses manually (rather than using .walk()) so that indentation
    reflects actual nesting depth, not just visitation order.
    """
    label = to_part_descriptor_string(email_msg)
    lines = [f"{_prefix}{label}"]

    if email_msg.is_multipart():
        children = email_msg.get_payload()  # list[EmailMessage] when multipart
        for i, child in enumerate(children):
            is_last = i == len(children) - 1
            connector = "└── " if is_last else "├── "
            child_prefix = to_child_prefix_string(_prefix, is_last)
            child_string = to_part_node_string(child, _depth + 1, child_prefix)
            lines.append(f"{_prefix}{connector}{child_string}")

    return "\n".join(lines)

# IMF: https://www.rfc-editor.org/info/rfc5322/
# Group From/To: https://www.rfc-editor.org/info/rfc6854/
# MIME Body: https://www.rfc-editor.org/info/rfc2045/
# MIME Media: https://www.rfc-editor.org/info/rfc2046/
# MIME non-ASCII,non-textual,multipart: https://www.rfc-editor.org/info/rfc2049/
# Original Mail: https://www.w3.org/Protocols/rfc822/
# SMTP: https://www.rfc-editor.org/info/rfc5321/
# Python: https://docs.python.org/3/library/email.parser.html#module-email.parser
# Python: https://docs.python.org/3/library/email.examples.html#parsing-a-message-from-a-file
def parse_email_meta(email_msg: "email.message.EmailMessage") -> dict:

    if email_msg.defects:
        raise EmailDefectsError(f"Email Parsing defects: {email_msg.defects}")

    if (
        not email_msg.get("subject")
        and not email_msg.get("from")
        and not email_msg.is_multipart()
        and not email_msg.get_content()
    ):
        # Truly empty email (no headers, no content)
        raise EmailEmptyError("No headers or content found — may not be a valid email")

    raw_date = email_msg.get("date")
    parsed_date = None
    if raw_date:
        try:
            parsed_date = parsedate_to_datetime(raw_date)
        except (TypeError, ValueError):
            parsed_date = None

    return {
        "subject": (
            email_msg.get("subject", "").strip() if email_msg.get("subject") else None
        ),
        "from": email_msg.get("from", "").strip() if email_msg.get("from") else None,
        "date": parsed_date,  # datetime object (or None)
    }

def to_email_ast(parent_path: list,part: EmailMessage) -> list:
    result = []
    content_type = part.get_content_type()
    current_path = parent_path + [content_type]
    if part.is_multipart():
        children = part.get_payload()  # list[EmailMessage] when multipart
        for i, child in enumerate(children):
          result.extend(to_email_ast(current_path, child))
    else:
        if content_type == "text/plain":
            result.append(".".join(current_path) + "=" + part.get_content())
        elif content_type == "text/html":
            result.append(".".join(current_path) + "=" + part.get_content())

    return result
          
def eml_file_to_markdown(eml_path: Path) -> None:
    email_msg = to_email_msg(eml_path)
    email_meta = parse_email_meta(email_msg)

    subject = email_meta["subject"]
    if not subject:
        raise EmailMissingSubjectError("Email has no Subject header — cannot name a chime")

    chime_path, created = ensure_entry_folder("chime", subject, base_dir=SESSION_DIR)
    if not created:
        raise ChimeVariantsNotSupportedError(
            f"chime already exists for subject {subject!r} — "
            "chime variants not yet supported"
        )

    email_ast = to_email_ast([],email_msg)
    print("\n".join(email_ast))

    email_part_tree_string = to_email_part_tree_string(email_msg)
    print(email_part_tree_string)

    with chime_path.open("a", encoding="utf-8") as f:
        f.write(email_part_tree_string)
    print(f"OK: {eml_path.name} -> {chime_path}")

def main() -> None:

    if len(sys.argv) != 2:
        sys.exit(f"Usage: {sys.argv[0]} <path-to-eml-file>")

    try:
        eml_file_to_markdown(to_path(sys.argv[1]))
    except Exception as e:
        sys.exit(f"Exception: {e}")


if __name__ == "__main__":
    main()
