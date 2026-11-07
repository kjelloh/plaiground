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

Nu har jag itererat en mängd små ändringar och skaffat mig en intuitiv känsla av vad Claude är bra på och vad den nästan alltid missar.

* Den missar nästan alltid att föreslå en lösning som passar bra 'i det stora hela'.

    * Som när jag bad om att starta edit-mode då en tangent saknar mappning till option.
    * Då föreslog den att gå in i edit_mode om state saknar on_input.
    * Men den enkla lösningen är ju att trigga på att ingen option mapar till rangenten.
    * Då jag påpekade detta så nappade den direkt.
    * Så det är hela tiden jag som måste ha helikopter-perspektivet och arkitekturen i beaktande.
    * Claude är duktig på att 'slå upp' vilken kod som åstadkommer det resultat jag vill ha.
    * Claude är dålig på att välja kod som också 'passar ihop' med 'befintligt beteende' (ungefär)
    * Det är svårt att sätta ord på det. Men Claude 'förstår inte konsekvenserna'. Det måste jag håla koll på och styra efter. 

Jag gjorde ett försök till att med hjälp av Claude införa state för listor som skapas och numrerar i runtime.

* Men den kod som Claude skrev blev för tungrodd?
* Jag tror heller inte att den kommer att fungera för de behov vi har?

Jag tog en promenad och insåg att jag redan klöst detta problem tidigare.

* Så jag får nog be Claude återskapa den design jag redan provat och som fungerar.
* Nämligen att låta trädet av state växa via aktuellt state hela tiden.
* Aktuellt state vet vilka nästa state det stöder.
* Och aktuellt state vet hur nästa state kan skapas ur aktuellt state.
* Så TUI behöver fråga aktuellt state hur aktuella val skall visas för användaren.
* Och TUI skall fråga state hur det agerar på en tangent och returnera ett nytt state när så är aktuellt.
* TUI kan då fortfarande ha ett träd av de state som just nu ligger på stacken.

Låt oss se om Claude kan skapa koden som gör detta istället? Jag fråga Claude.

```text
Jag har nu insett att vi inte kan låta TUI ha ett statiskt träd av state från början. TUI behöver be aktuellt state skapa nästa state på tangent när så skall ske. Anledningen är att många state visar en lista av val som vi vet först när programmet körs. Vi har exempel som vilka projekt vi kan välja att öppna. Vilka financiella år vi kan öppna. Vilka verifikation som finns i bokföringen osv.  Jag tänker mig att TUI får fråga state hur listan med val skall se ut för användaren. Frågan till dig blir om detta är görligt i aktuell kod? Hur stor ändring är det? Hur styår sig mitt förslag på hur andrqa TUI navigerar state? Vilka för och nackdelar har denna lösning. Vad kan du föreslå?
```

NEJ. Det här fungerade inte heller.

* Jag fick en konstig lösning med factory i State?
* Men mitt förslag var ju att TUI frågar state efter nytt state på tangent.
* Men jag inser nu att jag var otydlig?
    * Jag skulle ha specat att on_key skall returnera nytt state eller nullptr?
    * Och att frågan efter vilka options som finns kan returnera en vektor av strängar?
* Jag chattade fram och tillbaka och plötsligt drog Claude igång att koda fast jag bett om att ge mig ett förslag först!

Så jag avbryt, använde git för att reversera och startade om IGEN!

Jag prövade en ny prompt.

```text
Nu skall jag vara extra tydlig eftersom det verkar som om du blir förvirrad av mina beskrivningar. Jag vill nu att vi gör om koden så att TUI bygger trädet av State dynamiskt. Jag vill att TUI exponerar en basklass State som en 'klient-del' kan ärva ifrån för sina konklreta states. TUI frågart aktuellt state efter en vector<string> att visa för användarern som listar vilka val statet själv definierar. När TUI skicakr en tangent till State får den tillbaka State* eller nullptr och uppdaterar sitt träd efter svaret. Här betyder nullptr att ingen mappning finns i state. TUI frågar då 'sig själv' om en generell mappning finns. PÅ detta sätt föreställer jag mig att vi kan bygga trädet av State dynamiskt allt eftersom användar navigera sig in på en väg. TUI äger fortfarande skapade states men destruerar dom då användaren lämnar det. Vad tror du? Kan du göra om koden till detta beteende? Vad föreslår du?
```

Idag är en sådan dag då jag inte tror AI-agenter för kodning är tillräckligt bra än?

* Jag har nu hamnat i ett läge då det är svårt att få Claude att skapa kod som jag tycker är tillräckligt bra?
* Jag undrar om det beror på att Claude läser denna session.md och min besvikelse smutsar ner Claud:s context-fönster?
* Något skaver i alla fall idag?
* Kanske borde jag vara mer konstruktiv i min kritik av de förslag Clade ger mig?
* Så kan jag hitta ett sätt så att Claude resonerar i ett 'bättre' spår?

Nu gick det vägen!

* Claude gav mig nu något som beter sig som förut och som på papperet borde vara det jag vill ha.

Vi märker det när vi försöker igen att skapa numrerade listor av element.

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