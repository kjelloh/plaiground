# Consider take 2 on eml-to-chime python mechanism?

## 20260929

This is take 2 on [Consider ways to transform a mail eml-file into a chime folder with chime.md and image files?](../71388c86/session.md)

* We go back to a pipe-line eml-to-html, html-to-markdown and then markdown-to-chime
* And we want the end result emls-to-chimes calling eml-to-chime that in turn does the 'chime' init and then html-to-chime into the chime?

I imagine we can clone the relevant scripts from previous session '../71388c86/' and refactor them into the mechanism we would like to have?

I vide coded the mechanism.

* eml_to_chime.py
* eml_to_html.py
* emls_to_chimes.py
* html_to_markdown.py
* test_eml_to_chime.py

Using a cloned 'init_new.py' to be able to apply the git repo root 'in it new artcicle' mechanism.





