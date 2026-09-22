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

class DesignInsufficiencyError(EmailParseError):
    """Rais when the parsing encounters an insufficient to deal with the input data"""


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

class HTML2MarkdownParser(HTMLParser):

    def __init__(self) -> None:
      # convert_charrefs=True tells parser to convert 'character references' to actual unicode code points
      super().__init__(convert_charrefs=True)

      self.log: list[str] = []
      self.trace_ast: list[str] = []

      self.current_markdown_props: dict = {}
      self.markdown: list[str] = [""]

      self.current_html_path: list[str] = []
      self.current_attr: dict = {}
      self.current_data: str = ""
 
    def email_ast(self) -> list[str]:
        if self.current_html_path != []:
            raise IncompleteParseError(
                f"Expected empty 'current html path' after parsing end. Unconsumed ==> {self.current_html_path!r}"
                f"\n<Parse LOG>\n{'\n'.join(self.log)}"
            )        
        return self.trace_ast

    def result(self) -> tuple[list[str],list[str],list[str]]:
        return 
        self.log,
        self.email_ast(),
        self.markdown

    def print_to_log(self,entry: str) -> None:
        print(f"print_to_log:'{entry}'")
        self.log.append(entry)

    # -------------------------------------------------------------------
    # Markdown parser - BEGIN
    # -------------------------------------------------------------------

    def css_style_to_markdown_props(self,css_style_dict:dict) -> tuple[dict,dict]:
        markdown_props:dict = {}
        unconsumed:dict = dict(css_style_dict)
        for name,value in css_style_dict.items():
            log_entry:str = f"path:{'.'.join(self.current_html_path)}[style].{name} = '{value}'"
            
            # process style entries
            if name == "word-wrap":
                # view property only (no markdown mapping for now)
                unconsumed.pop(name,None)
            elif name == "-webkit-nbsp-mode":
                # view property only (no markdown mapping for now)
                unconsumed.pop(name,None)
            elif name == "-webkit-line-break":
                # view property only (no markdown mapping for now)
                unconsumed.pop(name,None)

            if name in unconsumed:           
                self.print_to_log(log_entry + " ?")
            else:
                self.print_to_log(log_entry + " CONSUMED")

        return markdown_props,unconsumed

    def to_css_style_dict(self,css_style_str: str) -> dict:
        result:dict = {}
        # todo: parse e.g., 'word-wrap: break-word; -webkit-nbsp-mode: space; -webkit-line-break: after-white-space;'

        for declaration in css_style_str.split(";"):
            declaration = declaration.strip()
            if not declaration:
                continue  # skip empty for ';;' or trailing ';'

            if ":" not in declaration:
                raise EmailParseError(
                    f"Failed to parse css style attribute:'{css_style_str}'"
                    f"Element:'{declaration}' is not a valid name-value-pair (no ':')"
                )

            prop, value = declaration.split(":", 1)
            prop = prop.strip().lower()
            value = value.strip()

            if prop:
                result[prop] = value

        return result

    def attrs_to_markdown_props(self,html_path: list[str],attrs_dict: dict) -> tuple[dict,dict]:
        unconsumed_attrs = dict(attrs_dict) # clone
        markdown_props: dict = {}

        # apply tag-based attributes
        if html_path == ["html","head","meta"]:
            # No attributes apply
            self.print_to_log(f"path:{'.'.join(html_path)} :  No meta attributes applies = Ignored")
            return markdown_props,{}

        if html_path == ["html","body","meta"]:
            # No attributes apply
            self.print_to_log(f"path:{'.'.join(html_path)} :  No meta attributes applies. Ignored:{attrs_dict}")
            return markdown_props,{}

        # Process html attributes
        for name, value in attrs_dict.items():
            log_entry:str = f"path:{'.'.join(html_path)}[attr:{name}] = '{value}'"
            if name=="class":
                if value=="":
                    unconsumed_attrs.pop(name,None)

            elif name=="style":
                css_style_dict = self.to_css_style_dict(value)
                css_md_props,unconsumed_style_attrs = self.css_style_to_markdown_props(css_style_dict)
                markdown_props.update(css_md_props)

                if unconsumed_style_attrs:
                    unconsumed_attrs["style"] = "; ".join(
                        f"{k}: {v}" for k, v in unconsumed_style_attrs.items()
                    )
                else:
                    unconsumed_attrs.pop(name, None)

            if name in unconsumed_attrs:
                self.print_to_log(log_entry + " ?")
            else:
                self.print_to_log(log_entry + " CONSUMED")

        return markdown_props,unconsumed_attrs

    def to_markdown_apply_void_html(self,tag,attrs_dict: dict) -> None:
        self.print_to_log(f"path:{'.'.join(self.current_html_path)}.{tag} :  to_markdown_apply_void_html: attrs_dict:{attrs_dict}")
        # Expect no unconsumed attributes
        if self.current_attr:
            raise IncompleteParseError(
                f"path:{'.'.join(self.current_html_path)}.{tag} :  Expected empty current attrs on void html attrs:{attrs_dict}"
                f" unconsumed ==> {self.current_attr}"
                f"\n<Parse LOG>\n{'\n'.join(self.log)}"
            )

        # Apply attributes
        markdown_props,unconsumed_attrs = self.attrs_to_markdown_props(
            self.current_html_path + [tag],
            attrs_dict
        )
        self.current_markdown_props.update(markdown_props)
        self.current_attr.update(unconsumed_attrs)

        # apply formatting tag
        if self.current_html_path[-1] == "br":
            self.markdown.append("")

        return

    def to_markdown_apply_open_html(self,attrs_dict: dict) -> None:
        self.print_to_log(f"path:{'.'.join(self.current_html_path)} :  to_markdown_apply_open_html: attrs_dict:{attrs_dict}")
        if self.current_attr:
            log_text = "\n".join(self.log)
            raise IncompleteParseError(
                f"path:{'.'.join(self.current_html_path)} :  Expected empty current attrs on open html attrs:{attrs_dict}"
                f" unconsumed ==> {self.current_attr}"
                f"\n<Parse LOG>\n{log_text}"
            )

        # Apply attributes
        markdown_props,unconsumed_attrs = self.attrs_to_markdown_props(
            self.current_html_path,
            attrs_dict
        )

        self.current_markdown_props.update(markdown_props)
        self.current_attr.update(unconsumed_attrs)

        return

    def to_markdown_apply_data(self,data: str) -> None:
        self.print_to_log(f"path:{'.'.join(self.current_html_path)} :  to_markdown_apply_data: data:{len(data)} chars")
        if self.current_data != "":
            raise IncompleteParseError(
                f"path:{'.'.join(self.current_html_path)} :  Expected empty (consumed) current data on open new html data"
                f"\n\tunconsumed ==> '{self.current_data}'"
                f"\n\tdata:{data}"
                f"\n<Parse LOG>\n{'\n'.join(self.log)}"
            )

        # Store for processing when html tag is closed (or new tag is opened?) 
        self.current_data = data

        return

    def to_markdown_apply_close_html(self) -> None:
        self.print_to_log(f"path:{'.'.join(self.current_html_path)} :  to_markdown_apply_close_html")
        if self.current_attr:
            raise IncompleteParseError(
                f"path:{'.'.join(self.current_html_path)} :  Expected empty unconsumed attrs on close html"
                f" unconsumed ==> {self.current_attr}"
                f"\n<Parse LOG>\n{'\n'.join(self.log)}"
            )

        # Process any data stored for closed tag
        if self.current_data != "":
            if any(ord(c) < ord(' ') for c in self.current_data):
                raise DesignInsufficiencyError(
                    f"path:{'.'.join(self.current_html_path)} :  Control characters in data (text) not yet supported"
                )

            # The markdown list always contains at least one entry.
            self.markdown[-1] += self.current_data
            self.current_data = "" # consumed

        if self.current_data != "":
            raise IncompleteParseError(
                f"path:{'.'.join(self.current_html_path)} :  Expected empty (consumed) current data on close html"
                f"\n\tunconsumed ==> '{self.current_data}'"
                f"\n<Parse LOG>\n{'\n'.join(self.log)}"
            )
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
        "area",
    }    

    # defines what tags are auto closed by a new start tag
    AUTO_CLOSE_ON_START = {
        "body":   {"head"},   # a <body> tag auto-closes <head> tag
    }

    def handle_starttag(self, tag, attrs):
        attrs_dict = dict(attrs)
        if tag in self.VOID_ELEMENTS:
          attrs_str = f" attrs:{attrs}" if attrs else ""
          self.print_to_log(f"{'.'.join(self.current_html_path)} Encountered void tag:{tag} {attrs_str}")
          self.to_markdown_apply_void_html(tag,attrs_dict)
        else:
          self.print_to_log(f"{'.'.join(self.current_html_path)} Encountered start tag:{tag}")
          while self.current_html_path and self.current_html_path[-1] in self.AUTO_CLOSE_ON_START.get(tag,()):
              print(f"{'.'.join(self.current_html_path)} auto-closed")
              self.current_html_path.pop()
          self.current_html_path.append(tag)
          self.trace_ast.append(f"{'.'.join(self.current_html_path)}")
          self.to_markdown_apply_open_html(attrs_dict)
            
    def handle_endtag(self, tag):
        self.print_to_log(f"{'.'.join(self.current_html_path)} Encountered end tag:{tag}")
        if not self.current_html_path or self.current_html_path[-1] != tag:
            raise IncompleteParseError(
                f"End tag <{tag}> does not match current_html_path:'"
                f"{'.'.join(self.current_html_path)}'"
                f"\n<Parse LOG>\n{'\n'.join(self.log)}"
            )
        self.to_markdown_apply_close_html()
        self.current_html_path.pop()
        self.trace_ast.append(f"{".".join(self.current_html_path)}")

    def handle_data(self, data):
        self.print_to_log(f"{'.'.join(self.current_html_path)} Encountered some data:{data}")
        self.trace_ast.append(f"{".".join(self.current_html_path)} = {data}")
        self.to_markdown_apply_data(data)

    # -------------------------------------------------------------------
    # HTML Parser - END
    # -------------------------------------------------------------------

# -------------------------------------------------------------------
# Mail Parsers - BEGIN
# -------------------------------------------------------------------

def parse_text_plain(content_path: list[str],plain_str: str) -> tuple[list[str],list[str],list[str]]:
    log: list[str] = [f"Parsing:{'.'.join(content_path)} = {plain_str}"]
    ast: list[str] = []
    markdown: list[str] = []
    markdown.append(plain_str)
    return log,ast,markdown

def parse_text_html(content_path: list[str],html_str: str) -> tuple[list[str],list[str],list[str]]:
    log: list[str] = [f"Parsing:{'.'.join(content_path)} = {html_str}"]
    html2markdown_parser = HTML2MarkdownParser()
    html2markdown_parser.feed(html_str)
    parser_log,parser_ast,parser_markdown = html2markdown_parser.reset()
    log.extend(parser_log)
    return log,parser_ast,parser_markdown
    
def dfs(parent_path: list[str],part: EmailMessage) -> tuple[list[str],list[str],list[str]]:
    log: list[str] = [f"Parsing:{'.'.join(parent_path)}"]
    ast: list[str] = []
    markdown_from_plain: list[str] = []
    markdown_from_html: list[str] = []
    content_type = part.get_content_type()
    current_content_path = parent_path + [content_type]
    current_content_path = parent_path + [content_type]
    if part.is_multipart():
        children = part.get_payload()  # list[EmailMessage] when multipart
        for i, child in enumerate(children):
          child_log,child_ast,child_markdown = dfs(current_content_path,child)
    else:
        if content_type == "text/plain":
            child_log,child_ast,child_markdown = parse_text_plain(current_content_path,part.get_content())
            print(f"{'\n'.join(child_log)}")
            log.extend(child_log)
            ast.extend(child_ast)
            markdown_from_plain.extend(child_markdown)
        elif content_type == "text/html":
            child_log,child_ast,child_markdown = parse_text_html(current_content_path,part.get_content())
            print(f"{'\n'.join(child_log)}")
            log.extend(child_log)
            ast.extend(child_ast)
            markdown_from_html.extend(child_markdown)
        else:
            raise UnsupportedContentTypeError(
                f"{'.'.join(current_content_path)}"
            )

    if len(markdown_from_plain) > 0 and len(markdown_from_html) == 0:
      return log,ast,markdown_from_plain
    elif len(markdown_from_html) > 0:
      return log,ast,markdown_from_html
    else:
        raise EmailParseError(
            f"No text/plain nor text/html found in mail"
        )

# -------------------------------------------------------------------
# Mail Parsers - END
# -------------------------------------------------------------------

def eml_file_to_markdown(eml_path: Path) -> None:
    print(f"\n--------------------------------------\nSTART PROCESSING: {eml_path}")
    email_msg = to_email_msg(eml_path)


    # -------------------------------------------------------------------
    # Mail Meta/Chime - BEGIN
    # -------------------------------------------------------------------

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

    with chime_path.open("a", encoding="utf-8") as f:
        f.write(f"{date_line}\n\n")

    # -------------------------------------------------------------------
    # Mail Meta/Chime - END
    # -------------------------------------------------------------------

    log,ast,markdown = dfs([],email_msg)
    print("\n".join(ast))

    email_part_tree_string = to_email_part_tree_string(email_msg)
    print(email_part_tree_string)


    tree_path = chime_path.with_name("tree.md")
    with tree_path.open("a", encoding="utf-8") as f:
        f.write(email_part_tree_string)

    log_path = chime_path.with_name("log.md")
    with log_path.open("a", encoding="utf-8") as f:
        f.write('\n'.join(log))

    with chime_path.open("a", encoding="utf-8") as f:
        f.write('\n'.join(markdown))
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
