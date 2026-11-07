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
* Prompten visar sökvägen till aktuellt state med `State::name` för varje state från roten, åtskilda av ` > ` och följda av `> `, t.ex. `Huvudmeny > Projekt> `. `State::name` är platsens namn (håll det kort), `Choice::caption` är hur man kommer dit och visas bara i menyn. Rubriken med statets namn ovanför menyn är borta.
* ` > ` i stället för `/`, så att sökvägen inte förväxlas med sökvägar på disk och så att `/` kan stå i ett namn.
* Öppen fråga: `Navigator` pekar in i ett träd som inte får ändras. När states ska skapas under körning (projekt från disk, år, konton) behöver ett val kunna skapa sitt state, och stacken med pekare får troligen göras om.
* Menyn listar bara valen som leder till ett nytt state från det state vi står i. Gemensamma val (`-` Tillbaka, `?` Visa gemensamma val, Ctrl-D/C/Z Avsluta) listas inte i menyn utan visas när användaren trycker `?` (`help_key`).
* `?` är inget state: listan skrivs ut och sedan samma prompt igen, så sökvägen och `-` påverkas inte. `?` hanteras i promptloopen, inte i `Navigator`, eftersom det är I/O. Ett val med nyckeln `?` kan inte nås.
* `common_keys(navigator)` ger listan. `-` och `?` tas från `back_key` och `help_key`, men texten för Ctrl-D/C/Z är skriven för sig och måste hållas i takt med `is_quit`.
* Introtexten är `Tryck ? för gemensamma val.` och skrivs en gång vid start.
* Enter visar valen till nya states igen, utan att statet byts. Enter är både U+000A (macOS och Linux, där terminalen gör CR till LF) och U+000D (Windows-konsol), se `is_enter`. Enter finns med bland de gemensamma valen.
* Indata: en skrivbar tangent som inte är en handling (val, `-`, `?`, Enter, avsluta) börjar redigering och läggs i en `InputBuffer` av kodpunkter. Det beror inte på om statet tar emot indata. Styrtecken utan handling ignoreras. `-` är alltid en handling i navigeringsläget och gör ingenting på roten.
* Under redigering är alla skrivbara tangenter text, även `-`, `?` och siffror. Backspace (U+007F, eller U+0008 från Windows-konsol) suddar ett tecken, och när bufferten blir tom navigerar man igen. Ctrl-U tömmer bufferten och går tillbaka till navigering. Ctrl-C/D/Z avslutar.
* Enter ger bufferten till `State::on_input`, som svarar `std::expected<std::string, std::string>`. Godkänt: svaret skrivs (om det inte är tomt), bufferten töms och man navigerar igen. Fel: feltexten skrivs, bufferten behålls och man står kvar i redigering för att rätta. Ett state utan `on_input` svarar med felet `Här finns inget att göra med indata.`
* Prompten slutar med `> ` vid navigering och `: ` vid redigering. Bytet görs på plats med två backsteg, och suddning med `\b \b`. Det fungerar för å, ä och ö men inte för breda tecken.
* `State` har konstruktorer i stället för att vara ett aggregat, så att `on_input` kan utelämnas utan varningar om saknade fält.
* Känd begränsning: indata kan inte börja med en tangent som är ett val i aktuellt state (t.ex. ett år som börjar med `2` där `2` är ett val). (Piltangenter löst i steg 19.)
* Ett svar från `on_input` kan inte byta state ännu. Väntar.
* Funktionstangenter: `KeyParser` (`pyroceph/tui/key_parser`) gör kodpunkter till `Key`, alltså en `CodePoint` eller en `FunctionKey` (pilar, Home, End, Delete, Page Up, Page Down, Esc). Den känner igen ESC `[` ... och ESC `O` ..., struntar i modifierare (Ctrl-pil är pil) och slukar okända sekvenser hela (t.ex. F-tangenter), så att inget skräp hamnar i bufferten.
* Esc ensam skiljs från början på en sekvens genom att vänta 50 ms (`escape_wait`) på mer. `KeyReader` har fått `ready(wait)` via en `ByteReady` (POSIX `poll()`, Windows `WaitForSingleObject`). Utan `ByteReady` räknas mer indata alltid som klar, vilket passar pipe och tester.
* Windows: `RawMode` slår på `ENABLE_VIRTUAL_TERMINAL_INPUT`, så att konsolen skickar samma sekvenser som andra terminaler. Går det inte (äldre Windows) körs utan.
* I redigering tömmer Esc bufferten som Ctrl-U, och Delete suddar som Backspace (markören står alltid sist). Pilar, Home, End, Page Up och Page Down gör ingenting ännu, i något läge. `?`-listan visar `Esc  Töm indata (även Ctrl-U)`.
* Steg 20: Trädet av states byggs medan användaren navigerar, i stället för att vara ett statiskt träd. Ersätter trädet som data från steg 14 och löser den öppna frågan om att `Navigator` pekade in i ett träd som inte fick ändras.
* `pyroceph::tui::State` är en basklass som klienten ärver ifrån. `name()` ger namnet i sökvägen, `options()` ger en `OptionsList` (par av `CodePoint` och text), `on_key(key)` ger ett `OnKeyResult` och `on_input(text)` ger ett `InputResult` (standard: `no_input_error`).
* `OnKeyResult` är för nu `std::unique_ptr<State>`: nästa state, eller nullptr när statet saknar mappning för tangenten. Ett eget namn så att vi har något att referera till om det byggs ut. `unique_ptr` i stället för `State*`, så att det syns i typen att TUI tar över det nya statet.
* `OptionsList` är par av tangent och text, så att TUI formaterar listan på samma sätt i alla states (`  <tangent> <text>`). Statet håller själv `options()` och `on_key()` i takt.
* `Navigator` äger stacken (`std::vector<std::unique_ptr<State>>`). `press(key)` frågar aktuellt state och lägger på det nya. `back()` tar bort det översta, som då förstörs. Roten skapas av klienten och lämnas till `Navigator`.
* Ordningen för en tangent vid navigering: Ctrl-C/D/Z avslutar alltid först. Sedan frågas statet. Ger det nullptr gäller de gemensamma tangenterna `-`, `?` och Enter. Annars börjar redigering. Ett state kan alltså ta `-` eller `?` själv (avviker från steg 14 och 16, där sådana val inte kunde nås).
* Demo-statena i `main.cpp` är klasser (`HuvudmenyState`, `ProjektState`, `OppnaProjektState`, `BokslutState`). Testerna använder en test-klass `test::MenuState` i `test/menu_state.hpp`, som bara finns i testerna.
* Steg 21: `q` (`quit_key`) avslutar vid navigering. Den är en gemensam tangent som `-` och `?`, så statet frågas först och kan ta `q` själv. Vid redigering är `q` text, så att indata kan innehålla q. Ctrl-C/D/Z avslutar som förut alltid, även vid redigering. `?`-listan visar `q  Avsluta` på en egen rad före Ctrl-D.
* Steg 22: cratchit installeras med CMake (`cmake --install`), val 1 av fyra (CMake, CPack-arkiv, installationspaket, pakethanterare, och Conan-paket som eget alternativ). Bara programmet installeras, till `<prefix>/bin` (`GNUInstallDirs`). Biblioteken är statiska och ingår i programmet. CPack och Conan-paket kan bli senare steg, Conan främst om `charset` och `pyroceph` ska delas mellan sessioner.
* Installation görs från ett Release-bygge (`conan-release`). Där kompileras `LOG_DEVELOPMENT_TRACE` bort. Prefix väljs av den som installerar, till exempel `~/.local`.
* Steg 23: Kod och dokumentation för utvecklare är på engelska, gränssnittet på svenska. Den som bygger från källkoden är utvecklare, och en svensk bokförare får cratchit först som färdigt paket. En användarguide, när den kommer, skrivs på svenska. Ingen tvåspråkig text, eftersom två versioner glider isär.
* `cratchit/README.md` är dokumentationen för den som bygger och installerar. Byggstegen finns bara där. `status.md` hänvisar dit. `beslut.md`, `status.md` och `session.md` är fortfarande på svenska.
* Steg 24: Framework kommer med installationen och runtime ligger i HOME. Ersätter beslutet från 2026-11-06 att `.cratchit` med `framework.txt` ligger i hemkatalogen och skapas av cratchit, och att `framework.txt` har `runtime = <mapp>`.
* Framework är `.cratchit` bredvid programmet (`<prefix>/bin/.cratchit/`). cratchit läser det men skriver aldrig dit. Senare kommer regler för svensk bokföring, skatt, bokslut, deklaration och årsredovisning dit.
* `framework.txt` har bara `version = <projektets version>`, som CMake skriver in från `project(VERSION ...)`. `created` och `runtime` är borta. Saknas framework (eller `version`) avslutar cratchit med fel och exit 1: installationen är inte hel.
* Källorna ligger i `cratchit/framework/` (`framework.txt.in`). CMake sätter ihop dem i byggmappen, kopierar dem bredvid programmet vid varje bygge (målet `cratchit_framework`) och installerar dem bredvid det installerade programmet. Utvecklarbygget och det installerade programmet hittar framework på samma sätt.
* Runtime är `~/.cratchit/runtime.txt` (`created = <dag>`), skapas första gången som förut. `log.txt` ligger kvar bredvid. En kvarglömd `framework.txt` i `~/.cratchit` bryr sig cratchit inte om.
* Workspace ligger i HOME som standard. Det görs i ett eget steg, när workspace kommer.
* Programmets mapp tas från plattformen, inte från `argv[0]` (som bara är vad den som startar programmet skickar, t.ex. bara namnet vid start via `PATH`; provat på macOS). macOS `_NSGetExecutablePath`, Linux `/proc/self/exe`, Windows `GetModuleFileNameW`, och sedan `std::filesystem::canonical`, så att en symbolisk länk leder till rätt mapp. Andra plattformar ger kompileringsfel.
* `open_dot_folder` och `DotFolder` är ersatta av `open_framework`/`Framework` och `open_runtime`/`Runtime`. `dot_folder_root` heter `user_home`, och `choose_dot_folder_root` heter `choose_user_home`.
* Steg 25: Användaren gav tillstånd att ändra `.github/workflows/cratchit-0a37da4b.yml`. Workflow installerar nu Debug-bygget till `installed/` och kör det därifrån via `PATH`, så att `program_folder()` och installationen provas på Linux, macOS och Windows. Debug i stället för Release, så att Conan inte behöver köras en gång till.
* Steg 26: `cratchit/install.py` kör byggstegen i ett svep: Release-bygge, test och `cmake --install`, standard `~/.local`. Python, eftersom Conan redan kräver det och ett skript då räcker för alla plattformar (i stället för ett skalskript plus ett för Windows). Skriptet och dess meddelanden är på engelska (verktyg för utvecklare).
* Alla sökvägar tas från skriptets egen mapp, så att det fungerar från vilken mapp som helst och när `cratchit` flyttas. `--try` installerar i `cratchit/_try/` (ignoreras av Git) och kör programmet med `HOME` och `USERPROFILE` satta till `_try/home`, som sparas mellan försöken.
* CMake sparar hela sökvägar i byggmappen. Är Release-byggmappen gjord på en annan plats (`CMAKE_CACHEFILE_DIR` i `CMakeCache.txt`) tar skriptet bort den och bygger om, med ett meddelande. Bara Release-byggmappen, eftersom det är den skriptet använder.
* Steg 27: Källorna till framework ligger i `cratchit/resources/` (var `cratchit/framework/`), så att mappen i källkoden inte förväxlas med det installerade framework. I byggmappen heter det fortfarande `framework/.cratchit`.
* Steg 28: `if constexpr` med en `return` i båda vägarna skrivs med `else`. Utan `else` blir den andra `return` död kod för en del typer, och MSVC varnar (C4702), vilket blir fel. Rättat i `charset/decoder.cpp` (`Decoder::finish`), där Windows-bygget föll.
* Steg 29: Två MSVC-varningar i `test/test_prompt_loop.cpp` rättade där de står, inte avstängda. `std::tmpfile` (C4996, "unsafe") ersätts med `tmpfile_s` bara för MSVC, i hjälpfunktionen `temporary_file()`. `_CRT_SECURE_NO_WARNINGS` valdes bort, eftersom det skulle tysta varningen för all kod. En `for` över `{0x03, ...}` (int) till `std::uint8_t` (C4244) skrivs med `std::uint8_t{...}`.
