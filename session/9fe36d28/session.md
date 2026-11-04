# Consider to vibe code a pdf-walker C++ console app?

```#9fe36d28```

## 20261104

I find myself needing to peek into pdf:s from tools like my book keeping app cratchit.

Now in the age of AI-tooling I wonder i I can design a small C++ code library and a console app to 'walk' pdf-files.

* I want the walker to 'walk' the structure of the pdf with a 'path' and 'content'.
* A client can use the walker to inspect the pdf-file as a composite structure.
* The 'path' shall be sufficient for the client to be able to determine what part of the pdf file structure is currently provided.
* The 'content' shall be sufficient for the client to process it in some form.
* The main focus of tge walker is to walk the pdf text content.
* The walker shall present the pdf text structure  'path' and 'content' so that a client shall be able to produce a raw text representation of the pdf.
* If possible the walker shall present the text structure of a pdf with 'path' and 'content' so that a client can process it into a markdown representation.

I want the C++ code and console app to be self-contained in modern C++.

* Cross platform with cmake build.
* Consume packages from conan package manager (where appropriate)

As a test I want the walker to work on my purchased e-books @/Users/kjell-olovhogdahl/Resilio Sync/Dropbox/The ITfied/Köpta Digitala Media/BAS.

* svensk_redovisning_bokforing.pdf
* svensk_redovisning_bokslutsb.pdf

Also as a text I want the walker to work on my book keeping pdf vouchers @/Users/kjell-olovhogdahl/Downloads/ITfied/Verifikationer and @
