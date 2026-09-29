# Consider a way to filter out all files that is reachable from a site with an index.md root document?

**805fef21**

## 20260929

So it turns out that the jekyll site generator engine in fact copies ALL files from the site source files.

* But I want to generate a site that contains only the REACHABLE files!
* I want to define the site by starting at the root document index.md
* Then I want a mechanism that walks this markdown files and registers all files reachable from index.md
    * This should be all markdown links (clickable refs and images)
    * Are there any other reachable files?
* I then imagine we can define a bsf that repeats this link-recollection from reachable files in a markdown file.

Suppose I implement this as a pythion script. What are my options to implement this in a terse and straight forward way?

I vibe coded [reachable.py](./reachable.py) that seems to work?