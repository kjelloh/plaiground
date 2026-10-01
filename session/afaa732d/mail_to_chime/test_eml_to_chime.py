import shutil
from pathlib import Path

import pytest

import eml_to_chime
import eml_to_html
from eml_to_chime import ChimeAlreadyExistsError, eml_file_to_chime

SITE_REPO_INIT_NEW = Path(__file__).parent.parent / "site_repo" / "init_new.py"


@pytest.fixture(autouse=True)
def init_new_in_tmp(tmp_path):
    """eml_file_to_chime loads init_new.py from base_dir (the target
    repo) — give tmp_path a copy of the real one, like any repo this
    mechanism is pointed at would already have."""
    shutil.copy2(SITE_REPO_INIT_NEW, tmp_path / "init_new.py")

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


def test_eml_file_to_chime_creates_chime(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    assert chime_path == tmp_path / "chime" / chime_path.parent.name / "chime.md"
    content = chime_path.read_text(encoding="utf-8")
    assert content.startswith("# test\n\n")
    assert "2026-09-11T12:00:00+00:00" in content
    assert "Body **text**" in content


def test_eml_file_to_chime_cleans_up_scratch_dir(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    assert not (chime_path.parent / "_html_scratch").exists()
    assert not (chime_path.parent / "test.md").exists()


def test_eml_file_to_chime_duplicate_raises(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    eml_file_to_chime(eml_path, base_dir=tmp_path)

    with pytest.raises(ChimeAlreadyExistsError):
        eml_file_to_chime(eml_path, base_dir=tmp_path)


def test_eml_file_to_chime_copies_inline_image(tmp_path):
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


def test_eml_file_to_chime_plain_text_fallback(tmp_path):
    eml_path = write_eml(tmp_path, "plain.eml", SIMPLE_PLAIN_EML)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    content = chime_path.read_text(encoding="utf-8")
    assert "First paragraph." in content
    assert "Second paragraph," in content
    assert "with a line break." in content


def test_eml_file_to_chime_images_only_fallback(tmp_path):
    raw = (
        "Subject: Photo\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/mixed; boundary="B"\r\n'
        "\r\n"
        "--B\r\n"
        "Content-Type: image/png\r\n"
        "Content-Disposition: inline; filename=photo.png\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "iVBORw0KGgo=\r\n"
        "--B--\r\n"
    )
    eml_path = write_eml(tmp_path, "photo.eml", raw)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    assert (chime_path.parent / "photo.png").is_file()
    assert "![](photo.png)" in chime_path.read_text(encoding="utf-8")


def test_eml_file_to_chime_document_only_fallback(tmp_path):
    raw = (
        "Subject: Document only\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/mixed; boundary="B"\r\n'
        "\r\n"
        "--B\r\n"
        "Content-Type: application/x-iwork-pages-sffpages\r\n"
        "Content-Disposition: attachment; filename=Plans.pages\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "AAAA\r\n"
        "--B--\r\n"
    )
    eml_path = write_eml(tmp_path, "doc.eml", raw)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    content = chime_path.read_text(encoding="utf-8")
    assert (chime_path.parent / "Plans.pages").is_file()
    assert "## Attachments" in content
    assert "[Plans.pages](Plans.pages)" in content


def test_eml_file_to_chime_document_attachment_alongside_text(tmp_path):
    raw = (
        "Subject: With attachment\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/mixed; boundary="B"\r\n'
        "\r\n"
        "--B\r\n"
        'Content-Type: text/html; charset="utf-8"\r\n'
        "\r\n"
        "<html><body><p>See attached.</p></body></html>\r\n"
        "--B\r\n"
        "Content-Type: application/x-iwork-pages-sffpages\r\n"
        "Content-Disposition: attachment; filename=Plans.pages\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "AAAA\r\n"
        "--B--\r\n"
    )
    eml_path = write_eml(tmp_path, "withdoc.eml", raw)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    content = chime_path.read_text(encoding="utf-8")
    assert (chime_path.parent / "Plans.pages").is_file()
    assert "See attached." in content
    assert "## Attachments" in content
    assert "[Plans.pages](Plans.pages)" in content


def test_eml_file_to_chime_wraps_body_in_liquid_raw(tmp_path):
    """Mail content with Liquid-looking "{{" / "{%" (e.g. C++ brace-init)
    must not break the Jekyll build — see session.md 20261001 for the real
    4433-mail run that hit this: some shapes abort the entire site build,
    not just render oddly."""
    raw = (
        "Subject: Braces\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: text/plain; charset="utf-8"\r\n'
        "\r\n"
        'std::array<std::array<int,3>,3> a{{{{-1,-1,-1}},{{-1,-1,-1}}}};\r\n'
    )
    eml_path = write_eml(tmp_path, "braces.eml", raw)

    chime_path = eml_file_to_chime(eml_path, base_dir=tmp_path)

    content = chime_path.read_text(encoding="utf-8")
    assert "{% raw %}" in content
    assert "{% endraw %}" in content
    assert "a{{{{-1,-1,-1}},{{-1,-1,-1}}}};" in content
    raw_start = content.index("{% raw %}")
    endraw_start = content.index("{% endraw %}")
    body_start = content.index("a{{{{-1,-1,-1}}")
    assert raw_start < body_start < endraw_start


def test_eml_file_to_chime_no_content_raises(tmp_path):
    raw = (
        "Subject: Nothing\r\n"
        "From: foo@bar.se\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "AAAA\r\n"
    )
    eml_path = write_eml(tmp_path, "empty.eml", raw)

    with pytest.raises(eml_to_html.NoRenderablePartError):
        eml_file_to_chime(eml_path, base_dir=tmp_path)


def test_parse_sample_eml():
    eml_dir = Path(__file__).parent.parent / "example_eml"
    eml_paths = list(eml_dir.glob("*.eml"))
    assert eml_paths, f"expected at least one sample .eml in {eml_dir}"
