# Weryfikacja wydania 1.0

Data: 18.10.2026. Środowisko: Windows, MSYS2 UCRT64, GCC 16.2.0,
CMake + Ninja, raylib 6.2, OpenGL 3.3, AMD Radeon 780M.

## Znalezione problemy i poprawki

- RESET/REPLAY zostawiały w mapie przeszkody z DROP ROCK. Aplikacja przechowuje teraz początkową mapę misji i odtwarza ją razem z resetem audio, efektów, telemetrii i kamery. Edycja ustanawia nową mapę początkową.
- Formularz nowej mapy zachowywał starsze parametry po zmianie konfiguracji. Jest synchronizowany z aktualnie wygenerowaną mapą. Błędy generowania wracają do formularza z komunikatem.
- Brak SHOW COSTS i czasu wyszukiwania: dodano odczyt g/h/f z istniejących węzłów A* oraz czas aktywnego wyszukiwania w podsumowaniu.
- NO_PATH używało komunikatu sugerującego nieudany replanning. Rozróżniono pierwsze wyszukiwanie, brak alternatywy i nieprawidłowe START/GOAL.
- Pierwszy badany seed MARS EXPEDITION miał trzy wąskie gardła. Wybrano seed 2027. Test sprawdza, że zablokowanie dowolnego kwalifikującego się pola pierwszej trasy każdego presetu nadal pozostawia połączenie START–GOAL. Kolejne blokady nadal mogą legalnie doprowadzić do NO_ROUTE.
- Renderer tworzył bufor stanów i zaparkowany pojazd w każdej klatce. Bufory są ponownie wykorzystywane, zaparkowana reprezentacja jest odświeżana przy zmianie START, a pula kurzu ma rezerwowaną pojemność.
- Powtarzane przyciski ujednolicono w `ui_controls.h`; długie napisy dopasowują czcionkę do szerokości. Poprawiono odstęp g/h/f od nagłówka telemetrii.
- CMake uporządkowano według zależności modułów, usunięto nadmiarowe linkowanie i powtarzane deklaracje testów. Audio odczytuje opcjonalne zasoby obok EXE zamiast z katalogu źródeł.

A*, reguła remisów, generator, ruch, replanning, reveal i zachowanie kamery pozostają zachowane. Pierwotny kod autora w `legacy/` pozostaje poza buildem.

## Kompilacja i uruchomienie

- `cmake -S . -B build` — nowa konfiguracja Ninja bez wcześniejszego cache w tym katalogu.
- `cmake --build build` — wszystkie 49 kroków kompilacji/linkowania od podstaw, bez błędów i ostrzeżeń.
- `cmake --preset release` i `cmake --build --preset release` — osobny, początkowo pusty katalog `build/release`.
- Końcowy clean build w pustym `build/final-clean` — 50/50 kroków, z końcową konfiguracją kopiowania zasobów, bez błędów i ostrzeżeń.
- Program z `build/src/mars_rover.exe` uruchomiono w ukrytym oknie i zapisano rzeczywisty interfejs do `build/visual-qa/12-clean-interface.png`.
- Końcowy zrzut po dopasowaniu napisów: `build/visual-qa/09-interface.png`.
- Zasoby są kopiowane obok EXE. W obecnym zestawie nie ma wymaganych WAV/PNG — przetestowano proceduralne warianty zastępcze. Runtime nadal wymaga DLL UCRT64 w PATH, zgodnie z README.

Cel `rover_assets` odświeża pliki przy każdym buildzie, również gdy zmieniły się wyłącznie zasoby i nie było ponownego linkowania programu.

## Testy automatyczne i renderowanie

Wynik: 11/11 zestawów CTest przeszło w Debug, w zwykłym buildzie i w Release. Zestawy obejmują:

- A* i mapy dynamiczne, optymalność trasy, START=GOAL, brak drogi i nieprawidłowe końce;
- pojedynczy STEP, PAUSE, PATH_REVEAL, ruch/skręty i zakończenie misji;
- edycję START/GOAL, skał i gumki, deterministyczny seed, generator 20×20/30×30/50×50/80×60 i małe mapy;
- zoom, manualną kamerę, AUTO CAMERA, reset i screen-to-grid po przesunięciu oraz przybliżeniu;
- kilka kolejnych blokad, hamowanie, ponowne A* od zatrzymanej kratki, brak alternatywy i PAUSE/STEP podczas RECALCULATING;
- ograniczenia śladów/cząsteczek, burzę LIGHT/MEDIUM/HEAVY i przejścia, sonar i jednorazowe sygnały audio;
- trzy presety prezentacyjne, dwie blokady w każdym, dotarcie do celu i powtórkę na oryginalnej mapie;
- RESET podczas SEARCHING, PATH_REVEAL, DRIVING i RECALCULATING.

Pełny scenariusz API i renderowania:
`SEARCHING → PATH_FOUND (A*) → PATH_REVEAL → DRIVING → ROUTE_BLOCKED → RECALCULATING → PATH_REVEAL → DRIVING → MISSION_COMPLETE`.

`visual_smoke` wygenerował zrzuty wyszukiwania, kosztów, reveal, jazdy, trzech poziomów burzy, manualnej kamery, blokady, replanningu, podsumowania, pomocy, audio, presetów i NO_ROUTE. Zrzuty kosztów, podsumowania, presetów oraz rzeczywistego interfejsu sprawdzono wizualnie. Wyniki są w `build/visual-qa/`.

`audio_smoke` uruchomił rzeczywiste urządzenie WASAPI: strumienie proceduralne, SOUND OFF/ON, MASTER VOLUME, PAUSE/wznowienie i reset bez pozostawionych odtwarzanych dźwięków. Zegar sonaru i ciągłość PCM są dodatkowo sprawdzane w CTest.

## Płynność

Pomiar bez limitu FPS: pełny szeroki widok, burza HEAVY, 20 klatek rozgrzewki i 150 mierzonych klatek. Obejmuje aktualizację efektów i rzeczywiste wysłanie renderowania raylib, bez aktywnego wyszukiwania. Wyniki z czystego builda bez optymalizacji Release:

| Mapa | Mediana klatki | 95. percentyl |
| --- | --- | --- |
| 20×20 | 4,18 ms | 5,29 ms |
| 50×50 | 5,57 ms | 7,36 ms |
| 80×60 | 10,07 ms | 13,65 ms |

To pomiar tego komputera, nie gwarancja na każdym sprzęcie. A* zachowuje wektorową implementację autora. W Release wygenerowanie mapy 80×60 i synchroniczne znalezienie trasy w teście zajęło około 4,2 ms.

Zachowano culling, limit 384 cząsteczek kurzu, 16000 znaczników śladów oraz stałe pule burzy: 112/512/56 dla bliskiej/średniej/przyziemnej warstwy. Zasoby audio są ładowane jednokrotnie.

## Zakres odbioru

Projekt jest przygotowany do prezentacji na sprawdzonym środowisku UCRT64. QA obejmuje automatyczne scenariusze sterowania przez API, sprzętowy render i kontrolę zrzutów; nie zastępuje subiektywnego odsłuchu na zestawie nagłośnienia sali. Nie dodano zapisu stanu, rankingów ani dodatkowych trybów gry.
