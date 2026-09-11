#!/usr/bin/env python3

import email
import sys
from email import policy
from pathlib import Path

class EmailParseError(Exception):
    """Base exception for email parsing failures."""


class EmailDefectsError(EmailParseError):
    """Raised when the parsed email has structural/MIME defects."""


class EmailEmptyError(EmailParseError):
    """Raised when the email has no usable headers or content."""

def to_path(path_str: str) -> Path:
    eml_path = Path(path_str)
    if not eml_path.is_file():
        raise FileNotFoundError(f"Not a file: {eml_path}")
    return eml_path

def to_email_msg(eml_path: Path) -> "email.message.EmailMessage":
    msg = email.message_from_bytes(eml_path.read_bytes(), policy=policy.default)
    return msg

def parse_email_msg(email_msg: "email.message.EmailMessage") -> dict:

  if email_msg.defects:
    raise EmailDefectsError(f"Email Parsing defects: {email_msg.defects}")

  if (
      not email_msg.get("subject")
      and not email_msg.get("from")
      and not email_msg.is_multipart()
      and not email_msg.get_content()
  ): raise EmailEmptyError("No headers or content found — may not be a valid email")

def main() -> None:

  if len(sys.argv) != 2:
      sys.exit(f"Usage: {sys.argv[0]} <path-to-eml-file>")

  try:
    eml_path = to_path(sys.argv[1])
    email_msg = to_email_msg(eml_path)
  except Exception as e:
      sys.exit(f"Exception: {e}")      

if __name__ == "__main__":
    main()