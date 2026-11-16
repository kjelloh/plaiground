# Överväg att knyta ihop säcken och skapa ett produktionsredo cratchit bokföringsprogram från framgångsrika sessioner?

```#029b12e4```

## Common

* Bygg med: cmake --build cratchit/build/Debug -j
* Installera med: sudo cmake --install cratchit/build/Debug

## 20261116

Nu är det väl ändå dags att till slut få till ett sätt att redigera och skapa verifikationer till bokföringen?

* Jag har hittills av någon anledning inte gjort en bra sådan!
* Varför? Jag vet inte! Det är som om problemet flyr mig hela tiden! 
* Eller så är det så att jag inte gillar hur dubbel bokföring på BAS-konton gör det så ogenomskinligt och krångligt?

Men så, hur tacklar vi detta problem. Kanske kan vi börja med vad jag tror jag 'vet'?

* En verifikation skall bokföra en affärshändelse.
* Den har en rubrik, ett datum och ett belopp.
* Beloppet skall fördelas på konton och motkonton till lika delar debit som kredit.
* I Sverige används nästan alltid BAS-kontoplanen.
* Så beloppet konteras på olika BAS-konton.
* En 'rad' konterar ett delbelopp på ett BAS-konto med en frivillig text.
* Summan av alla konteringar skall vara noll (lika mycket debit som kredit)
* BAS-kontoplanen definierar regler för vilka konton som skall användas för olika affärshändelser (finansiella händelser?).

Till detta kommer bokföringslagen som reglerar vad man måste och eller får göra i och med sin bokföring.

Jag tror jag vet vad som räcker till att börja med?

* En händelse som är bokförd får i princip inte ändras.
* Men det verkar som om man får stryka rader.
    * Men en strykt rad måste stå kvar i verifikationen för spårbarhet.
    * SIE-filformatet har stöd för detta (Se tagg BTRANS)
* Det verkar också som om man får lägga till rader.
    * Men man bör (måste?) markera att raden har lagts till (inte var där i den ursprungliga bokförda verifikationen)?
    * SIE-filformatet har stöd för det också (Se tagg RTRANS)
* Dock verkar SIE-filformatet inte kräva att strukna och tillagda rader kan tas emot av ett program som läser in SIE-filer?
    * Se officiella [SIE_filformat_ver_4C_2025-08-06](https://sie.se/wp-content/uploads/2026/02/SIE_filformat_ver_4C_2025-08-06.pdf)

    ```text
    Hanteras inte poster av typen #BTRANS och #RTRANS skall dessa rader ignoreras vid
    import av en SIE-fil.
    ```
    * Så det verkar som om en strykt rad BARA står som en BTRANS (ursprungliga TRANS är ersatt av en BTRANS?)
    * Då blir det OK för mottagaren att ignorera BTRANS (Den är ju 'borttagen')
    * PÅ samma sätt måste en tillagd rad BÅDE markeras med en rad RTRANS OCH en vanlig TRANS!
    * PÅ så sätt kan mottagaren ignorera RTRANS och ändå läsa in vanlig TRANS och få med rätt konteringar.

OK, så här tänker jag nu för Cratchit.

* Vi har delat upp bokföringen i 'Bokförda' och 'Utkast'
* En editor för bokförda verifikationer skall begränsa vad användaren kan göra
    * Man skall kunna stryka och lägga till rader
    * Frågan är om man skall kunna ändra i rubriken?
    * Frågan är också om man skall kunna ändra datum?
    * I Fortnox-programmet får man det (Så då kanske det är enklast om vi tillåter det också?)
    * Ett enkelt UI kan vara ett första val mellan 'sammanfattning' och 'konteringar'?
    * Och alternativen för 'sammanfattning' är 'rubrik','belopp' och 'datum'?
    * Och alternativen för konteringar är en numrerad lista 0..9 av konteringar?
    * Alternativen för en vald kontering kan vara numrerat 'konto', 'belopp', 'beskrivning'
    * Men så skall kontering också kunna strykas, vilket kanske kan ske med en dold bokstavs-val 'b' för 'borttagen'?
    * En grundregel för editorn kan vara att 'historiken måste bevaras'?
    * Bör man kunna välja att göra om en vald verifikation till utkast?
    * Det krånglar ju till det lite då endast den sista i en serie kan göras om till utkast?
    * Men det känns samtidigt naturligt att ha detta val för den verifikation man valt?
* Jag tänker mig att en bokförd verifikation skall kunna flyttas till 'utkast'
    * Endast den senaste i en serie skall gå att flytta till utkast
    * En verifikation skall behålla sin serie när den blir utkast
    * Frågan är om valet att flytta skall vara under 'bokförda' eller 'utkast'?
    * Under bokförda kanske man kan välja 'Flytta till Utkast' och sedan välja från vilken serie man vill flytta från (underförstått att man ändå bara kan välja den sista i en serie)?
    * Eller så finns ett val under 'Utkast' (men jag kommer inte på vad ett sådant val skulle kallas? Funkar 'Utkast från bokfört'?)
    * I så fall blir undervalet nog vilken serie (underförstått att man bara kan göra om sista bokförda verifikationen i en serie till utkast?)
    * Även här tycker jag ett utkast från tidigare bokförd verifikation behåller sin serie.
    * Men att man i utkastet kan ange vilken serie den skall bokföras på?
* Men vi kan tillåta editering av Utkast mycket friare till dess de blir bokförda.
    * Man skall INTE kunna ändra på konteringar som kommer från kontoutdrag.
    * Man skall INTE kunna ändra på sammanfattningar som kommer från underlag (inlästa dokument som beskriver affärshändelser, alltså elektroniska verifikat)
    * Man skall kunna slå samman utkast till ett utkast med de sammanslagna utkastens konteringar (för att få ett utkast som beskriver 'samma' händelse, till exempel ett kvitto och en kontering från ett kontoutdrag)
    * En editor för ett utkast skall lika editorn för en bokförd verifikation så långt det är möjligt (bara fler element som går att ändra)

OK, jag börjar förstå varför det tagit mig sådan tid att designa en editor för verifikationer. Det är KOMPLEXT!

Jag tror jag redan bestämt mig för ett flöde baserat på underlag och konteringar som sätts samman till utkast som blir bokförda verifikationer?

* Från kontoutdrag får vi konteringar och eventuella saldo-angivelser.
    * Vid inläsning av kontoutdrag väljer användaren vilket BAS-konto som skall spegla kontoutdragets angivelser.
    * Cratchit kan på så sätt spara enskilda konteringar. Men också eventuella saldo-angivelser.
    * Konteringar kan direkt sparas som utkast med en ännu ej angiven sammanfattning.
    * Saldo-angivelser kan sparas separat för avstämningar.
* Med kod från pdf_walker kan cratchit läsa in affärshändelser från pdf-er
    * pdf-er med endast grafik (t.ex., skannade underlag) kan cratchit spara som 'källa' för att ändå hålla samman en verifikation med ett elektroniskt dokument.
    * En pdf med text kan cratchit tolka till ett utkast med rubrik, belopp och datum.

OK, börjar vi få lite kött på benen? Var börjar vi för att försöka få allt på plats utan att glömma något eller skapa ett rörigt UI?

I vilken ända börjar vi?

* Skall vi först göra stommen till en editor av en verifikation?

```text
    Utkast/Verifikation
    ID Datum Rubrik Belopp
        Konto Beskrivning Belopp Konto-namn
        Konto Beskrivning Belopp Konto-namn
        Konto Beskrivning Belopp Konto-namn
        ...
    0 Sammanfattning
    1 Konteringar

    Sammanfattning
    0 Serie
    1 Datum
    2 Rubrik
    3 Belopp

    Konteringar
    0 Konto Beskrivning Belopp Konto-namn
    1 Konto Beskrivning Belopp Konto-namn
    2 Konto Beskrivning Belopp Konto-namn
    ...
    Dolt
    + Lägg till kontering

    Kontering
    0 Konto
    1 Beskrivning
    2 Belopp
    Dolt
    b stryk (ta bort)

```

* Eller skall vi börja med att ta in koden som läser in kontoutdrag?
    * Den koden finns redan i take 3 tror jag?
* Eller skall vi börja med inläsning av elektroniska underlag (som ännu inte finns)?
    * Vi har ingen kod som tolkar text i pdf och skapar utkast.
    * Är detta 'good to have' och kan vänta?
    * Eller skall vi i varje fall kunna länka utkast till filer?

Jag tror det nog är dags ändå att faktiskt göra ett UI för att editera en bokförd Verifikation och/eller ännu ej bokfört Utkast?

* Vi borde kunna använda 'samma' editor?
* Bara med mindre begränsningar för utkast?

## 20261115

Nu vill jag försöka skapa en prouktionsredo Cratychit från tidigare sessioner.

* Vi utgår från koden vi tog fram i session/0a37da4b (../0a37da4b)  
    * Den har vårt nya UI (pyroceph och TUI)
    * Den kod vi redan har i denna session är koden från session/0a37da4b
    * För detaljer se [Överväg att vibe-koda cratchit bokslut på svenska från senaste cratchit-sessioner](../0a37da4b/session.md)
* Vi har en grundstomme till cratchit bokföring i session/60cae9fe (../60cae9fe)
    * Den har funktioner för bokföring och en datamodell vi kan inspireras av
    * För detaljer se [Vibe code a Swedish book keeping app named cratchit - take 3 (Drop feed Claude)](../60cae9fe/session.md)
* Vi har cratchit_ar i session/5b35b2b9 '../5b35b2b9'
    * Den tar en SIE-fil och skapar underlag för en årsredivsining
    * Den är verifierad mot tidigare årsredovisningar samt ett utkast från Årsredovisning Online för årets bokföring ok
    * För detaljer se [# Consider a cratchit app that maps an SIE file to a Swedish Årsredovisning?](../5b35b2b9/session.md)
* Vi har cratchit_ink2 i session/7d4475fb '../7d4475fb'
    * Den tar en SIE-file och genererar ett underlag för inkomstdeklaration 2 (INK2)
    * Den är  verifierad mot tidigare SRU-filer ok
    * Den är nu använda för att generera årets deklaration.
    * För detaljer se [Consider a cratchit app that maps an SIE file to a Swedish SKV Ink_2?](../7d4475fb/session.md)
* Vi har en app walk_pdf i session/9fe36d28 (../9fe36d28)
    * Den producerar ett underlag som visar strukturen för en pdf 
    * Den verkar kapabel att skapa klartext från en pdf (inklusive avkoda en ebook till klartext)
    * [Consider to vibe code a pdf-walker C++ console app?](../9fe36d28/session.md)

I will mix Swedish and English in this markdown depending on what audience I imagine I am writing for.

* In english for Vibe coding and Claude related general experience and observations.
* På svenska för appens faktiska funktioner och relaterat till svensk bokföring.

I have now went 'full steam ahead' and vibe coded what I think is 'from SIE-File' functionality we need for now? 

* A 'don't touch' rule for files in users book keeping folder
* A 'create here' and 'from SIE' and 'From info' that creates in-place (no sub-folder)
* Some enhanced checks and reports on SIE-file to import
* Enhanced meta-data about SIE-files (covered year, status complete/ongoing,inconsistent)

I also had time to adjust tge manual about rebuilding books from scratch using SIE-files

ALl in all 17 steps as recorded by Claude in [status.md](./status.md)
