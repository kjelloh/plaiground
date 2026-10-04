import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

import eml_to_html
from eml_to_domain import EntrySupersededError, eml_file_to_entry, read_entry_date

SITE_REPO_INIT_NEW = Path(__file__).parent.parent / "site_repo" / "init_new.py"


@pytest.fixture(autouse=True)
def init_new_in_tmp(tmp_path):
    """eml_file_to_entry loads init_new.py from base_dir (the target
    repo) — give tmp_path a copy of the real one, like any repo this
    mechanism is pointed at would already have."""
    shutil.copy2(SITE_REPO_INIT_NEW, tmp_path / "init_new.py")


@pytest.fixture(params=["mail", "note"])
def domain(request):
    """Every test runs against two domains: nothing may assume a fixed one."""
    return request.param

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


def test_eml_file_to_entry_creates_entry(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert entry_path == tmp_path / domain / entry_path.parent.name / f"{domain}.md"
    content = entry_path.read_text(encoding="utf-8")
    assert content.startswith("# Hello\n\n")  # from the mail's Subject, not the filename
    assert content.startswith("# Hello\n\n2026-09-11T12:00:00+00:00\n\n")
    assert entry_path.parent.name == hashlib.md5(b"Hello").hexdigest()[:8]
    assert "Body **text**" in content


def test_eml_file_to_entry_cleans_up_scratch_dir(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert not (entry_path.parent / "_html_scratch").exists()
    assert not (entry_path.parent / "test.md").exists()


def test_eml_file_to_entry_reprocessing_same_mail_is_superseded(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    with pytest.raises(EntrySupersededError):
        eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)


def test_eml_file_to_entry_keys_by_subject_not_filename(tmp_path, domain):
    # Apple Mail's exporter appends " 2", " 3", ... to the *filename* when
    # several exported mails share a subject — the real Subject: header is
    # identical across them, and that's what must key the entry.
    older = write_eml(tmp_path, "todo.eml", SIMPLE_HTML_EML)
    newer_raw = SIMPLE_HTML_EML.replace(
        "Date: Fri, 11 Sep 2026 12:00:00 +0000",
        "Date: Sat, 12 Sep 2026 09:00:00 +0000",
    ).replace("Body <strong>text</strong>", "Updated <strong>text</strong>")
    newer = write_eml(tmp_path, "todo 2.eml", newer_raw)

    older_entry = eml_file_to_entry(older, base_dir=tmp_path, domain=domain)
    newer_entry = eml_file_to_entry(newer, base_dir=tmp_path, domain=domain)

    assert older_entry == newer_entry  # same subject -> same entry folder
    content = newer_entry.read_text(encoding="utf-8")
    assert "Updated **text**" in content
    assert "Body **text**" not in content  # old revision's content is gone


def test_eml_file_to_entry_older_revision_is_superseded_and_leaves_content_untouched(
    tmp_path, domain
):
    newer = write_eml(
        tmp_path,
        "todo.eml",
        SIMPLE_HTML_EML.replace(
            "Date: Fri, 11 Sep 2026 12:00:00 +0000",
            "Date: Sat, 12 Sep 2026 09:00:00 +0000",
        ),
    )
    older_raw = SIMPLE_HTML_EML.replace(
        "Body <strong>text</strong>", "Stale <strong>text</strong>"
    )
    older = write_eml(tmp_path, "todo 2.eml", older_raw)

    entry_path = eml_file_to_entry(newer, base_dir=tmp_path, domain=domain)

    with pytest.raises(EntrySupersededError):
        eml_file_to_entry(older, base_dir=tmp_path, domain=domain)

    content = entry_path.read_text(encoding="utf-8")
    assert "Body **text**" in content
    assert "Stale **text**" not in content


def test_eml_file_to_entry_copies_inline_image(tmp_path, domain):
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

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert (entry_path.parent / "pic.png").is_file()
    assert "![a pic](pic.png)" in entry_path.read_text(encoding="utf-8")


def test_eml_file_to_entry_plain_text_fallback(tmp_path, domain):
    eml_path = write_eml(tmp_path, "plain.eml", SIMPLE_PLAIN_EML)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    content = entry_path.read_text(encoding="utf-8")
    assert "First paragraph." in content
    assert "Second paragraph," in content
    assert "with a line break." in content


def test_eml_file_to_entry_images_only_fallback(tmp_path, domain):
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

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert (entry_path.parent / "photo.png").is_file()
    assert "![](photo.png)" in entry_path.read_text(encoding="utf-8")


def test_eml_file_to_entry_document_only_fallback(tmp_path, domain):
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

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    content = entry_path.read_text(encoding="utf-8")
    assert (entry_path.parent / "Plans.pages").is_file()
    assert "## Attachments" in content
    assert "[Plans.pages](Plans.pages)" in content


def test_eml_file_to_entry_document_attachment_alongside_text(tmp_path, domain):
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

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    content = entry_path.read_text(encoding="utf-8")
    assert (entry_path.parent / "Plans.pages").is_file()
    assert "See attached." in content
    assert "## Attachments" in content
    assert "[Plans.pages](Plans.pages)" in content


def test_eml_file_to_entry_wraps_body_in_liquid_raw(tmp_path, domain):
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

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    content = entry_path.read_text(encoding="utf-8")
    assert "{% raw %}" in content
    assert "{% endraw %}" in content
    assert "a{{{{-1,-1,-1}},{{-1,-1,-1}}}};" in content
    raw_start = content.index("{% raw %}")
    endraw_start = content.index("{% endraw %}")
    body_start = content.index("a{{{{-1,-1,-1}}")
    assert raw_start < body_start < endraw_start


def test_eml_file_to_entry_writes_and_links_plain_text(tmp_path, domain):
    eml_path = write_eml(tmp_path, "plain.eml", SIMPLE_PLAIN_EML)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    txt_path = entry_path.parent / f"{domain}.txt"
    assert txt_path.read_bytes().startswith(b"\xef\xbb\xbf")  # UTF-8 BOM, for browsers
    assert txt_path.read_text(encoding="utf-8-sig") == (
        "First paragraph.\n\nSecond paragraph,\nwith a line break.\n"
    )
    content = entry_path.read_text(encoding="utf-8")
    assert content.startswith(
        "# Plain\n\n2026-09-11T12:00:00+00:00\n\n"
        f"[Plain text]({domain}.txt)\n\n"
        "{% raw %}\n"
    )


def test_eml_file_to_entry_no_plain_text_part_writes_no_txt(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert not (entry_path.parent / f"{domain}.txt").exists()
    assert f"{domain}.txt" not in entry_path.read_text(encoding="utf-8")


def test_eml_file_to_entry_ignores_attached_txt_file(tmp_path, domain):
    raw = (
        "Subject: Attached txt\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/mixed; boundary="B"\r\n'
        "\r\n"
        "--B\r\n"
        'Content-Type: text/html; charset="utf-8"\r\n'
        "\r\n"
        "<html><body><p>See attached.</p></body></html>\r\n"
        "--B\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Disposition: attachment; filename=notes.txt\r\n"
        "\r\n"
        "Not the body.\r\n"
        "--B--\r\n"
    )
    eml_path = write_eml(tmp_path, "att.eml", raw)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert not (entry_path.parent / f"{domain}.txt").exists()


def test_eml_file_to_entry_newer_revision_replaces_plain_text(tmp_path, domain):
    older = write_eml(tmp_path, "plain.eml", SIMPLE_PLAIN_EML)
    newer_raw = SIMPLE_PLAIN_EML.replace(
        "Date: Fri, 11 Sep 2026 12:00:00 +0000",
        "Date: Sat, 12 Sep 2026 09:00:00 +0000",
    ).replace("First paragraph.", "Updated paragraph.")
    newer = write_eml(tmp_path, "plain 2.eml", newer_raw)

    eml_file_to_entry(older, base_dir=tmp_path, domain=domain)
    entry_path = eml_file_to_entry(newer, base_dir=tmp_path, domain=domain)

    text = (entry_path.parent / f"{domain}.txt").read_text(encoding="utf-8-sig")
    assert "Updated paragraph." in text
    assert "First paragraph." not in text


def test_eml_file_to_entry_no_content_raises(tmp_path, domain):
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
        eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)


# The sample emls stay in the session they were added in (read-only here).
SAMPLE_EML_DIR = Path(__file__).parent.parent.parent / "afaa732d" / "example_eml"


def test_parse_sample_eml():
    eml_dir = SAMPLE_EML_DIR
    eml_paths = list(eml_dir.glob("*.eml"))
    assert eml_paths, f"expected at least one sample .eml in {eml_dir}"


SITE_REPO_UPDATE_INDEX = SITE_REPO_INIT_NEW.parent / "update_index.py"
EML_TO_DOMAIN = Path(__file__).parent / "eml_to_domain.py"


def run_cli(*args, cwd):
    return subprocess.run(
        [sys.executable, str(EML_TO_DOMAIN), *map(str, args)],
        cwd=cwd,
        capture_output=True,
        text=True,
    )


def test_same_mail_into_two_domains_gives_same_hash_folder(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_PLAIN_EML)

    mail = eml_file_to_entry(eml_path, base_dir=tmp_path, domain="mail")
    note = eml_file_to_entry(eml_path, base_dir=tmp_path, domain="note")

    assert mail == tmp_path / "mail" / mail.parent.name / "mail.md"
    assert note == tmp_path / "note" / mail.parent.name / "note.md"
    assert (mail.parent / "mail.txt").is_file()
    assert (note.parent / "note.txt").is_file()


def test_invalid_domain_writes_nothing(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    with pytest.raises(ValueError):
        eml_file_to_entry(eml_path, base_dir=tmp_path, domain="not a domain")

    assert not [p for p in tmp_path.iterdir() if p.is_dir() and p.name != "__pycache__"]


def test_read_entry_date_reads_first_line_after_heading(tmp_path):
    md = tmp_path / "x.md"
    md.write_text("# X\n\n2026-09-11T12:00:00+00:00\n\nbody\n", encoding="utf-8")

    assert read_entry_date(md).isoformat() == "2026-09-11T12:00:00+00:00"


def test_read_entry_date_none_for_no_date_fallback(tmp_path):
    md = tmp_path / "x.md"
    md.write_text("# X\n\n(no date)\n", encoding="utf-8")

    assert read_entry_date(md) is None


def test_cli_requires_domain(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    result = run_cli(eml_path, "-o", tmp_path, cwd=tmp_path)

    assert result.returncode != 0
    assert "--domain" in result.stderr
    assert not [p for p in tmp_path.iterdir() if p.is_dir()]


def test_cli_rejects_invalid_domain(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    result = run_cli(eml_path, "--domain", "a/b", "-o", tmp_path, cwd=tmp_path)

    assert result.returncode != 0
    assert "invalid domain" in result.stderr


def test_cli_creates_entry_and_domain_index(tmp_path, domain):
    shutil.copy2(SITE_REPO_UPDATE_INDEX, tmp_path / "update_index.py")
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    result = run_cli(eml_path, "--domain", domain, "-o", tmp_path, cwd=tmp_path)

    assert result.returncode == 0, result.stderr
    short_hash = hashlib.md5(b"Hello").hexdigest()[:8]
    assert (tmp_path / domain / short_hash / f"{domain}.md").is_file()
    index = (tmp_path / domain / "index.md").read_text(encoding="utf-8")
    assert index == f"* [Hello]({short_hash}/{domain}.md)\n"


def test_cli_second_run_is_superseded(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)
    assert run_cli(eml_path, "--domain", domain, "-o", tmp_path, cwd=tmp_path).returncode == 0

    result = run_cli(eml_path, "--domain", domain, "-o", tmp_path, cwd=tmp_path)

    assert result.returncode != 0
    assert "superseded" in result.stderr
