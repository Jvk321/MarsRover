# Mars Rover A* Navigation Simulator 2.5D

Etap 9: audio misji, koncowe podsumowanie, pomoc i feedback stanow.

## Windows / MSYS2 UCRT64

W terminalu MSYS2 UCRT64 zainstaluj zaleznosci:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-raylib
```

W PowerShell dodaj narzedzia UCRT64 do PATH, nastepnie w katalogu projektu:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
cmake --preset ucrt64
cmake --build --preset ucrt64
ctest --preset ucrt64
.\build\ucrt64\src\mars_rover.exe
```

Preset zaklada MSYS2 w `C:/msys64`; dla innej instalacji zmien sciezki w `CMakePresets.json`.
PATH jest potrzebny rowniez podczas uruchamiania (biblioteki DLL UCRT64/raylib).
Zamknij okno lub nacisnij Escape, aby zakonczyc.

## Kod

- `src/astar.h`, `src/astar.cpp`: dynamiczny `Grid(width, height)` i `findPath` bez zaleznosci od raylib.
- `src/main.cpp`: integracja paneli, edycji, symulacji i renderowania; domyslna mapa 20x20.
- `src/terrain.h`, `src/terrain.cpp`: deterministyczny generator, weryfikacja przejezdnosci i operacje edycji.
- `src/terrain_panel.h`, `src/terrain_panel.cpp`: formularz parametrow i presetow.
- `src/simulation.h`, `src/simulation.cpp`: stan symulacji, odtwarzanie, zegar i odkrywanie trasy; bez raylib.
- `src/rover.h`, `src/rover.cpp`: ciagla pozycja, obrot, przechyl i animacja kol; bez raylib.
- `src/rover_renderer.h`, `src/rover_renderer.cpp`: izometryczny model z podstawowych ksztaltow raylib.
- `src/world_camera.h`, `src/world_camera.cpp`: kamera, granice, kadrowanie i konwersja klikniec na pola; bez wykonywania A* i logiki rovera.
- `legacy/astar_original.cpp`: oryginalny program, zachowany bez zmian i niewlaczony do kompilacji.
- `tests/astar_tests.cpp`: mapy prostokatne, granice, przeszkody, brak trasy i start rowny celowi.
- `tests/simulation_tests.cpp`: automatyczne odkrywanie, pauza, STEP, RESET i mapa 33x25.
- `tests/rover_tests.cpp`: interpolacja, cztery kierunki, skrety i niezaleznosc od kroku czasu.
- `tests/camera_tests.cpp`: mapy 20x20 i 48x35, zoom, przesuwanie, sledzenie, pauza, reset i zblizenie po przyjezdzie.
- `tests/terrain_tests.cpp`: wszystkie presety, seed, przejezdnosc, optymalna trasa A*, edycja i klikniecia po zoomie/przesunieciu.
- `tests/replanning_tests.cpp`: hamowanie, trzy kolejne blokady na mapie 80x60, nowe A*, kontynuacja, NO_ROUTE, PAUSE/STEP i RESET.
- `src/scene_renderer.*`: proceduralny teren, obiekty z wysokoscia, projekcje tras, beacon, paralaksa, culling i kolejnosc glebi.
- `src/visual_effects.*`: odseparowany zegar i dane efektow: slady, kurz, skany; nie modyfikuje symulacji.
- `src/visual_config.h`: wspolne parametry wygladu, limity i predkosci efektow.
- `tests/visual_tests.cpp`: deterministyczny wyglad, pauza, limity i czyszczenie efektow.
- `tests/visual_smoke.cpp`: scenariusz GPU zapisujacy obrazy osmiu faz na mapie 80x60.

Zachowano ruch gora/dol/lewo/prawo, koszt 1, heurystyke euklidesowa,
listy otwarta/zamknieta oraz wybor ostatniego elementu przy remisie f.
Powtorzone bloki obslugi sasiadow polaczono w jedna petle.
Wspolrzedne API: x = kolumna, y = wiersz od gory, oba od zera.
Sciezka zawiera start i cel; pusty wektor oznacza brak trasy lub niepoprawne/zablokowane konce.
Mapa nie jest modyfikowana przez wyszukiwanie. Dawna konwersja y od dolu i indeksy od 1 nie sa potrzebne.
Poniewaz repozytorium nie zawieralo `grid.txt`, demonstracja tworzy mape w pamieci.

## Generator i edytor

NEW MAP otwiera formularz WIDTH, HEIGHT, OBSTACLE DENSITY i SEED.
Kliknij pole i wpisz liczbe (Backspace usuwa, Tab zmienia pole).
Obslugiwane sa dowolne rozmiary 2-512 w kazdej osi, gestosc 0-95% i seed uint32.
Presety LAB 20x20, MEDIUM 30x30, EXPEDITION 50x50 i LARGE 80x60 zmieniaja rozmiar,
zachowujac gestosc i seed. GENERATE TERRAIN zatwierdza formularz.
Przycisk GENERATE nad mapa powtarza generowanie z ostatnimi zatwierdzonymi parametrami.

Generator laczy eliptyczne skupiska skal, wieksze formacje, obrecze malych kraterow
i pojedyncze glazy. Sprawdza polaczenie START-GOAL za pomoca BFS; jesli formacje
odciely droge, odblokowuje waski korytarz i sprawdza polaczenie ponownie.
Po tej naprawie rzeczywista gestosc moze byc nizsza od zadanej; panel pokazuje
rzeczywista liczbe i procent przeszkod. Te same parametry i seed daja identyczna mape.
START i GOAL sa zawsze przejezdne. Nowa mapa czysci symulacje, ustawia EDIT_MAP
i wykonuje FIT TO MAP, zachowujac wybor AUTO CAMERA.

EDIT MAP zatrzymuje symulacje i czysci stara trase. Narzedzia:

- SET START (domyslne): LPM na pustym polu zmienia START.
- SET GOAL: LPM na pustym polu zmienia GOAL.
- ADD ROCK: LPM/przeciagniecie maluje przeszkody.
- ERASER: LPM/przeciagniecie usuwa przeszkody.
- PPM: przelacza wskazane pole. Przeciagniecie stosuje ten sam tryb malowania/wymazywania co pierwsze pole.

Nie mozna ustawic przeszkody na START ani GOAL. Edycja dziala tylko w EDIT_MAP,
przez aktualna odwrotna transformacje kamery. Kazda zmiana usuwa stara trase,
OPEN/CLOSED z poprzedniego wyszukiwania, animacje i rover. Nie uruchamia A*.
PLAY lub STEP rozpoczyna nowe wyszukiwanie; reczna edycja moze odciac droge,
wtedy A* zakonczy sie stanem NO_PATH. CLEAR PATH czysci symulacje bez zmiany terenu.
Panel MAP/OBSTACLES/SEED pozostaje nieruchomy. Seed opisuje wygenerowana baze,
a reczne poprawki nie sa zapisywane w seedzie.

## Wizualizacja iteracyjna

Program zaczyna w stanie READY, z wstrzymanym odtwarzaniem.

- PLAY / PAUSE (Spacja): odtwarzanie lub zatrzymanie eksploracji, odkrywania trasy i jazdy.
- STEP (N): zatrzymuje odtwarzanie i wykonuje jedna iteracje A* lub odslania jeden segment trasy.
- RESET (R): czysci wyszukiwanie i wraca do READY, zachowujac wybrana predkosc.
- Suwak oraz przyciski - / +: predkosc 1-60 krokow na sekunde, wspolna dla obu faz.

OPEN jest niebieskie, CLOSED ciemnofioletowe, CURRENT ma jasna pulsujaca ramke.
S i G oznaczaja start i cel. Po PATH_FOUND algorytm A* juz sie nie zmienia,
a symulacja przechodzi do PATH_REVEAL (panel: REVEALING PATH...).
Trasa odtworzona po rodzicach jest uporzadkowana od START do GOAL.
Na poczatku podswietlony jest tylko START; kazdy krok odslania jeden segment
do nastepnego pola. Odkryte pola sa jasnozlote, a ostatnie odsloniete pole
ma pulsujaca ramke. Nieodkryta czesc nie ma oznaczenia trasy; OPEN i CLOSED pozostaja widoczne.
PLAY kontynuuje automatycznie odkrywanie, PAUSE zamraza postep,
a STEP odslania dokladnie jeden segment i pozostawia symulacje w pauzie.
Po odkryciu GOAL symulacja przechodzi do PATH_READY, a przy odtwarzaniu
w nastepnej aktualizacji rozpoczyna DRIVING od START. Przy pracy krokowej
PATH_READY pozostaje w pauzie do PLAY lub STEP.
NO_PATH oznacza wyczerpanie OPEN,
a INVALID_ENDPOINTS niepoprawny start lub cel. Aby zaczac ponownie, uzyj RESET.
Panel pokazuje status wyszukiwania, stan odtwarzania, x/y, g/h/f biezacego pola,
liczebnosc OPEN i CLOSED oraz liczbe iteracji. CLOSED zawiera takze CURRENT.
PATH LENGTH to liczba segmentow (liczba pol minus jeden, czyli koszt trasy).
REVEALED pokazuje odkryte segmenty / wszystkie segmenty.
RESET dziala takze podczas PATH_REVEAL: czysci trase, licznik odkrywania i wyszukiwanie.
Dla START = GOAL trasa nie ma segmentow i od razu osiaga PATH_READY.

`AStarSearch` przechowuje kopie mapy i nie zalezy od raylib ani zegara.
`step()` wybiera jedno pole o najmniejszym f (ostatnie przy remisie), przenosi
je do CLOSED i analizuje czterech sasiadow. Pierwszy krok przetwarza start.
Krok wybierajacy cel rekonstruuje trase i ustawia PATH_FOUND; wywolania
`step()` po zakonczeniu niczego nie zmieniaja. `reset()` przywraca stan poczatkowy.
Synchroniczne `findPath()` pozostaje dostepne jako wrapper dla testow i innych klientow.

## Jazda rovera

Rover porusza sie po zapisanej trasie, interpolujac pozycje miedzy srodkami pol.
Przed zmiana kierunku zatrzymuje sie i obraca plynnie przez 0.45 s.
Kierunek poczatkowy odpowiada pierwszemu segmentowi. Pomaranczowy przod
i strzalka wskazuja kierunek w tej samej projekcji izometrycznej co mapa.
Podczas skretu nadwozie delikatnie sie przechyla, a kola sa nieruchome.
Kola obracaja sie proporcjonalnie do przebytej drogi, bez animacji podczas pauzy.

Osobny suwak DRIVE i przyciski - / + ustawiaja predkosc 0.25-5 pol/s,
niezalezna od tempa A* i PATH_REVEAL. Panel pokazuje pozycje x/y,
numer segmentu (od 1), TRAVELLED (droga w jednostkach pol) i predkosc.
STEP podczas DRIVING zatrzymuje odtwarzanie i przesuwa czas jazdy o 1/60 s.
PAUSE zamraza pozycje, obrot, kola i przechyl. RESET usuwa rover i trase,
wracajac do READY; obie wybrane predkosci pozostaja zachowane.
Po dotarciu do GOAL rover zatrzymuje sie. Stan ARRIVAL trwa 2 sekundy odtwarzania,
po czym nastepuje MISSION_COMPLETE i zatrzymanie odtwarzania.
PAUSE zatrzymuje rowniez zegar przyjazdu; STEP przesuwa go o 1/60 s.
START = GOAL przechodzi przez ARRIVAL bez jazdy.

## Kamera 2.5D

- Kolko myszy: plynny zoom, ograniczony od widoku calej mapy do zblizenia rovera.
- Srodkowy przycisk myszy i przeciagniecie albo WASD: plynne przesuwanie.
- Sterowanie reczne dziala z kursorem nad swiatem gry; przewijanie nad panelem nie zmienia kamery.
- Reczne przesuniecie lub zoom automatycznie wylacza AUTO CAMERA. Przycisk w panelu wlacza go ponownie.
- RESET natychmiast kadruje cala mape i zachowuje wybor AUTO CAMERA.

Automat utrzymuje szeroki widok w READY/SEARCHING, delikatnie zbliza obszar
trasy w PATH_REVEAL i podaza za roverem w DRIVING. Sledzenie korzysta z ciaglej
pozycji, wykladniczego wygladzania i malego przesuniecia w kierunku jazdy.
PAUSE zamraza automatyczna kamere; sterowanie reczne pozostaje dostepne.
Podczas ARRIVAL kamera dodatkowo zbliza sie do rovera i celu.
Przy wylaczonym AUTO CAMERA kadrowanie pozostaje reczne, takze podczas ARRIVAL.
Punkt zainteresowania jest ograniczony do rombu mapy, aby przesuwanie nie zgubilo terenu.
Panel, legenda i przyciski sa rysowane poza transformacja kamery; swiat jest przyciety do osobnego obszaru ekranu.

## DROP ROCK i ponowne planowanie

Podczas DRIVING przycisk DROP ROCK losuje pole co najmniej cztery segmenty
przed roverem, poza START i GOAL. Przy zbyt krotkiej pozostalej trasie jest
nieaktywny. Nowy glaz staje sie przeszkoda zarowno dla A*, jak i edytora mapy.

Przebieg: DRIVING -> ROUTE_BLOCKED -> RECALCULATING -> PATH_REVEAL -> PATH_READY -> DRIVING.
W ROUTE_BLOCKED rover plynnie hamuje do srodka najblizszego bezpiecznego pola
naprzod, bez cofania do START. Jesli akurat obracal sie w miejscu, pozostaje tam.
Po zatrzymaniu czeka 0.8 s. RECALCULATING tworzy nowa instancje tego samego A*
z pola zatrzymania do pierwotnego GOAL, na mapie ze wszystkimi dodanymi skalami.
Stare OPEN/CLOSED/CURRENT znikaja, poprzednia trasa jest przygaszona,
a nowa eksploracja i odkrywanie sa animowane jak za pierwszym razem.
Nowa trasa zachowuje pozycje, kierunek i faze kol rovera przy wznowieniu jazdy.
Bez alternatywy nastepuje NO_ROUTE z komunikatem NO ALTERNATIVE ROUTE.

PLAY/PAUSE dzialaja we wszystkich aktywnych fazach. STEP w RECALCULATING wykonuje
dokladnie jedna iteracje A*; podczas hamowania/pauzy blokady przesuwa czas o 1/60 s.
Podczas PATH_REVEAL nadal odslania jeden segment. Mozna wielokrotnie uzyc DROP ROCK,
gdy pojazd ponownie przejdzie do DRIVING. Panel pokazuje REPLANS, OLD PATH i NEW PATH;
licznik wzrasta przy rozpoczeciu nowego wyszukiwania, dlugosci sa liczone w segmentach.
NEW PATH pozostaje puste do znalezienia nowej trasy.

AUTO CAMERA przez chwile spoglada w kierunku glazu, potem wraca do rovera
i obserwuje przeliczanie, zachowujac wygladzanie. Sterowanie reczne i pauza
pozostaja dostepne. RESET kasuje przebieg i telemetrie, wraca do pierwotnego
START i kadruje mape; dodane przeszkody pozostaja w terenie. GENERATE odtwarza
mape bazowa z seedu. Nie ma dodatkowego algorytmu planowania.

## Visual polish

Scena jest rysowana proceduralnie, bez platnych ani pobieranych assetow:

- Regolit z lagodnymi plamami koloru, kamykami i peknieciami. Warianty zaleza od wspolrzednych i seedu; sa stale miedzy klatkami.
- Glazy, grupy skal, male klify i kratery z jasna gora, ciemnymi bokami i cieniem w stalym kierunku.
- Jasny szesciokolowy rover z panelem slonecznym, zawieszeniem, masztem kamery, obracajaca sie antena i swiatlami.
- Dwa ciemne slady kol oraz kurz powstajacy za pojazdem, szczegolnie przy ruszaniu i skretach.
- Obrotowa cyjanowa wiaska sonaru z krotkim ogonem i siatka izometryczna. Przed jazda skan wychodzi od rovera zaparkowanego na START.
- Beacon GOAL z antena, swiatlem, hologramem i pierscieniami; mocniejszy po ukonczeniu misji.
- Zlota projekcja trasy z poswiata i przeplywajacym swiatlem; widoczna jest tylko odkryta czesc. Stara trasa pozostaje przygaszona.
- OPEN/CLOSED jako przezroczyste niebieskie/fioletowe nakladki; CURRENT z pulsujaca obwodka.
- Ciemne niebo, dwie warstwy gor z paralaksa i daleka antena badawcza.
- Subtelny wiatr i drobiny pylu; SANDSTORM w prawym dolnym rogu zwieksza ich ilosc i dodaje przesuwajaca sie kurtyne piasku oraz zamglenie horyzontu.
- Nieruchome ciemne panele z cyjanowymi obramowaniami i kolorami stanow.

PAUSE zamraza zegar efektow i czasteczki. RESET, CLEAR PATH, nowa mapa oraz edycja
czyszcza slady i czasteczki; replanning zachowuje slady przejazdu.
Efekty nie zmieniaja mapy logicznej, A*, generatora, czasow jazdy ani replanningu.
LIDAR jest wylacznie wizualizacja, nie zmienia wiedzy algorytmu o mapie.

### Parametry

| Miejsce | Parametry |
| --- | --- |
| `src/visual_config.h` | paleta, gridAlpha, openAlpha/closedAlpha, rockHeight/shadowAlpha, prog LOD, paralaksa |
| `src/visual_config.h` | maxDust=384, maxTracks=16000, trackSpacing=0.10 pola, trackHalfWidth=0.29 |
| `src/visual_config.h` | dustLife=1.8 s, dustRate=32/s, turnDustRate=48/s, windX/windY |
| `src/sonar.h` | okresy sonaru, promien, alpha i echo pingu |
| `src/visual_config.h` | ambientCount=72, beaconSignalSpeed; parametry burzy sa w `src/sandstorm.h` |
| `src/scene_renderer.cpp` | ksztalty terenu, skal, kraterow, beaconu, detal gruntu i grubosci projekcji |
| `src/rover_renderer.cpp` | geometria korpusu, kol, masztu i swiatel |
| `src/main.cpp`, `src/terrain_panel.cpp` | kolory i geometria paneli/przyciskow |

Po osiagnieciu limitu sladow usuwane sa najstarsze. Emisja kurzu i liczba skanow
sa ograniczone; sonar uzywa jednego zegara bez puli fal, a wiatr ma stala liczbe drobin.
Rysowanie detali i obiektow poza widokiem kamery jest pomijane; przy oddaleniu
wylaczane sa drobne detale gruntu i czesc dodatkowych skalek.
Maski A* sa przygotowywane raz na klatke w O(liczby pol + liczby wezlow),
bez przeszukiwania list dla kazdego kafla. Kolejnosc obiektow wynika z x+y.
Proceduralne funkcje `ground`, `obstacle`, `beacon` i `drawRover` mozna pozniej
zastapic rysowaniem PNG/sprite'ow bez zmieniania generatora, logiki lub modelu efektow.

### Weryfikacja graficzna

```powershell
.\build\ucrt64\visual_smoke.exe build/visual-qa
.\build\ucrt64\src\mars_rover.exe --snapshot build/visual-qa/09-interface.png
```

Pierwszy program uruchamia ukryte okno OpenGL i zapisuje SEARCHING, PATH_REVEAL,
DRIVING, SANDSTORM, kamere manualna, ROUTE_BLOCKED, RECALCULATING i MISSION_COMPLETE.
Drugi zapisuje rzeczywisty panel aplikacji i zamyka okno. Katalog wyjsciowy musi istniec
(tworzy go `visual_smoke`). Testy GPU sa osobne od `ctest`, ktory sprawdza logike i dane efektow.

## Warstwowa burza piaskowa

SANDSTORM ON/OFF pozostaje w prawym dolnym rogu. Przycisk obok przelacza
LIGHT -> MEDIUM -> HEAVY. Zarowno wlaczenie, jak i wylaczenie trwa ok. 2.5 s;
zmiana poziomu rowniez jest wygladzona. PAUSE zatrzymuje ruch pylu i zegar podmuchow.
Nie ma jednolitego pomaranczowego filtra ani zmian kamery.

Burza ma osobne warstwy: szybkie bliskie smugi o roznych rozmiarach/przezroczystosci,
drobniejszy falujacy pyl w sredniej odleglosci, neutralne zamglenie dalekich gor
i terenu oraz cienkie pasma na plaszczyznie izometrycznej.
W poblizu rovera pozostaje strefa czytelnej trasy. Dalej maleje nasycenie i kontrast;
zamglenie obejmuje materialy i tlo, nie nakladke na caly ekran.
Swiatla pojazdu i beaconu dostaja subtelna poswiate rozpraszana przez pyl.
Podmuchy zwiekszaja liczbe/szybkosc czastek i dodaja kilka szerokich, miekkich smug.

Wszystkie parametry burzy: `src/sandstorm.h`, przestrzen nazw `StormConfig`:

| Parametr | LIGHT | MEDIUM | HEAVY |
| --- | --- | --- | --- |
| nearCount / nearGust | 18 / 28 | 48 / 66 | 80 / 112 |
| midCount / midGust | 90 / 145 | 220 / 330 | 390 / 512 |
| groundCount / groundGust | 12 / 16 | 26 / 34 | 40 / 56 |
| haze | 0.16 | 0.34 | 0.72 |
| speed | 0.65 | 1.0 | 1.35 |

- Particle count i storm intensity: `profiles`, wyzej limity podstawowe / podmuch.
- Wind direction: `windDirection = {1.0, 0.32}` (prawo i w dol sceny).
- Gust frequency: `gustInterval = 5.5 s`; `gustDuration = 1.35 s`, `gustSpeedBoost = 2.0` (do 3x predkosci).
- Haze strength: `profiles[].haze`, neutralny `hazeColor`, `clearRadius = 155 px`, `fadeDistance = 620 px`.
- Transition: `transitionSeconds = 2.5`.
- Predkosci warstw: `nearSpeed`, `midSpeed`, `groundSpeed`.

`Sandstorm` przechowuje stale pule `std::array`: 112 near, 512 mid, 56 ground.
Aktualizacja i rysowanie burzy nie alokuja pamieci; liczba aktywnych czastek
zalezy od profilu, narastania i podmuchu. Ground dust jest przycinany do mapy i widoku.
Test `layered_sandstorm` sprawdza przejscia, podmuchy, limity i strefe czytelnosci.
`visual_smoke` zapisuje tez podglady `04a-light.png`, `04b-medium.png`, `04c-heavy.png` na mapie 80x60.

### Sandstorm final polish 8.2

Domyslnie OFF, wybrany poziom MEDIUM. Wylaczenie i ponowne wlaczenie zachowuje
poziom wybrany przez uzytkownika. Kamery i logiki symulacji nie zmieniono.

Haze jest neutralny kolorystycznie i nierownomierny: lagodne przemieszczajace sie
platy zalezne od wspolrzednych swiata, odleglosci oraz polozenia przed/za roverem.
Bliski plan ma strefe bez haze, srodkowy jest lekko zapylony, daleki najmocniej.
Aktywna trasa ma zmniejszone tlumienie, a dalekie tlo w HEAVY mocniejszy haze.
Polowa puli ground dust jest rozmieszczona w poblizu rovera; smugi sa pierzaste
i rysowane pod obiektami. Foreground ma dluzsze/szersze smugi i szybszy ruch.
Podmuch ma szybkie gladkie narastanie i dluzsze wygaszanie, bez naglych skokow.
Poswiate swiatel zwiekszono subtelnie, bez zmiany ich promienia.

W `src/sandstorm.h` / `StormConfig`:

| Efekt | Parametry |
| --- | --- |
| Foreground dust | `nearSpeed=1.65`, `foregroundLength=24`, `foregroundSpeedLength=26`, `foregroundGustLength=42`, `foregroundOpacity=0.32`, `foregroundWidth=1.05`, liczby near w `profiles` |
| Ground dust | `groundSpeed=0.014`, `groundOpacity=0.20`, `groundLength=34`, `groundGustLength=48`, `localGroundRadius=300 px`, liczby ground w `profiles` |
| Gust strength | `gustSpeedBoost=2.0`, `gustInterval=5.5 s`, `gustDuration=1.35 s`, limity `nearGust/midGust/groundGust` |
| Haze | `profiles[].haze`, `hazeColor`, `hazePatchScale=0.006`, `hazePatchDrift=0.17`, `hazePatchMinimum=0.38`, `farHazeBoost=1.25` |
| Visibility falloff | `clearRadius=155 px`, `fadeDistance=620 px`, `nearHazeFactor=0.16`; mniejsza wartosc near zachowuje bardziej czytelny bliski plan |
| Swiatla | `roverLightOpacity=0.28`, `beaconLightOpacity=0.27` |

Pule i ich limity pozostaja stale. W aktualizacji/rysowaniu burzy nie ma nowych alokacji.

## Audio i final misji

Przy uruchomieniu pojawia sie krotka pomoc. BEGIN otwiera interfejs misji.
F1 / HELP otwiera pomoc ponownie; F2 / AUDIO otwiera mikser (pauzuje misje).
M oraz SOUND ON/OFF wlaczaja/wylaczaja dzwiek.
Mikser ma MASTER VOLUME, MUSIC/AMBIENT i SFX. Ustawienia glosnosci przetrwaja RESET.
Przy braku urzadzenia audio symulator dziala dalej i pokazuje informacje w mikserze.

### Dzwieki i assety

`src/audio_system.h` / `AudioConfig` zawiera domyslne poziomy:
MASTER 0.65, AMBIENT 0.32, SFX 0.55. Aktualne poziomy WIND/ENGINE: `DriveAudioConfig`.
`AudioState` obserwuje stany i pozycje, nie modyfikuje symulacji.
Sygnaly stanow sa emitowane przy przejsciu, a sonar po pelnym obrocie anteny.
Silnik to stale zapetlony stream, wyciszany podczas pauzy/skretu/zatrzymania,
z glosnoscia zaleznia od rzeczywistego ruchu. Nie restartuje sie na kaflach.
Wiatr plynnie zmienia glosnosc z sila, poziomem i podmuchami sandstorm, bez podnoszenia pitchu.

Opcjonalne pliki WAV w `assets/audio/` sa ladowane raz przy starcie:

| Plik | Zdarzenie |
| --- | --- |
| `ui_click.wav` | klik interfejsu |
| `lidar.wav` | ping sonaru po pelnym obrocie |
| `path_found.wav` | znaleziona trasa, takze po replanningu |
| `route_blocked.wav` | alarm blokady |
| `recalculating.wav` | sygnal techniczny ponownego planowania |
| `success.wav` | MISSION_COMPLETE |
| `error.wav` | NO_ROUTE / NO_PATH / bledne konce |

Brakujace lub nieprawidlowe pliki sa zastepowane generowanymi tonami i szumem PCM.
Wiatr i silnik sa ciaglymi generatorami PCM, bez zapetlonych krotkich plikow.
Nie ma pobierania assetow ani ladowania dzwiekow w kazdej klatce.
Geometria tonow, czestotliwosci i dlugosci: `src/audio_system.cpp`.
Sciezka assetow: definicja `ROVER_AUDIO_DIR` w `src/CMakeLists.txt`.
Szczegoly podmiany plikow: `assets/audio/README.md`.

### Zakonczenie i reset

Na ostatnim kaflu rover hamuje plynnie do zera. Nastepnie istniejacy ARRIVAL
wykonuje zblizenie kamery; silnik cichnie, a beacon od ARRIVAL swieci mocniej.
Po MISSION_COMPLETE pojawia sie sukces audio i animowany tytul fade/scale.
Podsumowanie pokazuje PATH LENGTH ostatniej aktywnej trasy, laczna TRAVELLED,
ITERATIONS wszystkich wyszukiwan, REPLANS, MISSION TIME, MAP SIZE i SEED.
Czas obejmuje odtwarzanie, bez pauz; slady i dystans pozostaja przez replanning.
Beacon pulsuje rowniez po zakonczeniu misji.

- NEW MISSION otwiera generator po wyczyszczeniu poprzedniej misji.
- REPLAY uruchamia od nowa aktualna mape (dodane skaly pozostaja).
- BACK TO EDIT przechodzi do edycji.
- NO_ROUTE pokazuje duzy ostrzegawczy komunikat, sygnal bledu i EDIT MAP / RESET / NEW MAP.

RESET/NEW MAP czyszcza audio, statystyki, alerty, slady i czastki, wylaczaja burze
i resetuja kamere. Nowy ambient zaczyna sie od cichego fade-in, bez starego silnika/sygnalow.
RESET przywraca pierwotny START; teren pozostaje aktualny. GENERATE odtwarza baze z seedu.
Nie zmieniono A*, generatora, replanningu, PATH_REVEAL ani logiki kamery.

### Zmienione moduly i testy

- `src/audio_system.*`: zasoby raylib, generatory PCM, petle i model zdarzen audio.
- `src/mission_feedback.*`: pomoc, mikser, toasty stanow, podsumowanie i NO_ROUTE.
- `src/main.cpp`: integracja, kontrola wejscia przy modalach i wspolne czyszczenie.
- `src/rover.cpp`: hamowanie tylko na ostatnim segmencie, bez zmiany trasy.
- `src/scene_renderer.cpp`, `src/visual_effects.cpp`: silniejszy i zywy beacon finalu.
- CMake, `assets/audio/README.md` i testy.

`ctest --preset ucrt64` zawiera test audio/feedback bez urzadzenia: normalna misja,
trzy replanningi na 80x60, NO_ROUTE, PAUSE/STEP, reset, liczniki i poziomy burzy.
`audio_smoke.exe` sprawdza rzeczywiste urzadzenie, petle WAV, SOUND OFF/ON,
MASTER, pauze i zatrzymanie wszystkich zasobow przy RESET.
`visual_smoke.exe` zapisuje tez pomoc, mikser, podsumowanie i NO_ROUTE.

```powershell
.\build\ucrt64\audio_smoke.exe
.\build\ucrt64\visual_smoke.exe build/visual-qa
```

## Audio 9.1 / antena sonarowa

WIND jest cichym, niskopasmowym ambientem: niezalezny rumble, podwojnie filtrowany
szum powietrza i powolna nieregularna modulacja. Generator zachowuje filtry,
generator losowy i czas miedzy buforami; nie ma poczatku/konca ani petli 2-3 s.
Burza zwieksza poziom lagodnie, bez pisku i bez zmiany wysokosci szumu.
ENGINE laczy niski hum elektryczny, lekki whirr i sladowe serwo.
Glosnosc oraz pitch sa wygladzane i zaleza od rzeczywistej predkosci;
przy skrecie zostaje bardzo ciche serwo. Stream nie jest restartowany na kaflach.

Antena i wiaska uzywaja tego samego zegara `SonarClock` w obserwatorze wizualnym
i audio. Pelny obrot wywoluje dokladnie jeden ping 780 Hz z dwoma cichymi echami.
SEARCHING: 3.4 s/obrot, RECALCULATING: 2.7 s/obrot i pitch pingu 1.08,
DRIVING: 7.2 s/obrot. PAUSE zamraza obrot; MISSION_COMPLETE wylacza aktywny scan.
Zamiast rozchodzacej sie fali jest cienka cyjanowa wiaska, krotki sektor ogona
i subtelne izometryczne okregi odniesienia. Logika misji, UI i pozostale efekty sa bez zmian.

| Parametr | Miejsce i wartosc |
| --- | --- |
| wind volume | `src/drive_audio_synth.h`: `windVolume=0.075` |
| wind gust volume | `windGustVolume=0.035`, `stormWindLight=0.022`, `stormWindMedium=0.050`, `stormWindHeavy=0.095` |
| engine volume | `engineVolume=0.28`, `servoVolume=0.024` |
| engine pitch range | `enginePitchMin=0.94`, `enginePitchMax=1.10` |
| sonar rotation speed / ping interval | `src/sonar.h`: `searchPeriod=3.4`, `replanPeriod=2.7`, `drivePeriod=7.2`; predkosc = 360 stopni / okres |
| sonar ping volume | `pingVolume=0.58` wzgledem suwaka SFX |
| sonar sweep alpha | `sweepAlpha=0.55`, `gridAlpha=0.075`, `trailRadians=0.22` |
| sonar sweep radius | `sweepRadius=115` jednostek swiata |
| sonar ping charakter | `pingFrequency=780`, `pingDecay=0.105`, `echoDelay=0.105`, `echoGain=0.19` |

Hierarchia miksu: alerty > sonar/PATH FOUND > naped > ambient.
Wind/engine nie korzystaja juz z krotkich `ambient_wind.wav` / `rover_engine.wav`.
Pliki jednorazowych sygnalow w `assets/audio/` nadal sa obslugiwane.
Nie ma alokacji w generatorach ani tworzenia streamow w petli renderowania.

Test `wind_engine_sonar` generuje 35 s wiatru, sprawdza korelacje z opoznieniem 3 s,
ciaglosc 185 s silnika i synchronizacje wiaski/pingow. Zapisuje odsluchowy WAV
`build/audio-qa/wind-35s.wav` (zrodlo przed gainem miksera).
`audio_endurance.exe` sprawdza 240 s rzeczywistego odtwarzania: 35 s ambientu,
ponad 3 min streamu napedu, mute, pauze, wszystkie poziomy burzy i RESET.
Liczniki `engineStarts` / `windStarts` potwierdzaja brak restartowania streamow.
