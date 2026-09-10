# Consider ways to transform a mail eml-file into a chime folder with chime.md and image files?

## 20260910

So I now have an email in an eml-file 'Todo_ Consider to document the networks accessed by an ODB2 connecting device?.eml´.

I want to use a python script to transform this email into a markdown file "Todo_ Consider to document the networks accessed by an ODB2 connecting device?.md" plus all images it contains as local image files.

I also want the markdon file to correctly link (embed) the images as they are arranged in the mail.

I winder what option I have for this?

* Is the eml-file format known and avaiable for parsing in a python sccipt?
* Is the eml-file 'complete' in that it encodes the mail images?
  * That is, are images embedded as e.g., Base32 encoding in the eml-file?
  * Or are the images missing as in 'still on the server'?

I now have the script [eml_to_html.py](./eml_to_html.py) that claude code created for me.

* It seems to work on my example eml-file 'Todo_ Consider to document the networks accessed by an ODB2 connecting device?.eml' ok.
* But it does not provide any header for the html.

I now would like the script to generate a header 1 as the subject of the mail.