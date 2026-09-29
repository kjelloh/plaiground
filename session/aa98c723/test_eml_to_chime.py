from pathlib import Path

import pytest

import eml_to_chime
from eml_to_chime import ChimeAlreadyExistsError, eml_file_to_chime

SIMPLE_HTML_EML = (
    "Subject: Hello\r\n"
    "From: foo@bar.se\r\n"
    "Date: Fri, 11 Sep 2026 12:00:00 +0000\r\n"
    'Content-Type: text/html; charset="utf-8"\r\n'
    "\r\n"
    "<html><body><p>Body <strong>text</strong></p></body></html>\r\n"
)

SIMPLE_PLAIN_EML = (
    "Subject: Plain\r\n"
    "From: foo@bar.se\r\n"
    "Date: Fri, 11 Sep 2026 12:00:00 +0000\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "\r\n"
    "First paragraph.\r\n"
    "\r\n"
    "Second paragraph,\r\n"
    "with a line break.\r\n"
)


def write_eml(tmp_path: Path, name: str, content: str) -> Path:
    eml_path = tmp_path / name
    eml_path.write_text(content, encoding="utf-8")
    return eml_path


def test_eml_file_to_chime_creates_chime(tmp_path, monkeypatch):
    monkeypatch.setattr(eml_to_chime, "SESSION_DIR", tmp_path)
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    assert chime_path == tmp_path / "chime" / chime_path.parent.name / "chime.md"
    content = chime_path.read_text(encoding="utf-8")
    assert content.startswith("# test\n\n")
    assert "2026-09-11T12:00:00+00:00" in content
    assert "Body **text**" in content


def test_eml_file_to_chime_cleans_up_scratch_dir(tmp_path, monkeypatch):
    monkeypatch.setattr(eml_to_chime, "SESSION_DIR", tmp_path)
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    assert not (chime_path.parent / "_html_scratch").exists()
    assert not (chime_path.parent / "test.md").exists()


def test_eml_file_to_chime_duplicate_raises(tmp_path, monkeypatch):
    monkeypatch.setattr(eml_to_chime, "SESSION_DIR", tmp_path)
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    eml_file_to_chime(eml_path, base_dir=tmp_path)

    with pytest.raises(ChimeAlreadyExistsError):
        eml_file_to_chime(eml_path, base_dir=tmp_path)


def test_eml_file_to_chime_copies_inline_image(tmp_path, monkeypatch):
    monkeypatch.setattr(eml_to_chime, "SESSION_DIR", tmp_path)
    raw = (
        "Subject: Pic\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/related; boundary="B"\r\n'
        "\r\n"
        "--B\r\n"
        'Content-Type: text/html; charset="utf-8"\r\n'
        "\r\n"
        '<html><body><img src="cid:img1" alt="a pic"></body></html>\r\n'
        "--B\r\n"
        "Content-Type: image/png\r\n"
        "Content-ID: <img1>\r\n"
        "Content-Disposition: inline; filename=pic.png\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "iVBORw0KGgo=\r\n"
        "--B--\r\n"
    )
    eml_path = write_eml(tmp_path, "pic.eml", raw)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    assert (chime_path.parent / "pic.png").is_file()
    assert "![a pic](pic.png)" in chime_path.read_text(encoding="utf-8")


def test_eml_file_to_chime_plain_text_fallback(tmp_path, monkeypatch):
    monkeypatch.setattr(eml_to_chime, "SESSION_DIR", tmp_path)
    eml_path = write_eml(tmp_path, "plain.eml", SIMPLE_PLAIN_EML)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    content = chime_path.read_text(encoding="utf-8")
    assert "First paragraph." in content
    assert "Second paragraph," in content
    assert "with a line break." in content


def test_parse_sample_eml():
    session_dir = Path(__file__).parent
    eml_paths = list(session_dir.glob("*.eml"))
    assert eml_paths, "expected at least one sample .eml in the session folder"
