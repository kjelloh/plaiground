from html_to_markdown import html_to_markdown


def test_malformed_href_falls_back_to_plain_text():
    # Apple Mail's data detector can mis-parse plain text (e.g. a regex
    # snippet) into an href that urlsplit() rejects with "Invalid IPv6 URL".
    # There's no usable link target, so it should read as plain text, not
    # crash and not turn into a link of any form.
    html = '<html><body><p>See <a href="//[bad">//[bad</a> here.</p></body></html>'
    markdown, refs = html_to_markdown(html)
    assert markdown.strip() == "See //\\[bad here."
    assert refs == []


def test_unparsable_href_falls_back_to_plain_text():
    # Same data-detector mistake, but the href doesn't match the anchor's
    # visible text (it "ate" only part of the surrounding regex snippet) —
    # there's no usable link target here at all, just mis-tagged plain text.
    html = (
        '<html><body><p>“<a href="smb://[^//">X</a>Y” '
        "matches “Hello.exe”</p></body></html>"
    )
    markdown, refs = html_to_markdown(html)
    assert "smb://" not in markdown
    assert "[X]" not in markdown and "<smb" not in markdown
    assert "XY" in markdown
    assert refs == []


def test_external_url_with_matching_label_becomes_autolink():
    html = '<html><body><a href="https://example.com/x">https://example.com/x</a></body></html>'
    markdown, refs = html_to_markdown(html)
    assert markdown.strip() == "<https://example.com/x>"


def test_local_ref_with_matching_label_becomes_bracket_link():
    html = '<html><body><a href="file.pages">file.pages</a></body></html>'
    markdown, refs = html_to_markdown(html)
    assert markdown.strip() == "[file.pages](file.pages)"
    assert refs == ["file.pages"]
