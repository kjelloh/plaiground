# Överväg att vibe-koda cratchit bokslut på svenska från senaste cratchit-sessioner

```#0a37da4b```

## 20261106

Så var det dags att testa att vibe-koda med Claude på svenska.

Målet med denna session är att skapa en version av cratchit med kod från tidigare sessioner.

* Denna cratchit skall ha en ny bättre organisation av användargränssnittet.
* Användargränssnittet skall vara på svenska.
* Användargränssnittet skall ha stöd för bokslut.

Jag har tidigare sessioner med 'take 3' som är senaste bokförings-delen. Och sedan separat delar för pdf-läsning som text, Årsredovisning och Inkomstdeklaration 2.

1. [# Consider a cratchit app that maps an SIE file to a Swedish Årsredovisning?](../5b35b2b9/session.md)
1. [Consider a cratchit app that maps an SIE file to a Swedish SKV Ink_2?](../7d4475fb/session.md)
1. [Consider to vibe code a pdf-walker C++ console app?](../9fe36d28/session.md)
1. [Vibe code a Swedish book keeping app named cratchit - take 3 (Drop feed Claude)](../60cae9fe/session.md)

Jag vill vibe-koda med Claude.

* Jag vill att Claude genomför isolerade funktionsändringar med tydlig feedback efter varje moment.
* Jag vill själv testa och iterera tills jag väljer att 'committa' till Github.
* Jag vill fråga Claude hur den vill jobba för att komma ihåg vad vi beslutat och vad vi åstadkommit så långt?

Det verkar som om det första steget är att återskapa cratchit i ny form som kan öppna the lokala projektet '/Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/0a37da4b/The ITfied AB'?

* Skapa ett fräscht modernt C++ projekt som använder cmake och konsumerar från Conan package manager.
* Återskapar begränsat UI som bara kan navigera till att öppna det lokala projektet.
* INTE återskapa någon annan funktionalitet i detta steg!

Så har jag kommit igång med ett vibe-kodat tomt C++ project 'cratchit' ok.

* Vibe-kodat charset från walk_pdf (senaste versionen av text transcoding) ok.
* Vibe-kodat terminal_encoding från walk_pdf
* Vibe-kodat loggning fron 'take 3'
* Vibe-kodat dot-mapp i användarens hemmakatalog (Claude säger att det nu skall funka cross-platform?)
