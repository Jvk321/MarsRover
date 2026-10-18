# Mars Rover A* Navigation Simulator 2.5D

Etap 4: iteracyjny A*, odkrywanie trasy i jazda rovera 2.5D. Bez efektow srodowiskowych i dzwieku.

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
- `src/main.cpp`: przykladowa mapa 24x14, przeszkody, start, cel i trasa; rozmiar mozna zmienic w konstruktorze `Grid`.
- `src/simulation.h`, `src/simulation.cpp`: stan symulacji, odtwarzanie, zegar i odkrywanie trasy; bez raylib.
- `src/rover.h`, `src/rover.cpp`: ciagla pozycja, obrot, przechyl i animacja kol; bez raylib.
- `src/rover_renderer.h`, `src/rover_renderer.cpp`: izometryczny model z podstawowych ksztaltow raylib.
- `legacy/astar_original.cpp`: oryginalny program, zachowany bez zmian i niewlaczony do kompilacji.
- `tests/astar_tests.cpp`: mapy prostokatne, granice, przeszkody, brak trasy i start rowny celowi.
- `tests/simulation_tests.cpp`: automatyczne odkrywanie, pauza, STEP, RESET i mapa 33x25.
- `tests/rover_tests.cpp`: interpolacja, cztery kierunki, skrety i niezaleznosc od kroku czasu.

Zachowano ruch gora/dol/lewo/prawo, koszt 1, heurystyke euklidesowa,
listy otwarta/zamknieta oraz wybor ostatniego elementu przy remisie f.
Powtorzone bloki obslugi sasiadow polaczono w jedna petle.
Wspolrzedne API: x = kolumna, y = wiersz od gory, oba od zera.
Sciezka zawiera start i cel; pusty wektor oznacza brak trasy lub niepoprawne/zablokowane konce.
Mapa nie jest modyfikowana przez wyszukiwanie. Dawna konwersja y od dolu i indeksy od 1 nie sa potrzebne.
Poniewaz repozytorium nie zawieralo `grid.txt`, demonstracja tworzy mape w pamieci.

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
Po dotarciu do GOAL stan zmienia sie na MISSION_COMPLETE i odtwarzanie sie zatrzymuje.
START = GOAL konczy misje bez ruchu. Kamera pozostaje statyczna.
