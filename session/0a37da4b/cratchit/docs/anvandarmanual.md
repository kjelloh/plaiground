# Användarmanual för cratchit

cratchit är ett bokföringsprogram för terminalen, på svenska.

Manualen beskriver det som finns i cratchit i dag och växer i takt med programmet. Det som ännu inte finns står under [Kommer senare](#kommer-senare).

## Innehåll

1. [Installera och starta](#installera-och-starta)
2. [Att använda cratchit](#att-använda-cratchit)
3. [Samlingsmappen](#samlingsmappen)
4. [Bokföringar](#bokföringar)
5. [Var cratchit sparar sina filer](#var-cratchit-sparar-sina-filer)
6. [Kommer senare](#kommer-senare)
7. [Ordlista](#ordlista)

## Installera och starta

Hur du bygger och installerar cratchit står i [README.md](../README.md) (på engelska). I korthet: kör `python3 install.py` i mappen `cratchit`, så installeras programmet i `~/.local/bin`.

Starta cratchit med kommandot `cratchit` i en terminal. Det spelar ingen roll vilken mapp du står i.

Första gången skapar cratchit sin egen mapp `.cratchit` i din hemkatalog och berättar det:

```text
cratchit skapade .cratchit/runtime.txt i /Users/anna
Tryck ? för gemensamma val.

  0 Samlingsmapp
Cratchit> 
```

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

### Gemensamma val

Några tangenter fungerar överallt. De står inte bland valen. Tryck `?` så visar cratchit dem:

| Tangent | Gör |
|---|---|
| `-` | Tillbaka till platsen före (syns inte när du står i början) |
| `?` | Visa de gemensamma valen |
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

* **Tab** fyller i resten av ett mappnamn. Passar flera mappar fyller cratchit i så långt deras namn är lika. Kan cratchit inte fylla i mer listar den mapparna och skriver raden igen under listan. Mappar vars namn börjar med punkt syns bara när du själv har skrivit punkten.
* **`-`** går tillbaka så länge du inte har ändrat sökvägen som stod där. Har du ändrat den tömmer du först raden med Esc, och sedan går `-` tillbaka.
* **Enter** lämnar sökvägen till cratchit. Går det inte att använda den visar cratchit varför, och sökvägen står kvar så att du kan rätta den.

### Fel

Kan cratchit inte göra det du valde hamnar du på platsen *Fel*, som visar orsaken. Tryck `-` för att gå tillbaka.

### Inte än

Några val finns i menyerna innan cratchit kan utföra dem. De leder till platsen *Inte än*, som säger vad som saknas:

```text
Utvecklaren av cratchit har inte infört att skapa en ny bokföring från en SIE-fil ännu.
Cratchit > Samlingsmapp > Lägg till > Ny bokföring > Inte än (Från SIE)> 
```

Tryck `-` för att gå tillbaka.

## Samlingsmappen

Samlingsmappen är den mapp där cratchit håller reda på dina bokföringar. Den är mappen `cratchit` i din Dokument-mapp.

Tryck `0` i början för att öppna den. Första gången skapar cratchit mappen:

```text
cratchit skapade en ny samlingsmapp.
/Users/anna/Documents/cratchit
  + Lägg till bokföring
Cratchit > Samlingsmapp> 
```

På macOS kan systemet först fråga om terminalen får använda mappen Dokument. Svara ja, annars kan cratchit inte öppna samlingsmappen.

Samlingsmappen listar de bokföringar den kommer ihåg, med tangenterna `1` till `9`, i bokstavsordning efter namn:

```text
/Users/anna/Documents/cratchit
  1 The ITfied AB
  + Lägg till bokföring
Cratchit > Samlingsmapp> 
```

En bokföring kan ligga var som helst på datorn. Den behöver inte ligga i samlingsmappen. Har du fler än nio bokföringar visar cratchit de nio första och säger att det finns fler.

## Bokföringar

En bokföring är bokföringen för en juridisk person, till exempel ett aktiebolag. Den ligger i en egen mapp, som cratchit känner igen på filen `.cratchit/project.txt` i mappen.

### Öppna en bokföring

Tryck bokföringens siffra i samlingsmappen. Prompten får då bokföringens namn, och cratchit visar mappen:

```text
/Users/anna/Documents/Bokföringar/The ITfied AB
Cratchit > Samlingsmapp > The ITfied AB> 
```

Än så länge finns det inga val i en öppnad bokföring.

### Lägga till en befintlig bokföring

Har du en bokföring som samlingsmappen inte listar tar du in den så här:

1. Tryck `+` (*Lägg till bokföring*) i samlingsmappen.
2. Tryck `1` (*Öppna befintlig*).
3. Skriv sökvägen till bokföringens mapp. Den börjar i din Dokument-mapp, och Tab fyller i mappnamnen (se [Skriva en sökväg](#skriva-en-sökväg)).
4. Tryck Enter.

```text
Skriv sökvägen till mappen med bokföringen (den som har .cratchit/project.txt).
Tab fyller i mappnamn och Enter öppnar. - går tillbaka innan du skrivit något, Esc tömmer raden.
Cratchit > Samlingsmapp > Lägg till > Öppna befintlig: /Users/anna/Documents/Bokföringar/The ITfied AB/
Samlingsmappen kommer ihåg The ITfied AB.
/Users/anna/Documents/Bokföringar/The ITfied AB
Cratchit > Samlingsmapp > Lägg till > Öppna befintlig > The ITfied AB> 
```

cratchit öppnar bokföringen, och samlingsmappen kommer ihåg den. När du går tillbaka till samlingsmappen finns bokföringen i listan.

Saknar mappen `.cratchit/project.txt` säger cratchit det, och sökvägen står kvar så att du kan rätta den:

```text
cratchit kunde inte öppna bokföringen: /Users/anna/Documents/Bokföringar/.cratchit/project.txt saknas
```

### Ny bokföring

Tryck `+` och sedan `2` (*Ny bokföring*). Där finns två val:

* `1` *Från SIE*: skapa bokföringen från en SIE-fil.
* `2` *Från uppgifter*: skapa bokföringen från namn, organisationsnummer och räkenskapsår.

Inget av dem finns än. Båda leder till [Inte än](#inte-än).

## Var cratchit sparar sina filer

cratchit sparar sina uppgifter i mappar som heter `.cratchit`. Mappar som börjar med punkt syns normalt inte i Finder eller Utforskaren.

| Mapp | Innehåll |
|---|---|
| `.cratchit` i din hemkatalog | `runtime.txt` och loggen `log.txt` |
| `.cratchit` i samlingsmappen | `workspace.txt`, med de bokföringar samlingsmappen kommer ihåg |
| `.cratchit` i bokföringens mapp | `project.txt`, med bokföringens namn och id |

Dokument-mappen är `Documents` i hemkatalogen på macOS och Linux, och mappen *Dokument* på Windows (även om den är flyttad till OneDrive).

## Kommer senare

Det här finns som val men leder till [Inte än](#inte-än):

* Skapa en ny bokföring från en SIE-fil.
* Skapa en ny bokföring från namn, organisationsnummer och räkenskapsår.

## Ordlista

| Ord | Betyder |
|---|---|
| bokföring | Bokföringen för en juridisk person, i en egen mapp |
| samlingsmapp | Mappen där cratchit håller reda på dina bokföringar |
| sökväg | Var en mapp eller fil ligger på datorn, till exempel `/Users/anna/Documents` |
| prompt | Raden där cratchit väntar på din tangent eller text |
