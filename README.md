# Mars Rover A* Navigation Simulator 2.5D

Etap 2: raylib, mapa izometryczna i wizualizacja A* krok po kroku. Bez rovera, efektow i dzwieku.

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
- `legacy/astar_original.cpp`: oryginalny program, zachowany bez zmian i niewlaczony do kompilacji.
- `tests/astar_tests.cpp`: mapy prostokatne, granice, przeszkody, brak trasy i start rowny celowi.

Zachowano ruch gora/dol/lewo/prawo, koszt 1, heurystyke euklidesowa,
listy otwarta/zamknieta oraz wybor ostatniego elementu przy remisie f.
Powtorzone bloki obslugi sasiadow polaczono w jedna petle.
Wspolrzedne API: x = kolumna, y = wiersz od gory, oba od zera.
Sciezka zawiera start i cel; pusty wektor oznacza brak trasy lub niepoprawne/zablokowane konce.
Mapa nie jest modyfikowana przez wyszukiwanie. Dawna konwersja y od dolu i indeksy od 1 nie sa potrzebne.
Poniewaz repozytorium nie zawieralo `grid.txt`, demonstracja tworzy mape w pamieci.

## Wizualizacja iteracyjna

Program zaczyna w stanie READY, z wstrzymanym odtwarzaniem.

- PLAY / PAUSE (Spacja): automatyczne iteracje lub zatrzymanie.
- STEP (N): zatrzymuje odtwarzanie i wykonuje dokladnie jedna iteracje.
- RESET (R): czysci wyszukiwanie i wraca do READY, zachowujac wybrana predkosc.
- Suwak oraz przyciski - / +: predkosc 1-60 iteracji na sekunde.

OPEN jest niebieskie, CLOSED ciemnofioletowe, CURRENT ma jasna pulsujaca ramke.
S i G oznaczaja start i cel. Zolta trasa pojawia sie dopiero w PATH_FOUND;
odtwarzanie zatrzymuje sie wtedy automatycznie. NO_PATH oznacza wyczerpanie OPEN,
a INVALID_ENDPOINTS niepoprawny start lub cel. Aby zaczac ponownie, uzyj RESET.
Panel pokazuje status wyszukiwania, stan odtwarzania, x/y, g/h/f biezacego pola,
liczebnosc OPEN i CLOSED oraz liczbe iteracji. CLOSED zawiera takze CURRENT.

`AStarSearch` przechowuje kopie mapy i nie zalezy od raylib ani zegara.
`step()` wybiera jedno pole o najmniejszym f (ostatnie przy remisie), przenosi
je do CLOSED i analizuje czterech sasiadow. Pierwszy krok przetwarza start.
Krok wybierajacy cel rekonstruuje trase i ustawia PATH_FOUND; wywolania
`step()` po zakonczeniu niczego nie zmieniaja. `reset()` przywraca stan poczatkowy.
Synchroniczne `findPath()` pozostaje dostepne jako wrapper dla testow i innych klientow.
