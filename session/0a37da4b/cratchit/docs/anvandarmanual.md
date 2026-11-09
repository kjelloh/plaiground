# Användarmanual för cratchit

cratchit är ett bokföringsprogram för terminalen, på svenska.

Manualen beskriver det som finns i cratchit i dag och växer i takt med programmet. Det som ännu inte finns står under [Kommer senare](#kommer-senare).

## Innehåll

1. [Starta och avsluta](#starta-och-avsluta)
2. [Att använda cratchit](#att-använda-cratchit)
3. [Samlingsmappen](#samlingsmappen)
4. [Bokföringar](#bokföringar)
5. [Kommer senare](#kommer-senare)
6. [Ordlista](#ordlista)

## Starta och avsluta

<!-- hjälp: start -->
Du står i början av cratchit. Tryck `0` för att öppna samlingsmappen med dina bokföringar. Tryck `q` för att avsluta.
<!-- /hjälp -->

Starta cratchit genom att skriva `cratchit` i en terminal och trycka Enter:

```text
Tryck ? för gemensamma val.

  0 Samlingsmapp
Cratchit> 
```

Tryck `q` för att avsluta.

## Att använda cratchit

### Prompten

Raden som slutar med `>` är prompten. Den visar var du är i programmet, från början till där du står nu:

```text
Cratchit > Samlingsmapp > Lägg till> 
```

Ovanför prompten visar cratchit det som hör till platsen du står på, och sedan valen du har där.

### Välja med en tangent

Varje val har en tangent framför sig. Tryck tangenten, utan Enter, så går du dit:

```text
  1 Öppna befintlig
  2 Ny bokföring
Cratchit > Samlingsmapp > Lägg till> 
```

Här tar `1` dig till *Öppna befintlig* och `2` till *Ny bokföring*.

### Hjälp

Tryck `?` var du än står, så visar cratchit de gemensamma valen. Har platsen en hjälptext säger cratchit det, och trycker du `?` en gång till visar cratchit hjälptexten ur den här manualen. Trycker du `?` ännu en gång visas de gemensamma valen igen.

### Gemensamma val

Några tangenter fungerar överallt. De står inte bland valen. Tryck `?` så visar cratchit dem:

| Tangent | Gör |
|---|---|
| `-` | Tillbaka till platsen före |
| `?` | Visa de gemensamma valen. Tryck igen för hjälp om platsen du står på. |
| Enter | Visa valen igen |
| Esc | Töm raden du skriver på (även Ctrl-U) |
| `q` | Avsluta |
| Ctrl-D | Avsluta (även Ctrl-C och Ctrl-Z) |

### Skriva text

Trycker du en tangent som inte är ett val börjar du skriva text. Prompten slutar då med `:` i stället för `>`:

```text
Cratchit > Samlingsmapp > Lägg till: x
```

Medan du skriver är alla tangenter text, även `-`, `?` och `q`.

* **Backspace** suddar det sista tecknet. Suddar du allt väljer du med tangenter igen.
* **Esc** tömmer raden, och sedan väljer du med tangenter igen.
* **Enter** lämnar texten till cratchit.

Tar platsen inte emot text säger cratchit det och låter texten stå kvar, så att du kan sudda den:

```text
Programmet tar inte emot text här.
Cratchit > Samlingsmapp > Lägg till: x
```

### Skriva en sökväg

När cratchit vill ha en mapp står en sökväg redan på raden, och du fortsätter skriva därifrån.

* **Tab** fyller i resten av ett mappnamn. Passar flera mappar fyller cratchit i så långt deras namn är lika. Kan cratchit inte fylla i mer listar den mapparna och skriver raden igen under listan.
* **`-`** går tillbaka så länge du inte har ändrat sökvägen som stod där. Har du ändrat den tömmer du först raden med Esc, och sedan går `-` tillbaka.
* **Enter** lämnar sökvägen till cratchit. Går det inte att använda den visar cratchit varför, och sökvägen står kvar så att du kan rätta den.

### Fel

<!-- hjälp: fel -->
cratchit kunde inte göra det du valde, och orsaken står ovanför prompten. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Kan cratchit inte göra det du valde hamnar du alltså på platsen *Fel*.

### Inte än

<!-- hjälp: inte-an -->
Valet du gjorde finns i menyn men cratchit kan inte utföra det än. Vad som saknas står ovanför prompten. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Ett val kan finnas i menyn innan cratchit kan utföra det. Det leder då till platsen *Inte än*, som säger vad som saknas, till exempel:

```text
Utvecklaren av cratchit har inte infört att … ännu.
Cratchit > … > Inte än (<valet>)> 
```

Tryck `-` för att gå tillbaka. Just nu finns inga sådana val.

## Samlingsmappen

<!-- hjälp: samlingsmapp -->
Samlingsmappen listar de bokföringar cratchit kommer ihåg, med namn, organisationsnummer, när de skapades och mappen de ligger i. Tryck en bokförings siffra för att öppna den. Tryck `+` för att lägga till en bokföring.
<!-- /hjälp -->

Tryck `0` i början för att öppna samlingsmappen.

Samlingsmappen listar de bokföringar den kommer ihåg, med tangenterna `1` till `9`, i bokstavsordning efter namn:

```text
/Users/anna/Documents/cratchit
    Namn           Org.nr       Skapad      Mapp
  1 Borttagen AB                            /Users/anna/Borttagen AB  (finns inte längre)
  2 Exempel AB     556677-8899  2026-11-09  Exempel AB
  3 The ITfied AB  556782-8172  2026-10-23  ~/Documents/Bokföringar/The ITfied AB
  + Lägg till bokföring
Cratchit > Samlingsmapp> 
```

* *Mapp* visar var bokföringen ligger. Ligger den i samlingsmappen står bara mappens namn, och ligger den i din hemkatalog börjar sökvägen med `~`.
* Uppgifterna hämtas från bokföringen själv varje gång listan visas. Samlingsmappen kommer bara ihåg var bokföringarna ligger.
* *(finns inte längre)* betyder att mappen är borttagen eller flyttad. Har du flyttat bokföringen lägger du till den igen på det nya stället (se [Flytta en bokföring](#flytta-en-bokföring)).
* *(går inte att öppna)* betyder att mappen finns, men att den inte innehåller den bokföring samlingsmappen kommer ihåg.

Har du fler än nio bokföringar visar cratchit de nio första och säger att det finns fler.

## Bokföringar

En bokföring är bokföringen för en juridisk person, till exempel ett aktiebolag. Den ligger i en egen mapp, som kan ligga var som helst på datorn.

### Öppna en bokföring

<!-- hjälp: bokforing -->
Du har öppnat en bokföring. Ovanför prompten står mappen den ligger i. Valen är bokföringens räkenskapsår: `0` är det aktuella året och `1`, `2`, `3` de äldre. Tryck ett års siffra för att öppna året. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Tryck bokföringens siffra i samlingsmappen. Prompten får då bokföringens namn, och cratchit visar mappen den ligger i:

```text
/Users/anna/Documents/Bokföringar/The ITfied AB
  0 2026/27 (2026-05-01 – 2027-04-30)
  1 2025/26 (2025-05-01 – 2026-04-30)
  2 2024/25 (2024-05-01 – 2025-04-30)
Cratchit > Samlingsmapp > The ITfied AB> 
```

Valen är bokföringens räkenskapsår. `0` är det aktuella, det senaste i bokföringen, och `1`, `2`, `3` är åren före det, på samma sätt som i en SIE-fil men utan minustecken. Ett räkenskapsår som följer kalenderåret heter till exempel `2025`, och ett som går över två kalenderår heter till exempel `2025/26`. Har bokföringen fler än tio räkenskapsår visar cratchit de tio senaste och säger att det finns fler.

### Räkenskapsår

<!-- hjälp: rakenskapsar -->
Du har öppnat ett räkenskapsår i bokföringen. Ovanför prompten står vilken period det gäller. Än så länge finns det inga val här. Tryck `-` för att gå tillbaka till bokföringen.
<!-- /hjälp -->

Tryck räkenskapsårets siffra i bokföringen. Prompten får då årets namn:

```text
Räkenskapsår 2025-05-01 – 2026-04-30
Cratchit > Samlingsmapp > The ITfied AB > 2025/26> 
```

### Lägg till

<!-- hjälp: lagg-till -->
Tryck `1` för att lägga till en bokföring som redan finns, eller `2` för att skapa en ny.
<!-- /hjälp -->

Du kommer hit med `+` i samlingsmappen.

### Lägga till en befintlig bokföring

<!-- hjälp: oppna-befintlig -->
Skriv sökvägen till mappen som bokföringen ligger i och tryck Enter. Tab fyller i mappnamn. Så länge du inte har ändrat sökvägen går `-` tillbaka. Annars tömmer Esc raden först.
<!-- /hjälp -->

Har du en bokföring som samlingsmappen inte listar tar du in den så här:

1. Tryck `+` (*Lägg till bokföring*) i samlingsmappen.
2. Tryck `1` (*Öppna befintlig*).
3. Skriv sökvägen till mappen som bokföringen ligger i. Sökvägen börjar i samlingsmappen, och Tab fyller i mappnamnen (se [Skriva en sökväg](#skriva-en-sökväg)).
4. Tryck Enter.

```text
Cratchit > Samlingsmapp > Lägg till > Öppna befintlig: /Users/anna/Documents/Bokföringar/The ITfied AB/
Samlingsmappen kommer ihåg The ITfied AB.
/Users/anna/Documents/Bokföringar/The ITfied AB
Cratchit > Samlingsmapp > Lägg till > Öppna befintlig > The ITfied AB> 
```

cratchit öppnar bokföringen, och samlingsmappen kommer ihåg den. När du går tillbaka till samlingsmappen finns bokföringen i listan.

Ligger det ingen bokföring i mappen säger cratchit det, och sökvägen står kvar så att du kan rätta den.

### Flytta en bokföring

Allt som hör till en bokföring ligger i dess mapp. Flytta eller kopiera mappen dit du vill ha den, och lägg sedan till den på det nya stället med *Öppna befintlig*. Samlingsmappen kommer då ihåg det nya stället i stället för det gamla.

### Ny bokföring

<!-- hjälp: ny-bokforing -->
Välj hur den nya bokföringen ska skapas: `1` från en SIE-fil eller `2` från namn, organisationsnummer och räkenskapsår.
<!-- /hjälp -->

Tryck `+` och sedan `2` (*Ny bokföring*). Där finns två val:

* `1` *Från SIE*: skapa bokföringen från en SIE-fil (se nedan).
* `2` *Från uppgifter*: skapa bokföringen från namn, organisationsnummer och räkenskapsår (se [Ny bokföring från uppgifter](#ny-bokföring-från-uppgifter)).

### Ny bokföring från en SIE-fil

<!-- hjälp: fran-sie -->
Skriv sökvägen till SIE-filen och tryck Enter. Tab fyller i mappar och SIE-filer. cratchit läser filen och visar vad den innehåller innan något skapas. Så länge du inte har ändrat sökvägen går `-` tillbaka.
<!-- /hjälp -->

En SIE-fil är en fil med bokföring som du har hämtat från ett annat bokföringsprogram. Så skapar du en bokföring från den:

1. Tryck `+`, `2` (*Ny bokföring*) och `1` (*Från SIE*).
2. Skriv sökvägen till SIE-filen. Sökvägen börjar i samlingsmappen, och Tab fyller i mappar och SIE-filer.
3. Tryck Enter. cratchit läser filen och visar vad den innehåller.
4. Tryck `1` (*Skapa bokföringen*).

```text
SIE-fil: /Users/anna/Documents/cratchit/TheITfiedAB.se
Företag: The ITfied AB (556782-8172)
Räkenskapsår: 2026-05-01 – 2027-04-30
Verifikationer: 42
Konton: 501
cratchit skapar bokföringen i /Users/anna/Documents/cratchit/The ITfied AB och flyttar dit SIE-filen.
  1 Skapa bokföringen
Cratchit > Samlingsmapp > Lägg till > Ny bokföring > Från SIE > Bekräfta> 
```

<!-- hjälp: bekrafta-sie -->
Ovanför prompten står vad cratchit hittade i SIE-filen och var bokföringen skapas. Tryck `1` för att skapa bokföringen. cratchit går då tillbaka till samlingsmappen, där bokföringen finns i listan. Tryck `-` för att gå tillbaka utan att skapa något.
<!-- /hjälp -->

cratchit skapar en ny mapp med företagets namn bredvid SIE-filen, tar in hela filen och flyttar in SIE-filen i den nya mappen. Sedan går cratchit tillbaka till samlingsmappen, där den nya bokföringen nu finns i listan.

cratchit skapar ingenting om filen inte går att använda, till exempel om den inte är en SIE-fil, har fel eller saknar företagets namn eller räkenskapsår. Det gäller också om det redan finns en mapp med företagets namn bredvid filen. Orsaken står då under prompten, och sökvägen står kvar så att du kan rätta den.

### Ny bokföring från uppgifter

<!-- hjälp: fran-uppgifter -->
Skriv företagets namn och tryck Enter. Bokföringen får en mapp med det namnet i samlingsmappen. Sedan frågar cratchit efter organisationsnummer och räkenskapsår.
<!-- /hjälp -->

Har du ingen SIE-fil skapar du bokföringen från uppgifter om företaget:

1. Tryck `+`, `2` (*Ny bokföring*) och `2` (*Från uppgifter*).
2. Skriv företagets namn och tryck Enter.
3. Skriv organisationsnumret och tryck Enter.
4. Skriv första och sista dagen i det första räkenskapsåret och tryck Enter.
5. Tryck `1` (*Skapa bokföringen*).

```text
Cratchit > Samlingsmapp > Lägg till > Ny bokföring > Från uppgifter: Exempel AB
Cratchit > Samlingsmapp > Lägg till > Ny bokföring > Från uppgifter > Exempel AB: 556677-8899
Cratchit > Samlingsmapp > Lägg till > Ny bokföring > Från uppgifter > Exempel AB > 556677-8899: 2026-01-01 2026-12-31
Företag: Exempel AB (556677-8899)
Räkenskapsår: 2026-01-01 – 2026-12-31
cratchit skapar bokföringen i /Users/anna/Documents/cratchit/Exempel AB.
Bokföringen får ingen kontoplan än.
  1 Skapa bokföringen
Cratchit > Samlingsmapp > Lägg till > Ny bokföring > Från uppgifter > Exempel AB > 556677-8899 > Bekräfta> 
```

Prompten visar det du har skrivit hittills. Går det inte att använda det du skrev visar cratchit varför, och texten står kvar så att du kan rätta den. Trycker du `-` innan du har ändrat raden kommer du tillbaka till förra uppgiften, där det du skrev står kvar.

<!-- hjälp: uppgifter-orgnr -->
Skriv företagets organisationsnummer och tryck Enter, med eller utan bindestreck, till exempel 556677-8899. cratchit kontrollerar att kontrollsiffran stämmer.
<!-- /hjälp -->

Organisationsnumret måste anges.

<!-- hjälp: uppgifter-rakenskapsar -->
Skriv första och sista dagen i det första räkenskapsåret, till exempel 2026-01-01 2026-12-31, och tryck Enter. Innevarande kalenderår står redan på raden. Räkenskapsåret ska börja den första dagen i en månad, sluta den sista dagen i en månad och vara högst 18 månader.
<!-- /hjälp -->

Ett första räkenskapsår får vara kortare eller längre än tolv månader, men högst 18 månader.

<!-- hjälp: bekrafta-uppgifter -->
Ovanför prompten står uppgifterna och var bokföringen skapas. Tryck `1` för att skapa bokföringen. cratchit går då tillbaka till samlingsmappen, där bokföringen finns i listan. Tryck `-` för att gå tillbaka och ändra något.
<!-- /hjälp -->

Bokföringen får än så länge ingen kontoplan (se [Kommer senare](#kommer-senare)).

## Kommer senare

* En kontoplan för en ny bokföring från uppgifter. cratchit kommer att erbjuda BAS-kontoplanen.

## Ordlista

| Ord | Betyder |
|---|---|
| bokföring | Bokföringen för en juridisk person, i en egen mapp |
| samlingsmapp | Där cratchit håller reda på dina bokföringar |
| sökväg | Var en mapp ligger på datorn, till exempel `/Users/anna/Documents` |
| prompt | Raden där cratchit väntar på din tangent eller text |
| organisationsnummer | Företagets nummer hos Bolagsverket, tio siffror, till exempel 556677-8899 |
| kontoplan | Listan över bokföringens konton, till exempel BAS-kontoplanen |
| SIE-fil | En fil med bokföring i ett standardformat, som bokföringsprogram använder för att lämna över bokföring till varandra |
| räkenskapsår | Den period, oftast tolv månader, som bokföringen räknas och avslutas för |
