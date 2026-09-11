import email
from datetime import datetime, timezone
from email import policy
from pathlib import Path

import pytest

from eml_to_markdown import (
    EmailDefectsError,
    EmailEmptyError,
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
