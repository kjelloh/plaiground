# Beslut

Kort logg över vad vi beslutat och varför.

## 2026-11-06

* Kod på engelska, UI-texter på svenska.
* Bokföringstermer som `Bokslut` och kommentarer kan vara på svenska.
* Minnet av sessionen hålls i `beslut.md` och `status.md`. `session.md` är användarens egen text.
* Användaren testar själv och committar. Claude committar inte.
* C++23, CMake och Conan 2, samma grund som 'take 3' (`session/60cae9fe`).
* `charset` tas från walk_pdf (`session/9fe36d28`) oförändrat: `unicode`, `utf8`, `utf16`, `cp437` och `detect`. Användaren vill ha `detect` redan nu eftersom appen behöver anpassa sig till plattform och runtime.
* PDF-kodningarna `winansi`, `macroman` och `adobe_standard` väntar till PDF-läsningen.
* Varningar räknas som fel för alla mål, som i walk_pdf.
* `terminal_encoding` tas från 'take 3' (`session/60cae9fe`) till `src/core/`, i ett nytt bibliotek `cratchit_core`. Enda ändringen i koden är att varningstexterna är översatta, eftersom de är UI.
* MSVC kompilerar med `/utf-8`, så att svenska UI-texter i källkoden blir rätt.
* Loggningen `core/log` tas från 'take 3'. Felet när loggen inte kan öppnas är översatt, eftersom det är UI. Loggvisaren `core/log_view` väntar, eftersom den hör till REPL:en.
* `open_data_folder` från 'take 3' heter här `open_dot_folder` (fil `core/dot_folder`, struct `DotFolder`). `core/path_value` följde med som beroende. Felen är översatta eftersom de är UI.
* Loggen är `log.txt` bredvid `runtime.txt`, som i 'take 3'. Loggtexterna från `main` är på svenska.
* Filformatet behålls: `.cratchit/framework.txt`, `.cratchit/runtime.txt` och nycklarna `created` och `runtime`.
* Kedjan i 'take 3' är framework -> runtime -> workspace -> project. Tills vidare öppnas bara framework och runtime. Loggen ligger i runtime-mappens `.cratchit`. Workspace och project väntar.
* `.cratchit` ligger i hemkatalogen (`HOME`, på Windows `USERPROFILE`), inte i startmappen och inte bredvid binären. Det gör att appen kan installeras i `PATH`. Avviker från 'take 3'.
* Ingen egen miljövariabel som `CRATCHIT_HOME` för nu: vi har inget behov av den ännu, och fler alternativ ger fler sätt att gå fel. Hemkatalogen måste vara en hel sökväg till en mapp som finns, annars blir det fel. Vid test pekas `HOME` om för den enskilda körningen.
* GitHub Actions: en workflow per session i `.github/workflows/`. Här `cratchit-0a37da4b.yml`, som bara startar vid ändringar i `session/0a37da4b/cratchit/**` och i workflow-filen. Användaren gav uttryckligt tillstånd att skriva i `.github/workflows/`, annars gäller sandlådan.
* GitHub Actions använder Conan 2.34.0, eftersom 2.17.1 inte känner till Apple clang 21 på `macos-latest`. Lokalt är Conan 2.17.1 kvar.
* GitHub Actions hämtar alltid bara sessionens filer, `session/0a37da4b/`, på alla plattformar (sparse checkout utan cone mode). På Windows stängs `core.protectNTFS` av på den tillfälliga maskinen, eftersom andra sessioner har filnamn som Windows inte tillåter.

## 2026-11-07

* `Transcoder` i `charset/transcoder`: parar ihop en Decoder med en Encoder. `push(byte)` ger `std::optional<Bytes>`, alltså de färdiga bytes för ett tecken eller `nullopt`. `finish()` tömmer det som decodern har kvar. Inga structar med fler fält i svaret (första försöket backades av den anledningen).
* `BasicTranscoder<Decoder, Encoder>` är öppen och kan användas direkt. `Transcoder` väljs när programmet körs via `make_transcoder(Encoding from, Encoding to)` och lägger en `BasicTranscoder` bakom ett virtuellt gränssnitt.
* `charset::Bytes` (1–4 bytes) är gemensam returtyp. Encoderna ändras inte, mallen gör om deras svar.
* Ett tecken som målkodningen saknar (t.ex. `€` till CP437) ger `nullopt` och försvinner tyst.
* `Encoding` har `utf8`, `utf16be`, `utf16le` och `cp437`.
* Två sorters fel hålls isär: en Decoder kan alltid mappa till Unicode men kan misslyckas att parsa sin indata, och räknar det i `failure_count()`. En Encoder kan alltid skriva sin kodning men kan sakna mappning för en kodpunkt, och räknar det i `unmapped_count()`. Olika namn så att skillnaden syns i koden.
* För UTF-8 och UTF-16 räknas ett surrogat eller ett värde över U+10FFFF som omappat: det blir U+FFFD. U+FFFD i sig är ett riktigt tecken och räknas inte.
* En Encoder ger ingenting för en kodpunkt den saknar mappning för, bara en uppräkning av `unmapped_count()`. Det gäller även U+FFFD till CP437. Ett försök med `?` för U+FFFD backades (ett `?` kan inte skiljas från ett äkta `?`, och det blev ett specialfall). Hur något bättre ska produceras beslutas senare.
* `BasicTranscoder` och `Transcoder` lämnar ut `failure_count()` från decodern och `unmapped_count()` från encodern, rakt igenom utan egen räkning. Ingen summa, eftersom samma tecken kan räknas i båda (trasig UTF-8 till CP437).
* TUI-biblioteket heter `pyroceph`, med namespace `pyroceph::tui` och kod i `src/pyroceph/tui/`. Det beror bara på `charset`, inte på `cratchit_core`, så att det kan bli ett eget bibliotek. `main` översätter `cratchit::Charset` till `charset::Encoding`.
* Tangenter blir Unicode-kodpunkter direkt vid gränsen mot terminalen (`KeyReader`). Bytekällan skickas in som funktion, så att loopen kan testas utan terminal.
* `charset::Decoder` väljs när programmet körs, med `Encoding`. `Encoding` ligger nu i `charset/encoding.hpp`, och `make_any_decoder` delas med `make_transcoder`.
* `RawMode` stänger av radbuffring, eko och signaler. Ctrl-C, Ctrl-D och Ctrl-Z kommer då som tecken och avslutar loopen, så att terminalen alltid ställs tillbaka. Avviker från förslaget att behålla signalerna: Ctrl-C hade dödat programmet med terminalen kvar i rått läge.
* På Windows läses en konsol med `ReadConsoleW` som UTF-16, oavsett kodtabell. Från en pipe läses bytes i den valda kodningen.
* Promptloopen skriver bara ASCII (`U+00E5 (inget val)`), så att kodningen av utdata kan vänta. `Choice` är tom och uppslaget hittar aldrig något. Piltangenter med mera kommer som flera kodpunkter (ESC `[` `A`) och tolkas i ett senare steg.
* Användargränssnittet är ett träd av `State` (namn och val) i `pyroceph/tui/state`. Ett `Choice` har en nyckel (`CodePoint`, helst '0'..'9'), en text och sitt mål-`State` som värde. Trädet är bara data, utan pekare och id.
* `Navigator` håller vägen från roten till aktuellt state som en stack. Ett val går in i sitt state, `-` (`back_key`) går tillbaka och gör ingenting på roten. Ett val med nyckeln `-` kan inte nås. Trädet måste leva längre än navigatorn.
* Promptloopen visar aktuellt state och dess val, och visar det nya varje gång state byts. Tangenter som inte är val ignoreras tyst. `Lookup` och den tomma `Choice` från steg 13 är borta.
* Namn och val skrivs som UTF-8 rakt ut. Att skriva i terminalens kodning (med `Transcoder`) blir ett eget steg.
