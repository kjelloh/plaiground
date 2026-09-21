#!/usr/bin/env python3

import email
from operator import index
import sys
from email import policy
from email.utils import parsedate_to_datetime
from email.message import EmailMessage
from pathlib import Path
# See https://docs.python.org/3/library/html.parser.html
from html.parser import HTMLParser

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


class ChimeAlreadyExistsError(Exception):
    """Rais when a chime already exists for this eml file's name.

    The eml file name is used as the chime key, so this only fires when
    the same eml file is processed more than once — a guard against
    accidentally reprocessing (and silently reusing or overwriting) an
    already-imported mail.
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

class IncompleteParseError(ValueError):
    """Raised when parsing did not consume a well-formed document."""

class MyHTMLParser(HTMLParser):

    def __init__(self) -> None:
      # convert_charrefs=True tells parser to convert 'character references' to actual unicode code points
      super().__init__(convert_charrefs=True)
      self.current_html_path: list[str] = []
      self.current_attr: dict = {}
      self.current_data: str = ""
      self.trace_ast: list[str] = []

    def email_ast(self) -> list[str]:
        if self.current_html_path != []:
            raise IncompleteParseError(
                f"Expected empty 'current html path' after parsing end. Unconsumed ==> {self.current_html_path!r}"
            )        
        return self.trace_ast

    # -------------------------------------------------------------------
    # Markdown parser - BEGIN
    # -------------------------------------------------------------------

    def to_markdown_apply_void_html(self,tag,attrs_dict: dict) -> None:
        print(f"{'.'.join(self.current_html_path)}: to_markdown_apply_void_html: attrs_dict:{attrs_dict}")
        if self.current_attr:
            raise IncompleteParseError(
                f"{'.'.join(self.current_html_path)}: Expected empty current attrs on void html attrs:{attrs_dict}"
                f" unconsumed ==> {self.current_attr}"
            )
        
        return

    def to_markdown_apply_open_html(self,attrs_dict: dict) -> None:
        print(f"{'.'.join(self.current_html_path)}: to_markdown_apply_open_html: attrs_dict:{attrs_dict}")
        if self.current_attr:
            raise IncompleteParseError(
                f"{'.'.join(self.current_html_path)}: Expected empty current attrs on open html attrs:{attrs_dict}"
                f" unconsumed ==> {self.current_attr}"
            )
        return

    def to_markdown_apply_data(self,data: str) -> None:
        print(f"{'.'.join(self.current_html_path)}: to_markdown_apply_data: data:{len(data)} chars")
        if self.current_data != "":
            raise IncompleteParseError(
                f"{'.'.join(self.current_html_path)}: Expected empty (consumed) current data on open new html data"
                f" unconsumed ==> '{data}'"
            )

        # TODO: Apply proper processing.
        #       Current code only to trigger unprocessed data for now
        self.current_data = data
        return

    def to_markdown_apply_close_html(self) -> None:
        print(f"{'.'.join(self.current_html_path)}: to_markdown_apply_close_html")
        return

    # -------------------------------------------------------------------
    # Markdown parser - END
    # -------------------------------------------------------------------

    # -------------------------------------------------------------------
    # HTML Parser - BEGIN
    # -------------------------------------------------------------------

    # tags that are 'void' as in has no end tag
    # See https://html.spec.whatwg.org/multipage/syntax.html#void-elements
    VOID_ELEMENTS = {
        "br",
        "img",
        "meta",
        "col",
        "link",
        "base",
        "hr",
        "input",      
        "wbr",        # The wbr element represents a line break opportunity.
        "area",       # 
    }    

    # defines what tags is auto closed by a new start tag
    AUTO_CLOSE_ON_START = {
        "body":   {"head"},   # a <body> tag auto-closes <head> tag
    }

    def handle_starttag(self, tag, attrs):
        attrs_dict = dict(attrs)
        if tag in self.VOID_ELEMENTS:
          attrs_str = f" attrs:{attrs}" if attrs else ""
          print(f"{'.'.join(self.current_html_path)} Encountered void tag:{tag} {attrs_str}")
          self.to_markdown_apply_void_html(tag,attrs_dict)
        else:
          print(f"{'.'.join(self.current_html_path)} Encountered start tag:{tag}")
          while self.current_html_path and self.current_html_path[-1] in self.AUTO_CLOSE_ON_START.get(tag,()):
              print(f"{'.'.join(self.current_html_path)} auto-closed")
              self.current_html_path.pop()
          self.current_html_path.append(tag)
          self.trace_ast.append(f"{'.'.join(self.current_html_path)}")
          self.to_markdown_apply_open_html(attrs_dict)
            
    def handle_endtag(self, tag):
        print(f"{'.'.join(self.current_html_path)} Encountered end tag:{tag}")
        if not self.current_html_path or self.current_html_path[-1] != tag:
            raise IncompleteParseError(
                f"End tag <{tag}> does not match current_html_path:'"
                f"{'.'.join(self.current_html_path)}'"
            )
        self.to_markdown_apply_close_html()
        self.current_html_path.pop()
        self.trace_ast.append(f"{".".join(self.current_html_path)}")

    def handle_data(self, data):
        print(f"{'.'.join(self.current_html_path)} Encountered some data:{data}")
        self.trace_ast.append(f"{".".join(self.current_html_path)} = {data}")
        self.to_markdown_apply_data(data)

    # -------------------------------------------------------------------
    # HTML Parser - END
    # -------------------------------------------------------------------

def parse_text_plain(content_path: list[str],plain_str: str) -> tuple[list[str],list[str],list[str]]:
    log: list[str] = []
    ast: list[str] = []
    markdown: list[str] = []
    raise UnsupportedContentTypeError(
        f"{'.'.join(content_path)} = '{plain_str}'"
    )
    return log,ast,markdown
    

def to_html_ast(html_str: str) -> list[str]:
    html_parser = MyHTMLParser()
    html_parser.feed(html_str)
    return html_parser.email_ast()
    
def to_email_ast(parent_path: list[str],part: EmailMessage) -> list:
    email_ast = []
    content_type = part.get_content_type()
    current_content_path = parent_path + [content_type]
    if part.is_multipart():
        children = part.get_payload()  # list[EmailMessage] when multipart
        for i, child in enumerate(children):
          email_ast.extend(to_email_ast(current_content_path, child))
    else:
        if content_type == "text/plain":
            log,ast,markdown = parse_text_plain(current_content_path,part.get_content())
            email_ast.append(".".join(current_content_path) + "=" + "\n".join(ast))
        elif content_type == "text/html":
            email_ast.append(".".join(current_content_path) + "=" + "\n".join(to_html_ast(part.get_content())))

    return email_ast
          
def eml_file_to_markdown(eml_path: Path) -> None:
    print(f"\n\nSTART PROCESSING: {eml_path}")
    email_msg = to_email_msg(eml_path)
    email_meta = parse_email_meta(email_msg)

    subject = email_meta["subject"]
    if not subject:
        raise EmailMissingSubjectError("Email has no Subject header — cannot name a chime")

    chime_path, created = ensure_entry_folder("chime", eml_path.stem, base_dir=SESSION_DIR)
    if not created:
        raise ChimeAlreadyExistsError(
            f"chime already exists for eml file {eml_path.name!r} — "
            "already processed"
        )

    date_line = email_meta["date"].isoformat() if email_meta["date"] else "(no date)"

    email_ast = to_email_ast([],email_msg)
    print("\n".join(email_ast))

    email_part_tree_string = to_email_part_tree_string(email_msg)
    print(email_part_tree_string)

    with chime_path.open("a", encoding="utf-8") as f:
        f.write(f"{date_line}\n\n")
        f.write(email_part_tree_string)
    print(f"END PROCESSING: {eml_path.name} -> {chime_path}")

def main() -> None:

    if len(sys.argv) != 2:
        sys.exit(f"Usage: {sys.argv[0]} <path-to-eml-file>")

    try:
        eml_file_to_markdown(to_path(sys.argv[1]))
    except Exception as e:
        sys.exit(f"Exception: {e}")


if __name__ == "__main__":
    main()
