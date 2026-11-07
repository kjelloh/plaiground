# Överväg att vibe-koda cratchit bokslut på svenska från senaste cratchit-sessioner

```#0a37da4b```

## 20261107

Idag började jag med att försöka plantera lite mer 'arkitektur' så vi får en mer 'snitsld bana' då vi tar in kod från tidigare sessioner.

* Jag bad Claude föreslå och implementera an 'Transcoder' för text-kodning.
* Det såg bra ut på papperet.
* MEN: Då jag hav OK så skapade Claude en typ som returnerade en struct!

    * I mitt huvud borde det gå att para ihop en Decoder med en Encoder till en Transcode?
    * Och bara ha en push av bytes i 'encoder'ändan'.
    * Och få den att producera nullopt eller ut-bytes från Encoder-delen?
    * Varför krånglade Claude till det?

Jag backade och gjorde omtag.

* Jag prövade en ny prompt baserad på att första försöket misslyckades.

```text
 Jag vill att vi gör ett nytt försök att skapa en 'Transcoder' som parar ihop en Decoder med en Encoder och producerar push-to-endoded-bytes i ett steg. Vi prövar att designa en dynamisk 'Trasncoder' som tillverkas run-time av 'make_transcoder()' och returnerar en 'Transcoder'. Den borde kunna vata en template av 'Decoder' och 'Encoder' då de alla har stöd för de anrop en Transcoder behöver? Jag spekulerar, Transcoder::push annropar Decoder::push. Om en Unicode code point tillvertkas så annropar den Encoder::push och får bytes att returnera. Det känns som om Transcoder::push returnerar en optional 'bytes' med ut-representationen? Håller du med? Skulle detta fungera? Kan du göra det? Coh hur ser till förslag till lösning ut?
```

* Och nu ser det ut som om Claude 'förstår' bättre vad jag siktar på?

Jag har nu en Transcoder.

* Den har push i ena ändan.
* Och producerar nullopt eller 'bytes' så som de alstras av Encoder för varje Unicode code point.
* Parsningsfel till Unicode producerar Unicodes 'mysslyckad tolkning tecken' ( U+FFFD, Srr ut som '?')
* Parsningefel (Decoder) och misslyckad mappning (Encoder) räknas nu separat.
* NOTERA: Parsningefel till  U+FFFD till 256-bytes-encoding som CP437 räknas två gånger (parsningsfel + ingen mappning)

    * Vi kan senare frestas att skapa en representation typ ```<U+FFFD>```.
    * Men vi behöver tänka igenom detta då sådan representation beror på vilket teckenset vi kodar till (e.g., hur gör vi på ryska, grekiska, kinesiska?)

Ser bra ut för nu tror jag?!

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

Nu haf jag blivit lockad att aktivera 'Github Actions' för att bygga för macOS, Linux och Windows uppe på Github.

* Frågan är om detta är en 'bra' ide?
* Hur mycket jobb har jag nu att hela tiden hålla dessa byggen fungerande?
* Å andra sidan, ganska coolt om jag på detta sätt kan hålla cratchit byggbar och testad på alla tre plattformar? 