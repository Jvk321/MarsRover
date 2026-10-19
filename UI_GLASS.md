# Liquid Glass — Mission HUD

## Układ

HUD znajduje się po prawej stronie. W 1920×1080 ma 410 px szerokości, a w 2560×1440 około 473 px. Obszar kadrowania świata zajmuje odpowiednio około 77% i 80% szerokości. Panel STATUS, CURRENT NODE, g/h/f, OPEN/CLOSED/ITERATIONS i telemetria misji są zawsze widoczne.

Ustawienia są w zakładkach MAP, CAMERA, FX i AUDIO. Przy niższym oknie zawartość ustawień można przewijać kółkiem; kluczowa telemetria pozostaje nieruchoma. Dolny pływający pasek zawiera PLAY, PAUSE, STEP, RESET oraz prędkości wyszukiwania i jazdy. W edycji pojawia się osobny pasek narzędzi. Minimapę przeniesiono do własnego szklanego panelu.

## Moduły

- `src/ui_glass.h/.cpp` — paleta, układ responsywny, font, RenderTexture świata, shader szkła, ikony i ograniczona tablica animacji.
- `src/ui_controls.h/.cpp` — wspólne przyciski, suwaki i animowane przełączniki.
- `src/mission_hud.h/.cpp` — telemetria, zakładki i zdarzenia istniejących kontrolek.
- `src/mission_feedback.cpp` — wygląd pomocy, miksera, presetów, alertów i podsumowania; obliczanie czasu, dystansu i iteracji pozostaje dotychczasowe.
- `src/terrain_panel.cpp` — nowy wygląd formularza; zasady walidacji i generator pozostają dotychczasowe.
- `SceneRenderer::drawMinimap` — prezentacja mapy, rovera, celu, aktywnej trasy i granic kamery.

`main.cpp` zbiera zdarzenia kontrolek przed aktualizacją symulacji, więc PAUSE i STEP zachowują swoje działanie. Mapowanie ekranu na mapę pomija interaktywne panele. A*, Rover, Simulation, Terrain, WorldCamera, audio oraz efekty środowiskowe nie zostały przebudowane.

## Szkło i wydajność

Świat trafia do jednej ponownie używanej RenderTexture o rozmiarze klienta okna. Tekstura jest odtwarzana na ekranie. Shader jest wykonywany jedynie na powierzchniach paneli; nie rozmywa całego ekranu. Uwzględnia zaokrągloną maskę, lokalne przesunięcie obrazu na krawędzi, desaturację, gradient, drobny statyczny grain i jasny górny rant. Cienie są lekkimi warstwami rysowanymi pod panelem.

W FX znajduje się **UI GLASS QUALITY**:

- **HIGH** — 9 próbek tła na piksel szkła, delikatny blur.
- **LOW** — pojedyncza próbka, zachowany tint, gradient, grain i krawędzie.

Nie powstają dodatkowe pełnoekranowe tekstury blur. RenderTexture zmienia się tylko przy zmianie rozmiaru okna. Font i shader są ładowane raz i zwalniane przed zamknięciem okna. Animacje komponentów korzystają ze stałej tablicy 512 wpisów. Jeśli shader jest niedostępny, działa prosty wariant rysowany figurami.

## Parametry

`Ui::Theme` w `src/ui_glass.h` zawiera kolory, promienie, `blurRadius` (2.6 px), `refraction` (1.25 px) i `transition` (0.22 s). Liczby te trafiają do shadera przez uniformy. Konstrukcję efektu można zmienić w `glassFragment` w `src/ui_glass.cpp`.

`Ui::layout` w tym samym pliku wyznacza skalę `sqrt(height / 1080)`, ograniczoną do 0.84–1.25, oraz szerokości, odstępy i położenie paneli. Wymiary i input korzystają z jednej przestrzeni natywnych pikseli klienta; nie są ponownie mnożone przez skalę monitora. Unika to podwójnego skalowania UI i rozjazdu stref kliknięcia. Rozróżnienie rozmiaru ekranu, framebufferu i skali monitora opisuje implementacja [raylib — obsługa okna i DPI](https://github.com/raysan5/raylib/blob/master/src/platforms/rcore_desktop_glfw.c).

Typografia ma osobne rozmiary tytułów, nagłówków sekcji, głównych wartości i podpisów. Font ma atlas 80 px i filtrowanie bilinear. Kolejność wyboru:

1. `assets/ui/Inter-Regular.ttf` przy aplikacji lub w katalogu projektu.
2. Systemowy `C:/Windows/Fonts/segoeui.ttf` — bez kopiowania fontu do projektu.
3. Domyślny font raylib.

Font można podmienić bez zmiany komponentów UI. Podstawowe skróty pozostają: SPACE, N, R, C, F1, F2, M, WASD, MMB i kółko myszy. FIT TO MAP oddaje kamerę pod kontrolę ręczną i wywołuje istniejący reset kadru; AUTO CAMERA ponownie włącza śledzenie.

## Weryfikacja

`liquid_glass_layout` sprawdza pięć rozmiarów okna, szerokości HUD-u w 1080p i 1440p, granice paneli, brak kolizji elementów, udział świata w ekranie i wyłączenie inputu świata pod kontrolkami.

```powershell
ctest --preset release -j 4
.\build\release\ui_smoke.exe build/ui-qa
```

Test graficzny wymaga rzeczywiście skompilowanego shadera HIGH. Obejmuje 1920×1080 i 2560×1440, SEARCHING, PAUSE, PATH_REVEAL, DRIVING, ROUTE_BLOCKED, RECALCULATING, ręczny STEP, MISSION_COMPLETE, EDIT_MAP, generator, pomoc, audio, presety, kamerę i oba poziomy jakości szkła. Używa istniejącej misji na mapie 80×60 i zapisuje zrzuty oraz pomiary pełnej klatki. Osobny korytarz bez objazdu sprawdza modal NO_ROUTE i stan UI po resecie.

Przez wewnętrzne `PlayAutomationEvent` raylib 6.2 test odtwarza kliknięcia samych widgetów, bez wysyłania zdarzeń do pulpitu Windows. Sprawdza PLAY/PAUSE/STEP/RESET, dokładnie jedną iterację po STEP, wybór zakładek, SOUND, MASTER, SANDSTORM, intensywność, jakość LOW/HIGH, AUTO CAMERA, FIT TO MAP, SHOW COSTS i blokadę inputu przez modal. Mechanizm opisuje [implementacja zdarzeń automatyzacji raylib](https://github.com/raysan5/raylib/blob/master/src/rcore.c).
