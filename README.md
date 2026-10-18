# Mars Rover A* Navigation Simulator 2.5D

Etap 8: proceduralny Mars 2.5D, efekty wizualne i dopracowany interfejs; bez audio.

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
- Cyjanowe elipsy LIDAR co 3 s podczas SEARCHING/RECALCULATING, mocniejsze przy rozpoczeciu fazy. Przed jazda skan wychodzi od rovera zaparkowanego na START.
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
| `src/visual_config.h` | scanInterval=3 s, scanLife=2.4 s, scanRadius=145 jednostek swiata |
| `src/visual_config.h` | ambientCount=72, stormCount=220, stormOpacity=0.16, antennaSpeed, beaconSignalSpeed |
| `src/scene_renderer.cpp` | ksztalty terenu, skal, kraterow, beaconu, detal gruntu i grubosci projekcji |
| `src/rover_renderer.cpp` | geometria korpusu, kol, masztu i swiatel |
| `src/main.cpp`, `src/terrain_panel.cpp` | kolory i geometria paneli/przyciskow |

Po osiagnieciu limitu sladow usuwane sa najstarsze. Emisja kurzu i liczba skanow
sa ograniczone (maks. 4 aktywne pulsy); wiatr ma stala liczbe drobin.
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
