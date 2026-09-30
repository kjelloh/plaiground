from html_to_markdown import html_to_markdown


def test_malformed_href_falls_back_to_plain_text():
    # Apple Mail's data detector can mis-parse plain text (e.g. a regex
    # snippet) into an href that urlsplit() rejects with "Invalid IPv6 URL".
    # There's no usable link target, so it should read as plain text, not
    # crash and not turn into a link of any form.
    html = '<html><body><p>See <a href="//[bad">//[bad</a> here.</p></body></html>'
    markdown, refs = html_to_markdown(html)
    assert markdown.strip() == "See //&#91;bad here."
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


def test_bracket_text_does_not_become_a_link():
    # A bare "[0-9]" in prose (e.g. a regex snippet) must not read as
    # CommonMark link syntax in the rendered output.
    html = '<html><body><p>&quot;\\d&quot; is short for [0-9]</p></body></html>'
    markdown, refs = html_to_markdown(html)
    assert "&#91;0-9&#93;" in markdown
    assert "[0-9]" not in markdown


def test_bracket_text_does_not_collide_with_latex_math_delimiters():
    # "\[" / "\]" is valid CommonMark escaping for a literal bracket, but
    # it's also the LaTeX display-math delimiter many renderers (e.g. VS
    # Code's Markdown preview) recognize — so backslash-escaping a bracket
    # right before an underscore can produce a broken math block instead of
    # plain text. Entity-escaping sidesteps this: no renderer treats "&#91;"
    # as the start of a math block.
    html = "<html><body><p>matches [A-Za-z0-9_]</p></body></html>"
    markdown, refs = html_to_markdown(html)
    assert "\\[" not in markdown and "\\]" not in markdown
    assert "&#91;A-Za-z0-9_&#93;" in markdown
