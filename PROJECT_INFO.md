# Informacje o projekcie

## Podział odpowiedzialności

| Moduł | Pliki | Odpowiedzialność |
| --- | --- | --- |
| Dane mapy | `astar.h/.cpp`: `Cell`, `Grid` | Wymiary, współrzędne i zajętość pól; bez raylib |
| A* | `astar.h/.cpp`: `AStarSearch` | OPEN, CLOSED, CURRENT, koszty, rodzice, jedna iteracja i odtworzenie ścieżki |
| Generator | `terrain.h/.cpp`: `Terrain` | Deterministyczne formacje, kontrola połączenia, edycja mapy |
| Symulacja | `simulation.h/.cpp`: `Simulation` | Stany, odtwarzanie, reveal, DROP ROCK, ponowna inicjalizacja tego samego A* |
| Łazik | `rover.h/.cpp`: `Rover` | Pozycja ciągła, obrót, przechył, hamowanie i segment trasy |
| Kamera | `world_camera.h/.cpp`: `WorldCamera` | Projekcja izometryczna, screen-to-grid, zoom, przesuwanie, śledzenie i arrival |
| Renderowanie | `scene_renderer.*`, `rover_renderer.*` | Proceduralny teren, skały, pojazd, beacon, trasa i g/h/f; bez zmiany mapy |
| Efekty | `visual_effects.*`, `sandstorm.*`, `sonar.h` | Ślady, ograniczone pule pyłu, warstwy burzy i zegar sonaru |
| Audio | `audio_system.*`, `drive_audio_synth.*` | Obserwacja stanów, pojedyncze sygnały, ciągły wiatr i napęd, miks i zasoby |
| UI | `main.cpp`, `terrain_panel.*`, `mission_feedback.*`, `ui_controls.h` | Panel sterowania, formularz, pomoc, podsumowanie i wspólne przyciski |
| Presety | `presentation.h` | Trzy konfiguracje zwykłego generatora i prędkości prezentacji |

`main.cpp` łączy moduły i tłumaczy działania użytkownika na wywołania API. Nie zawiera algorytmu wyszukiwania. `Grid` i `AStarSearch` pozostają w jednym niewielkim module; nie wymagają osobnych klas menedżerów. `legacy/astar_original.cpp` to zachowany pierwotny kod autora, wyłączony z kompilacji.

## Przepływ danych

1. `Terrain` tworzy `Grid`, START i GOAL. A* otrzymuje kopię danych, niezależną od renderera.
2. `Simulation::step()` rozwija jeden węzeł, odsłania jeden segment albo przesuwa ruch o 1/60 s. PLAY wywołuje ten sam mechanizm według zegara symulacji.
3. A* zachowuje rodzica każdego rozpatrzonego pola. Po znalezieniu GOAL odtwarza uporządkowaną ścieżkę. Symulacja przechowuje liczbę odsłoniętych segmentów.
4. `Rover` interpoluje pozycję na tej ścieżce w współrzędnych siatki. Kamera i renderer używają projekcji `screenX = (x-y)*32`, `screenY = (x+y)*16`.
5. DROP ROCK aktualizuje mapę symulacji i mapę wizualną. Łazik hamuje do następnej bezpiecznej kratki; nowe `AStarSearch` zaczyna od niej, bez przenoszenia pojazdu do pierwotnego START.
6. Efekty, audio i telemetria obserwują stan po aktualizacji. Jednorazowe sygnały reagują na przejścia stanów, nie na każdą klatkę.
7. Aplikacja zachowuje początkowy `Terrain` misji. RESET/REPLAY odtwarzają tę kopię; edycja lub generowanie ustanawiają nową mapę początkową.

`PATH_FOUND` jest stanem A*, nie dodatkową pauzą symulacji. Symulacja przechodzi wtedy do PATH_REVEAL, a UI pokazuje krótki komunikat PATH FOUND. ARRIVAL wykorzystuje istniejącą kamerę i zatrzymany pojazd; po nim pojawia się MISSION_COMPLETE.

## Jak wyjaśnić algorytm na prezentacji

Włącz LAB DEMO, zatrzymaj SEARCHING i wykonaj kilka STEP. Pokaż CURRENT, OPEN i CLOSED oraz `f = g + h` w panelu. Zbliż kamerę, włącz SHOW COSTS i porównaj koszt sąsiednich pól. Wznów odtwarzanie, zaczekaj na reveal i jazdę, a następnie użyj DEMO BLOCK ROUTE. W RECALCULATING pokaż nowy początek wyszukiwania przy zatrzymanym łaziku oraz przygaszoną starą trasę. Na końcu porównaj liczbę replanningów, iteracje i przebyty dystans.

Pełne sterowanie i polecenia kompilacji są w [README.md](README.md).

## Wydajność i ograniczenia

Renderer odrzuca obiekty spoza widoku, ogranicza szczegóły przy oddaleniu i ponownie wykorzystuje bufory stanów/kosztów pól. Etykiety g/h/f pojawiają się dopiero przy czytelnym zoomie. Burza używa stałych pul; kurz i ślady mają limity. Audio jest ładowane raz, a syntezatory zachowują stan między blokami próbek.

OPEN i CLOSED w oryginalnej implementacji są wektorami, a nie kolejką priorytetową. Przeszukiwanie tych wektorów może być droższe dla bardzo dużych, zablokowanych map. Zachowano algorytm autora; limit formularza 512×512 chroni wejście, ale nie jest obietnicą 60 FPS przy skrajnym przypadku. Rozmiary prezentacyjne i QA: 20×20, 50×50, 80×60.

Czasy wyszukiwania w UI mierzą aktywne odtwarzanie wyszukiwania, łącznie z regulowanymi odstępami między iteracjami. Nie służą do porównywania wydajności algorytmów; pomiary CPU/GPU są raportowane osobno przez narzędzia QA.
