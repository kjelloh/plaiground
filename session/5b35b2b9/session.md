# # Consider a cratchit app that maps an SIE file to a Swedish Årsredovisning?

```#5b35b2b9```

## 20261102

So the reason and ways to generate amounts and structure of the Swedish 'Årsrapport' remains opaque!

* But what amounts have I already figured out when doing the 'Årsrapport' manually?
* What mechanism could I and should I use to produce something that prints in a good looking way?
* How can I name the elements and amounts on the Swedish 'Årsrapport'?

For financial year 2023-2024 I found an AppleNumbers document where I have defined mapping from BAS accounts to ÅR rows.

* I created [bas_to_ar_2024.csv](./cratchit/Resources/bas_to_ar_2024.csv)

Now where did I get this mapping from?

* In cratchit::zeroth I had code that parsed a text file and dumped those mappings.

```c++
    else if (ast[0] == "-ar_vs_bas") {
      auto ar_entries = BAS::K2::AR::parse(BAS::K2::AR::ar_online::bas_2024_mapping_to_k2_ar_text);
      std::cout << "\nÅR <--> BAS Accounts table {";
      std::cout << "\n\tÅR\tBAS Range";
      for (auto const& ar_entry : ar_entries) {
        std::cout << "\n\t" << ar_entry.m_field_heading_text;
        std::cout << "\t" << ar_entry.m_bas_accounts_text;
      }
      std::cout << "\n} // ÅR <--> BAS Accounts table";
    }
```

* AHA! I found a comment that this data comes from [BAS-kontoplan och din årsredovisning](https://www.arsredovisning-online.se/bas-kontoplan/)

Well then? We are in fact all set to ask Claude to help us put all this together?

* Create a C++ cratchit APP by cloning code from cratchit app based on  [Cratchit session 60cae9fe CMakeLists.txt](../60cae9fe/cratchit/CMakeLists.txt)
    * It shall re-use SIE file parser.
    * It shall make a cratchit app that only takes and SIE File and produces an 'Årsrapport'.
    * It shall use current [BAS-kontoplan och din årsredovisning](https://www.arsredovisning-online.se/bas-kontoplan/) and create bas_to_ar_2026.csv on the same form as [bas_to_ar_2024.csv](./cratchit/Resources/bas_to_ar_2024.csv)
* The created cratchit app shall be able to take an SIE file as the input argument and print out a generated Swedish 'Årsrapport' formatted as plain text but mirroring the organisation of the historical 'årsrapporter', e.g., [Årsredovisning%20220501-230430.pdf](./_Resources/_årsredovisningar/Årsredovisning%20220501-230430.pdf)

Claude has now implemented an cratchit_ar that takes an SIE file and prints out a 'Resultaträkning' and 'Balansräkning'.

* It looks like well formatted (bit I have not checked the amounts myself yet)
* I think we should ask Claude to also produce a report about how the amounts are generated.
    * I would like to have the applied Voucher and BAS account and amount that goes into an entry in ÅR in an indented list under each ÅR Item.
* I think I will make Claude maintain a user manual for documentation and also to help Claude track what the app is doing and how.

Claude found a discrepancy for 2650, 2510 and 1630 and proposed they should go on the receivable or liability line depending on their balance. I let Claude do this.

* It seems reasonable?
* And we may catch this as an error when we test against existing pdfs?

I then made Claude read my pdf about BAS accounting (e-book).

* I suspected the wrong signs where due to erroneous closed books?
* That is, that I should in fact adjust the books when the signs did not match the semantics of these BAS accounts?
* And now Claude confirmed this was the case!

So I asked Claude to remove the mechanism to map based on sign.

So now we should be back on 'plain BAS mapping to ÅR' again?

So now I had some chats with Claude about 'Not 3' on AR.

* I had some thinking myself about maybe adding a column with unique machine-readable name for the 'items' on the ÅR.
* In the end I decided to let Claude do its thing as long as it produces the correct output.
* Although I now noticed it has changed the format of bast_to_ar csv-file.

    * It has columns 'Nivå;Typ;BAS Range;ÅR'
    * But the entries are not in the order of the actual output.
    * And the output also generate tables.
    * So there must be some hard coded stuff that maps items to actual aoutput format.
    * This is OK I think.
    * Although I had some ideas of my own that is now NOT implemented.

But lets see where we end up (and if I want to later tweak it)?

And now I have vibe coded a two-year Flerårsöversikt, a Resultatdisposition ("Utdelas till aktieägare" marked TBD), and a Not column in the BR.

I now gave Claude a go on refactoring a bas_to_ar_2026 from the information on the web [BAS-kontoplan och din årsredovisning](https://www.arsredovisning-online.se/bas-kontoplan/).

* I now start to think what mechanism we may incorporate to keep track of what BAS account plan revision (year) to apply vs what mapping a company actually uses?

* The SIE file lists the used accounts and how they are mapped to SRU accounts.
    * But this says nothing about what version of the BAS account plan the company uses?
    * And I fail to see how such a tracking is even possible given the current design of SIE files?

Anyhow, I now made Claude implement an bas_to_ar_2026 mapping.

* It discovered that åresredovisning online has NOT changed (still as edited 2023)
* So Claude decided to extract the mapping from my e-book!
* It then discovered my SIE files conflicts with the 2026 BAS account plan.


┌───────────────────────────────────────────┬─────────────────────────────────────┬───────────────────────────────────────┐
│                   Post                    │            Was (website)            │            Now (BAS 2026)             │
├───────────────────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────┤
│ Handelsvaror                              │ 4960‑4969, 4980‑4989                │ 4000‑4299, 4960‑4969, 4980‑4989       │
├───────────────────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────┤
│ Råvaror och förnödenheter                 │ 4000‑4799, 4910‑4931                │ 4300‑4899, 4910‑4929                  │
├───────────────────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────┤
│ Övriga immateriella anläggningstillgångar │ —                                   │ new post, 1090‑1099 (e.g. tomträtter) │
├───────────────────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────┤
│ Maskiner och andra tekniska anläggningar  │ 1210‑1219                           │ 1210‑1219, 1230‑1249                  │
├───────────────────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────┤
│ Inventarier, verktyg och installationer   │ 1220‑1279 except 1260               │ 1220‑1229, 1250‑1269                  │
├───────────────────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────┤
│ Not 3 sections                            │ over 12x0–12x9 incl. 123x/124x/127x │ over 122x, 125x, 126x                 │
├───────────────────────────────────────────┼─────────────────────────────────────┼───────────────────────────────────────┤
│ Utanför                                   │ 8999                                │ 8990‑8999                             │
└───────────────────────────────────────────┴─────────────────────────────────────┴───────────────────────────────────────┘

- Class 4 is the most important correction. BAS 2026 says outright that groups 40–42 are Handelsvaror and 43–48 Råvaror och förnödenheter. A trading company would have had its purchases on the wrong line with the website mapping.
- What's still on no line is deliberate: the 10 accounts marked "Ej K2", and the equity accounts of other company forms (2010–2072, 2080, 2088). An amount on one of those still stops the program with an error. No account is on two lines.
- Your account names differ from BAS 2026. 1226 "Elektroniklabb - Verktyg och maskiner" is "Bilar och transportmedel" in BAS 2026. 1227 "Elektroniklabb - Lager" is "Finansiellt leasade inventarier (Ej K2)". Both still land on Inventarier, but you may want to look at them.

So I have to decide how to adress these issues.

* For one, I have NOT switched to BAS 2026 yet.
* So having cratchit_to_ar apply it seems wrong?
* Also, if/when I switch I need to move my 'inventarier' 2026 and 2027 to something (or something free) in BAS 2026?

Finally, there are still parts missing in generated Årsredovisning that needs information from sources outside an SIE-file

* 'Allmänt om verksamheten' is a piece of text written by me (the company)
    * Where do we feed this to the app?
* 'Väsentliga händelser under räkenskapsåret' is also a piece of text written by me (the company)
    * Where do we feed this to the app?
* 'Flerårsöversikt (tkr)' now covers only two years (what can be determined from a single SIE file) and no note (comment)
    * We may be tempted to support several SIE files to allow the app to generated more years?
    * The note (comment) 'Den minskade omsättningen beror på vårt åtagande att utveckla nya tjänster och produkter' is written by me.
    * Should we allow a note (comment) to be added to 'Flerårsöversikt'?
* The comment in 'Resultatdisposition'
    * I had a comment 'Med hänvisning till ovanstående ...' referring to 'ABL 17 kap 3 §'
    * I think this comment was originally generated by årsredovisning-online?
    * Do I need it?
    * If so, how tdo I feed it to the app?
    * How do I link it up through the current mechanism?
* 'Not 1-Redovisningsprinciper' is missing
    * It contains a definition of how 'avskrivningar' is applied by the company.
    * How can I provide (feed) information so that the app can generate this note?
* 'Not 2-Medelantal anställda' is missing.
    * It needs to know how many employees the company has had during the year.
    * Again, how to we feed this to the app?

Maybe I should ask Claude to try and map the current generated ÅR to the requirements in [Bokföringsnämndens vägledning Årsredovisning i mindre företag (K2)](https://www.bfn.se/wp-content/uploads/vl16-10-k2ar-kons2025.pdf)?

## 202061101

Now it is time to see if we can make a C++ app like cratchit actually map an SIE file to a proper and valid Swedish 'Årsredovisning' (ÅR)?

As usual this is A DJUNGEL!

* But it seems we are to report 'Årsredovisning' to Bolagsverket?
* They also specify some electronic formats of the Årsredovisning that may accept in addition to a plain paper copy [Digital inlämning av årsredovisningar](https://bolagsverket.se/download/18.7e28353618c67d1aad6bcb/1711611730725/tekniskt-ramverk-digital-inlamning-arsredovisning-1.2.pdf)

    * XBRL - eXtensible Business Reporting Language, en XML-standard för rapportering av olika typer av * företagsinformation
    * iXBRL - Inline XBRL – ett XHTML-dokument som innehåller data som taggats upp enligt en XBRL-taxonomi
    * XHTML - eXtensible HyperText Markup Language, en striktare variant av HTML. Dokumentformat som används för presentation i webbläsare

At this stage I am nopt user it is worth the effort to generate ÅR in iXBRL or XHTML?

* Can we manually upload such document to Bolagsverket?

Initially I wonder what path from our existing BAS accounting we can 'bet on' to generate a valid Årsredovisning?

* Can we map from [BAS Inkomstdeklaration 2](https://www.bas.se/wp-content/uploads/2024/11/INK2_P1-231002_241119.pdf)?
* Can we map to [Bokföringsnämndens vägledning Årsredovisning i mindre företag (K2)](https://www.bfn.se/wp-content/uploads/vl16-10-k2ar-kons2025.pdf)?

It would be nice if the SRU account values are all we need?

* Then we need to define BAS to SRU only once?

    * And we get BAS -> SRU -> Årsredovisning

For a more solid base back to actual law we can maybe use SKV specification of the 'K-regelverken' back to a set of official laws and rules  [Kännetecknande för de olika K-regelverken](https://www4.skatteverket.se/rattsligvagledning/edition/2026.14/461535.html#h-K2-arsredovisning-i-mindre-foretag)

* ABL Aktiebolagslagen (2005:551)
* BFL Bokföringslagen (1999:1078)
* FL Lagen (2018:672) om ekonomiska föreningar
* IL Inkomstskattelagen (1999:1229)
* Lagen om förvaltning av samfälligheter (1973:1150) om förvaltning av samfälligheter
* Stiftelselagen Stiftelselagen (1994:1220)
* Tryggandelagen Lagen (1967:531) om tryggande av pensionsutfästelse m.m.
* ÅRL Årsredovisningslagen (1995:1554)

I think I will reverse engineer this from actual ÅR I have for previous years.

* I have pdf:s generated by [Årsredovisning enkelt online](https://edeklarera.se/arsredovisning?gad_source=1&gad_campaignid=10591808768&gbraid=0AAAAADZf1hd5WPQhwcgaErB8ROkkRMByR&gclid=CjwKCAjwxpvXBhASEiwAue4B4c-tVENj_8i3KBv0NW_ag7xi0On96PEGbimAGWQiwvRCmFrbKFo-lBoCijkQAvD_BwE)

    * They refer to [Årsredovisningslag (1995:1554)](https://www.riksdagen.se/sv/dokument-och-lagar/dokument/svensk-forfattningssamling/arsredovisningslag-19951554_sfs-1995-1554/#K10)

    ```text
    3 §   Balansräkningen ska upprättas enligt den uppställningsform som anges i bilaga 1 till denna lag. Resultaträkningen ska upprättas enligt någon av de uppställningsformer som anges i bilagorna 2 och 3.
    ```

    ```text
Bilaga 1

Uppställningsform för balansräkningen (kontoform)
TILLGÅNGAR
      A. Tecknat men ej inbetalt kapital
      B. Anläggningstillgångar
      I. Immateriella anläggningstillgångar
   1. Balanserade utgifter för utvecklingsarbeten och liknande arbeten
   2. Koncessioner, patent, licenser, varumärken samt liknande rättigheter
   3. Hyresrätter och liknande rättigheter
   4. Goodwill
   5. Förskott avseende immateriella anläggningstillgångar
II. Materiella anläggningstillgångar
   1. Byggnader och mark
   2. Maskiner och andra tekniska anläggningar
   3. Inventarier, verktyg och installationer
   4. Pågående nyanläggningar och förskott avseende materiella anläggningstillgångar
III. Finansiella anläggningstillgångar
   1. Andelar i koncernföretag
   2. Fordringar hos koncernföretag
   3. Andelar i intresseföretag och gemensamt styrda företag
   4. Fordringar hos intresseföretag och gemensamt styrda företag
   5. Ägarintressen i övriga företag
   6. Fordringar hos övriga företag som det finns ett ägarintresse i
   7. Andra långfristiga värdepappersinnehav
   8. Lån till delägare och andra, till vilka delägare står i sådant förhållande
som sägs i 21 kap. 1 § 3, 4 eller 5 aktiebolagslagen (2005:551)
   9. Andra långfristiga fordringar
      C. Omsättningstillgångar
      I. Varulager m.m.
   1. Råvaror och förnödenheter
   2. Varor under tillverkning
   3. Färdiga varor och handelsvaror
   4. Pågående arbete för annans räkning
   5. Förskott till leverantörer
II. Fordringar
   1. Kundfordringar
   2. Fordringar hos koncernföretag
   3. Fordringar hos intresseföretag och gemensamt styrda företag
   4. Fordringar hos övriga företag som det finns ett ägarintresse i
   5. Övriga fordringar
   6. Förutbetalda kostnader och upplupna intäkter
III. Kortfristiga placeringar
   1. Andelar i koncernföretag
   2. Övriga kortfristiga placeringar
IV. Kassa och bank
EGET KAPITAL, AVSÄTTNINGAR OCH SKULDER
      A. Eget kapital, med uppgift om vad som utgör fritt eget kapital och bundet eget kapital
Aktiebolag:
      I. Aktiekapital
II. Överkursfonder
   1. Bunden överkursfond
   2. Fri överkursfond
III. Uppskrivningsfond
IV. Andra fonder
   1. Reservfond
   2. Kapitalandelsfond
   3. Fond för verkligt värde
   4. Fond för utvecklingsutgifter
   5. Övrigt
      V. Balanserad vinst eller förlust
VI. Årets resultat
Ekonomiska föreningar:
      I. Inbetalda insatser och emissionsinsatser
   1. Medlemsinsatser
   2. Förlagsinsatser
II. Uppskrivningsfond
III. Andra fonder
   1. Reservfond
   2. Kapitalandelsfond
   3. Fond för verkligt värde
   4. Fond för utvecklingsutgifter
   5. Övrigt
IV. Balanserad vinst eller förlust
      V. Årets resultat
Handelsbolag och enskilda näringsidkare:
      I. Eget kapital vid räkenskapsårets början
II. Insättningar eller uttag under året
III. Förändringar i kapitalandelsfonden
IV. Förändringar i fonden för verkligt värde
      V. Årets resultat
VI. Eget kapital vid räkenskapsårets slut
Övriga företag:
      I. Kapital
II. Fonder
   1. Underhålls- och förnyelsefond
   2. Kapitalinkomstfond
   3. Kapitalandelsfond
   4. Fond för verkligt värde
   5. Övrigt
III. Balanserad vinst eller förlust
IV. Årets resultat
      B. Obeskattade reserver
      C. Avsättningar
   1. Avsättningar för pensioner och liknande förpliktelser
   2. Avsättningar för skatter
   3. Övriga avsättningar
      D. Skulder
   1. Obligationslån
   2. Skulder till kreditinstitut
   3. Förskott från kunder (får även redovisas som avdragspost under
Varulager m.m.)
   4. Leverantörsskulder
   5. Växelskulder
   6. Skulder till koncernföretag
   7. Skulder till intresseföretag och gemensamt styrda företag
   8. Skulder till övriga företag som det finns ett ägarintresse i
   9. Skatteskulder
   10. Övriga skulder
   11. Upplupna kostnader och förutbetalda intäkter Lag (2024:347).
Bilaga 2

Uppställningsform I för resultaträkning (rapportform med kostnadsslagsindelning)
   1. Nettoomsättning
   2. Förändring av varulager
   3. Aktiverat arbete för egen räkning
   4. Övriga rörelseintäkter
   5. Råvaror och förnödenheter
   6. Övriga externa kostnader
   7. Personalkostnader
   8. Avskrivningar och nedskrivningar av materiella och immateriella anläggningstillgångar
   9. Nedskrivningar av omsättningstillgångar utöver normala nedskrivningar
   10. Övriga rörelsekostnader
   11. Intäkter från andelar i koncernföretag
   12. Intäkter från andelar i intresseföretag och gemensamt styrda företag
   13. Intäkter från övriga företag som det finns ett ägarintresse i
   14. Intäkter från övriga värdepapper och fordringar som är anläggningstillgångar (med särskild uppgift om intäkter från koncernföretag)
   15. Övriga ränteintäkter och liknande intäkter (med särskild uppgift om intäkter från koncernföretag)
   16. Nedskrivningar av finansiella anläggningstillgångar och kortfristiga placeringar
   17. Räntekostnader och liknande kostnader (med särskild uppgift om kostnader som avser koncernföretag)
   18. Bokslutsdispositioner
   19. Skatt på årets resultat
   20. Övriga skatter
   21. Årets resultat Lag (2015:813).    
    ```

