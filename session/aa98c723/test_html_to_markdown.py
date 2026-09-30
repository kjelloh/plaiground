from html_to_markdown import html_to_markdown


def test_malformed_href_does_not_crash():
    # Apple Mail's data detector can mis-parse plain text (e.g. a regex
    # snippet) into an href that urlsplit() rejects with "Invalid IPv6 URL".
    html = '<html><body><p>See <a href="//[bad">//[bad</a> here.</p></body></html>'
    markdown, refs = html_to_markdown(html)
    assert "[//\\[bad](//[bad)" in markdown


def test_external_url_with_matching_label_becomes_autolink():
    html = '<html><body><a href="https://example.com/x">https://example.com/x</a></body></html>'
    markdown, refs = html_to_markdown(html)
    assert markdown.strip() == "<https://example.com/x>"


def test_local_ref_with_matching_label_becomes_bracket_link():
    html = '<html><body><a href="file.pages">file.pages</a></body></html>'
    markdown, refs = html_to_markdown(html)
    assert markdown.strip() == "[file.pages](file.pages)"
    assert refs == ["file.pages"]
