# Mars Rover A* Navigation Simulator 2.5D

Projekt studencki w C++17 i raylib. Symulator pokazuje krok po kroku wyszukiwanie A*, odkrywanie trasy, ruch łazika oraz ponowne planowanie po zablokowaniu drogi. Izometryczna grafika 2.5D jest rysowana proceduralnie.

Celem projektu jest pokazanie związku między stanem algorytmu, mapą przeszkód i ruchem pojazdu. Efekty graficzne i dźwięk są oddzielone od pathfindingu.

## Algorytm A*

Mapa to dynamiczna siatka `width × height`. Każde pole jest przejezdne albo zablokowane. Łazik porusza się do czterech sąsiadów: góra, dół, lewo, prawo. Koszt przejścia wynosi 1.

Algorytm wybiera pole o najmniejszym `f = g + h`, gdzie:

- `g` — koszt drogi od początku aktualnego wyszukiwania;
- `h` — heurystyka euklidesowa: `sqrt((x-goal.x)^2 + (y-goal.y)^2)`;
- `f` — suma kosztu dotychczasowego i szacowania pozostałej drogi.

Heurystyka nie zawyża rzeczywistego kosztu przy ruchu w czterech kierunkach. OPEN zawiera odkryte pola oczekujące na rozwinięcie; CLOSED — pola już rozpatrzone. CURRENT to pole wybrane w bieżącej iteracji. Rodzice pól pozwalają odtworzyć najkrótszą ścieżkę od START do GOAL.

Wizualizacja: OPEN — niebieski, CLOSED — fioletowy, CURRENT — jasna obwódka. Panel pokazuje współrzędne, g/h/f i liczbę iteracji. `SHOW COSTS` wyświetla koszty odwiedzonych pól przy odpowiednim zbliżeniu; najechanie na pole daje dokładny odczyt także przy szerokim widoku. Efekty nie zmieniają wartości mapy.

`STEP` wykonuje dokładnie jedną iterację A*. Po znalezieniu celu status algorytmu to `PATH_FOUND`, a symulacja rozpoczyna `PATH_REVEAL`. Kolejne segmenty złotej trasy pojawiają się od START do GOAL; jeden STEP odkrywa jeden segment. Po `PATH_READY` rozpoczyna się jazda.

## Przebieg misji i replanning

`READY → SEARCHING → PATH_REVEAL → PATH_READY → DRIVING → ARRIVAL → MISSION_COMPLETE`

Podczas jazdy `DROP ROCK` umieszcza przeszkodę na dalszej części aktualnej trasy. Łazik hamuje do bezpiecznej kratki. Po krótkim zatrzymaniu to samo A* jest inicjalizowane od tej kratki do dotychczasowego GOAL:

`DRIVING → ROUTE_BLOCKED → RECALCULATING → PATH_REVEAL → DRIVING`

Poprzednia trasa jest przygaszona. Brak alternatywy daje `NO_ROUTE`; brak drogi w pierwszym wyszukiwaniu daje `NO_PATH`. Oba przypadki pozostawiają możliwość edycji albo resetu.

Podsumowanie pokazuje długość ostatniej trasy (w segmentach), całkowity przebyty dystans, sumę iteracji wszystkich wyszukiwań, replany, czas wyszukiwania, czas misji, rozmiar mapy i seed. Czasy oznaczają czas aktywnego odtwarzania, bez PAUSE; SEARCH TIME obejmuje również animowane tempo A*, a nie sam czas CPU. Dystans jest wyrażony w długościach kafla.

`RESET` i `REPLAY` przywracają mapę z początku misji, usuwają skały dodane przez DROP ROCK, ślady, cząsteczki, alerty, dźwięki, liczniki i cinematic oraz wykonują FIT TO MAP. REPLAY od razu uruchamia misję. Zmiany wykonane w edytorze stają się nowym stanem początkowym mapy. Ustawienia głośności i prędkości pozostają wybrane przez użytkownika.

## Presentation Mode

Na ekranie pomocy lub w górnym panelu wybierz `PRESENTATION`, następnie preset i `START DEMO`:

| Preset | Mapa | Gęstość zadana | Seed |
| --- | --- | --- | --- |
| LAB DEMO | 20 × 20 | 20% | 42 |
| MARS EXPEDITION | 35 × 35 | 20% | 2027 |
| LARGE TERRAIN | 60 × 50 | 18% | 47 |

Preset generuje mapę, ustawia odległe START/GOAL, prędkości demonstracyjne, AUTO CAMERA i FIT TO MAP, a potem uruchamia SEARCHING. Dalej działają normalne PATH_REVEAL i DRIVING. Przyciskiem `DEMO BLOCK ROUTE` można pokazać blokadę i replanning. Skała nigdy nie pojawia się automatycznie. Przycisk jest dostępny tylko wtedy, gdy pozostała odpowiednio długa część drogi. PLAY/PAUSE, STEP i pozostałe ustawienia nadal działają.

Do prezentacji algorytmu warto rozpocząć od LAB DEMO przy wyłączonej burzy, zatrzymać wyszukiwanie, użyć STEP i SHOW COSTS, następnie wznowić misję i ręcznie zablokować drogę. Presety nie zastępują zwykłego generatora.

## Generator i edytor

`NEW MAP` otwiera formularz WIDTH, HEIGHT, OBSTACLE DENSITY i SEED. Wymiary: 2–512 w obu osiach, gęstość: 0–95%, seed: uint32. Dostępne są skróty 20×20, 30×30, 50×50 i 80×60, ale rozmiar można wpisać ręcznie. `GENERATE` powtarza generowanie z bieżącą konfiguracją.

Skały powstają w skupiskach, formacjach i kraterach. Generator sprawdza połączenie START–GOAL; jeżeli trzeba, usuwa przeszkody w wąskim korytarzu. Dlatego rzeczywisty procent przeszkód może być mniejszy od zadanego. Te same parametry i seed dają tę samą mapę. Edycja ręczna może utworzyć mapę bez drogi.

`EDIT MAP` udostępnia SET START, SET GOAL, ADD ROCK i ERASER. LPM używa narzędzia, PPM przełącza przeszkodę i pozwala malować przeciągnięciem. START i GOAL są chronione przed skałami; mogą wskazywać to samo przejezdne pole. Zmiana mapy czyści wyszukiwanie i trasę. Kliknięcia uwzględniają zoom oraz przesunięcie kamery.

## Sterowanie

| Przycisk / klawisz | Działanie |
| --- | --- |
| PLAY / PAUSE, Spacja | Uruchomienie / zatrzymanie symulacji |
| STEP, N | Jedna iteracja A*, segment reveal lub 1/60 s ruchu |
| RESET, R | Reset całej misji i kamery |
| SHOW COSTS, C | Koszty g/h/f na polach i pod kursorem |
| Kółko myszy | Zoom nad światem |
| Środkowy przycisk + przeciąganie, WASD | Przesuwanie kamery nad światem |
| AUTO CAMERA | Automatyczny szeroki widok, śledzenie i zakończenie |
| EDIT MAP / CLEAR PATH | Edycja / czyszczenie wyszukiwania |
| DROP ROCK / DEMO BLOCK ROUTE | Blokada dalszej części drogi podczas jazdy |
| SANDSTORM oraz LIGHT / MEDIUM / HEAVY | Burza i jej intensywność |
| AUDIO, F2 | SOUND, MASTER VOLUME, MUSIC/AMBIENT, SFX |
| M | Włączenie / wyłączenie dźwięku |
| HELP, F1 | Pomoc; START MISSION rozpoczyna lub wznawia misję |
| Esc | Zamknięcie programu |

Ręczne przesunięcie/zoom wyłącza AUTO CAMERA. Można ponownie włączyć śledzenie przyciskiem. Prędkość jazdy i tempo A* / PATH_REVEAL są niezależne. Pomoc, generator i panel audio zatrzymują misję; powrót z audio wymaga PLAY.

## Wymagania i kompilacja — Windows / MSYS2 UCRT64

Wymagane: CMake ≥ 3.20, Ninja, kompilator C++17, raylib oraz sterownik obsługujący OpenGL 3.3. Projekt korzysta z zainstalowanego raylib, bez pobierania zależności podczas konfiguracji.

W terminalu **MSYS2 UCRT64** zainstaluj pakiety:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-raylib
```

Następnie w PowerShell, w katalogu projektu (zakładana instalacja MSYS2 w `C:\msys64`):

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
cmake --preset release
cmake --build --preset release
ctest --preset release
.\build\release\src\mars_rover.exe
```

Wersja Debug: zamień `release` na `ucrt64`. Presety zawierają ścieżki do `C:/msys64/ucrt64`; przy innej instalacji zmień `CMakePresets.json`.

Zwykła konfiguracja od czystego katalogu build, bez presetów:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
$env:CMAKE_GENERATOR = "Ninja"
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
.\build\src\mars_rover.exe
```

Nie mieszaj różnych generatorów i kompilatorów w tym samym katalogu build. Możesz wskazać nowy katalog przez `-B build/clean` zamiast usuwać istniejące wyniki. Do uruchomienia potrzebne są biblioteki DLL z UCRT64 — pozostaw `ucrt64/bin` w PATH. Zasoby są kopiowane obok EXE, więc aplikacja nie wymaga uruchomienia z katalogu źródeł. Brak urządzenia audio nie blokuje symulacji.

## Zasoby i konfiguracja

- `assets/audio/` — opcjonalne pliki WAV; nazwy i zasady zastępowania w [instrukcji audio](assets/audio/README.md). Wiatr i napęd są ciągłymi strumieniami proceduralnymi.
- `assets/textures/`, `assets/ui/` — miejsca na przyszłe grafiki; obecnie świat i UI są proceduralne, bez wymaganych PNG.
- `src/presentation.h` — rozmiary, seedy i prędkości presetów demonstracyjnych.
- `src/visual_config.h` — teren, kurz, ślady, beacon i ogólne parametry wizualne.
- `src/sandstorm.h` / `StormConfig` — pule pyłu, wiatr, podmuchy, haze i przejścia. Domyślnie OFF, po włączeniu MEDIUM.
- `src/sonar.h` / `SonarConfig` — obroty anteny, okres pingu i wygląd wiązki.
- `src/audio_system.h` / `AudioConfig` — domyślny miks; `src/drive_audio_synth.h` / `DriveAudioConfig` — wiatr i silnik.

Nie są używane płatne assety. Opcjonalne pliki dźwiękowe ładowane są raz; brak lub błąd pliku uruchamia generowany dźwięk zastępczy.

## Architektura i testy

Opis modułów, przepływu danych i ograniczeń znajduje się w [PROJECT_INFO.md](PROJECT_INFO.md).

CTest obejmuje A*, dynamiczne mapy, ruch, edycję, kamerę, replanning, efekty, sonar, audio oraz pełne scenariusze presetów. Dodatkowe testy wymagające urządzeń uruchamia się jawnie:

```powershell
.\build\release\visual_smoke.exe build/visual-qa
.\build\release\audio_smoke.exe
.\build\release\audio_endurance.exe
```

`visual_smoke` renderuje scenariusze do PNG i mierzy koszt renderowania 20×20, 50×50 oraz 80×60. `audio_endurance` trwa około 4 minut. Wyniki robocze trafiają do ignorowanego katalogu `build/`. Bieżący raport weryfikacji wydania: [RELEASE_QA.md](RELEASE_QA.md).
