# plaiground thinking

I find that thinking by writing helps me focus on my goals and what may be done to get there.

* [sessions](../session/index.md)
* [chimes](../chime/index.md)

## 20260912

I now think it is time to try a batch run on my almost 4500 Todo-mails with the current eml_to_markdown-script?

* I want the script to wine about what it fails to parse in the eml-file (e-mail).
  * That is, I currently fail on mail parts with unsupported conet 

## 20260911

I dont like to transform mail eml-file to markdown over html.

maybe I can go for a python script that goes from a mail eml-file directly to markdown?

I continued thinking in [Consider ways to transform a mail eml-file into a chime folder with chime.md and image files?](../session/71388c86/session.md)

## 20260910

So I created this 'plaiground' git repo and project to have a home for my AI interactions and to keep AI sandboxed from any production code.

* I can have the AI get at code here and decide later if and how to integrate it intp production code.
* I can freely experiment (play around with) different harnesses and LLM:s.
* I can version control and document my progress and result with LLM:s, harnesses and AI related tooling for later reference and re-use.

So let's see how this plays out?

I now created [Consider ways to transform a mail eml-file into a chime folder with chime.md and image files?](../session/71388c86/session.md)

May aim was to sandbox claude to a 'session' folder.

Ok, Claude does its thing allright.

* But I feel out-of-touch.
* I need to remember to now refactor all the code it created to my taste and understanding!

At this stage I seem to have two scripts that at leats works on the example emf-file.

* eml_to_httml.py
  * It takes an eml-file as argument
  * It parses the eml-file and extracts all images to png-files
  * It transforms the mail body into a static html page (file)
  * The html-file refers to local png-images.
* html_to_markdown.py
  * It takes a path to an html-folder as argument
  * It creates a folder 'markdown' paralell to the html folder
  * It transfers all 'non html-files' to the markdon folder
  * It then transforms 'in code' the html to markdown as-is

This seems to work in the current state.

* But I have to tests.
* And I have no batch mechanism

So how do I iterate this with my aim to sanbox claude?

* Should I create a new session and seed it with the result from previous session?
* Or can I continue in the same session a bt further?
  * E.g., have claude implement tests for the two scripts?