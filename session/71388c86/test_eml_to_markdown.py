import email
from datetime import datetime, timezone
from email import policy
from pathlib import Path

import pytest

import eml_to_markdown
from eml_to_markdown import (
    ChimeVariantsNotSupportedError,
    EmailDefectsError,
    EmailEmptyError,
    EmailMissingSubjectError,
    eml_path_to_markdown_folder,
    parse_email_msg,
    to_email_msg,
    to_path,
)

# BEWARE: tmp_path is a default pytest fixture.
#         pytest uses introspection to find the test functions based on a valid ficture argument
#         You can define and name a proprietary fixture (see pytest docs)

def test_to_path_missing_file(tmp_path):
    path_to_non_existing_eml = tmp_path / "dummy.eml"
    with pytest.raises(FileNotFoundError):
        to_path(str(path_to_non_existing_eml))

def test_to_path_existing_file(tmp_path):
    test_eml_path = tmp_path / "test.eml"
    test_eml_path.write_text("From: foo@bar.se\n")
    result = to_path(str(test_eml_path))
    assert result == test_eml_path

def test_malformed_multipart_email_msg():
    # multipart Content-Type missing the required 'boundary' parameter
    # is considered malformed (RFC 2046 §5.1.1)
    raw_missing_boundary = b"Content-Type: multipart/mixed\r\n\r\nfoo"
    msg = email.message_from_bytes(raw_missing_boundary, policy=policy.default)
    with pytest.raises(EmailDefectsError):
        parse_email_msg(msg)

def test_parse_empty_email_msg():
    raw = b"Content-Type: text/plain\r\n\r\n"
    msg = email.message_from_bytes(raw, policy=policy.default)
    with pytest.raises(EmailEmptyError):
      parse_email_msg(msg)

def test_parse_ok_email_msg(tmp_path):
    test_eml_path = tmp_path / "test.eml"
    test_eml_path.write_text(
        "Subject: Hello\nFrom: foo@bar.se\nDate: Fri, 11 Sep 2026 12:00:00 +0000\n\nBody text\n"
    )
    msg = to_email_msg(to_path(str(test_eml_path)))
    result = parse_email_msg(msg)
    assert result["subject"] == "Hello"
    assert result["from"] == "foo@bar.se"
    assert result["date"] == datetime(2026, 9, 11, 12, 0, tzinfo=timezone.utc)

def test_parse_email_msg_missing_date_is_none(tmp_path):
    test_eml_path = tmp_path / "test.eml"
    test_eml_path.write_text("Subject: Hello\nFrom: foo@bar.se\n\nBody text\n")
    msg = to_email_msg(to_path(str(test_eml_path)))
    result = parse_email_msg(msg)
    assert result["date"] is None

def test_parse_mail_malformed_date(tmp_path):
    test_eml_path = tmp_path / "test.eml"
    test_eml_path.write_text(
        "Subject: Hello\nFrom: foo@bar.se\nDate: not-a-real-date\n\nBody text\n"
    )
    msg = to_email_msg(to_path(str(test_eml_path)))
    result = parse_email_msg(msg)
    assert result["date"] is None

def test_parse_email_msg_parts_single_text_plain(tmp_path):
    test_eml_path = tmp_path / "test.eml"
    test_eml_path.write_text("Subject: Hello\nFrom: foo@bar.se\n\nBody text\n")
    msg = to_email_msg(to_path(str(test_eml_path)))
    result = parse_email_msg(msg)
    assert result["parts"] == [
        {"index": 0, "content_disposition": None, "content_type": "text/plain"}
    ]

def test_parse_email_msg_parts_multipart_text_plain_only():
    raw = (
        "From: foo@bar.se\r\n"
        "Subject: Multi\r\n"
        'Content-Type: multipart/mixed; boundary="BOUNDARY"\r\n'
        "\r\n"
        "--BOUNDARY\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Body text\r\n"
        "--BOUNDARY\r\n"
        "Content-Type: text/plain\r\n"
        'Content-Disposition: attachment; filename="note.txt"\r\n'
        "\r\n"
        "Attachment text\r\n"
        "--BOUNDARY--\r\n"
    ).encode()
    msg = email.message_from_bytes(raw, policy=policy.default)
    result = parse_email_msg(msg)
    assert result["parts"] == [
        {"index": 1, "content_disposition": None, "content_type": "text/plain"},
        {"index": 2, "content_disposition": "attachment", "content_type": "text/plain"},
    ]

def test_eml_path_to_markdown_folder_missing_subject_raises(tmp_path):
    test_eml_path = tmp_path / "test.eml"
    test_eml_path.write_text("From: foo@bar.se\n\nBody text\n")
    with pytest.raises(EmailMissingSubjectError):
        eml_path_to_markdown_folder(to_path(str(test_eml_path)))

def test_eml_path_to_markdown_folder_creates_chime(tmp_path, monkeypatch):
    monkeypatch.setattr(eml_to_markdown, "SESSION_DIR", tmp_path)
    test_eml_path = tmp_path / "test.eml"
    test_eml_path.write_text("Subject: Hello\nFrom: foo@bar.se\n\nBody text\n")

    eml_path_to_markdown_folder(to_path(str(test_eml_path)))

    chime_files = list((tmp_path / "chime").glob("*/chime.md"))
    assert len(chime_files) == 1
    assert chime_files[0].read_text(encoding="utf-8") == "# Hello\n\nBody text\n"

def test_eml_path_to_markdown_folder_skips_attachment_body(tmp_path, monkeypatch):
    monkeypatch.setattr(eml_to_markdown, "SESSION_DIR", tmp_path)
    raw = (
        "Subject: Multi\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/mixed; boundary="BOUNDARY"\r\n'
        "\r\n"
        "--BOUNDARY\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Body text\r\n"
        "--BOUNDARY\r\n"
        "Content-Type: text/plain\r\n"
        'Content-Disposition: attachment; filename="note.txt"\r\n'
        "\r\n"
        "Attachment text\r\n"
        "--BOUNDARY--\r\n"
    )
    test_eml_path = tmp_path / "test.eml"
    test_eml_path.write_text(raw)

    eml_path_to_markdown_folder(to_path(str(test_eml_path)))

    chime_files = list((tmp_path / "chime").glob("*/chime.md"))
    assert len(chime_files) == 1
    content = chime_files[0].read_text(encoding="utf-8")
    assert "Body text" in content
    assert "Attachment text" not in content

def test_eml_path_to_markdown_folder_subject_collision_raises(tmp_path, monkeypatch):
    monkeypatch.setattr(eml_to_markdown, "SESSION_DIR", tmp_path)

    first_eml_path = tmp_path / "first.eml"
    first_eml_path.write_text("Subject: Hello\nFrom: foo@bar.se\n\nBody text\n")
    eml_path_to_markdown_folder(to_path(str(first_eml_path)))

    second_eml_path = tmp_path / "second.eml"
    second_eml_path.write_text("Subject: Hello\nFrom: baz@qux.se\n\nOther body\n")
    with pytest.raises(ChimeVariantsNotSupportedError):
        eml_path_to_markdown_folder(to_path(str(second_eml_path)))

def test_parse_sample_eml():
    session_dir = Path(__file__).parent
    eml_paths = list(session_dir.glob("*.eml"))
    assert len(eml_paths) == 1, f"expected exactly one sample .eml, found {len(eml_paths)}"
    msg = to_email_msg(to_path(str(eml_paths[0])))
    assert msg["subject"]
    email_dict = parse_email_msg(msg)
    assert email_dict["subject"]
    assert email_dict["from"]
    assert email_dict["date"] is None or isinstance(email_dict["date"], datetime)
