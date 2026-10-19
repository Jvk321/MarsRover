# Visual Match Pass — 19.10.2026

Kierunek graficzny: ciepły czerwono-pomarańczowy Mars, chłodne projekcje nawigacyjne, biały sześciokołowy rover i złota trasa. Grafika powstaje z kształtów raylib; pliki PNG nie są wymagane. A*, generator, replanning, ruch i skróty sterowania zachowują dotychczasowe działanie.

## Teren, skały i światło

`terrain_art.cpp` rysuje regolit z łagodnym szumem przestrzennym, przebarwieniami, kamykami, pęknięciami i smugami piasku. Warianty zależą od współrzędnych i seedu, bez losowania w kolejnych klatkach. Siatka jest ledwo widoczna podczas jazdy i wyraźniejsza w SEARCHING, RECALCULATING, SHOW COSTS oraz EDIT MAP.

Izolowane przeszkody mają warianty głazów, małych grup i kraterów. Pole mające przynajmniej dwóch sąsiadów będących przeszkodami może należeć do klifu. Sąsiednie części klifu dzielą te same wierzchołki oraz wysokości; wewnętrzne ściany nie są rysowane. Krawędzie są nieregularne, a powierzchnie mają kilka odcieni i szczeliny. Wszystko nadal oznacza zwykłe pola obstacle w A*.

Przednie krawędzie mapy mają skalny przekrój z warstwami, pęknięciami i pojedynczymi filarami. Dla bardzo małych map wysokość przekroju jest zmniejszana, żeby dekoracja nie dominowała nad mapą. Cienie są rzucane w jednym kierunku, a górne powierzchnie i wybrane krawędzie otrzymują ciepłe światło.

## Rover, baza i cel

`rover_renderer.cpp` rysuje jasny korpus, ciemne podwozie, sześć kół z bieżnikiem i obracającymi się piastami, ramiona zawieszenia, maszt z dwoma obiektywami, antenę i przednie światła. Wykorzystuje istniejące heading, tilt i wheelAngle z `Rover`; nie oblicza ruchu.

`world_art.cpp` rysuje moduły mieszkalne, kopułę, antenę, panele słoneczne, skrzynie i lampy. Placówki znajdują się poza logiczną siatką, na połączonych z płaskowyżem dekoracyjnych tarasach. Liczba placówek jest ograniczona do trzech; bardzo małe lub wąskie mapy ich nie mają.

START ma zielony holograficzny pad. GOAL to mały nadajnik, unoszący się pomarańczowy sześcian, wąska wiązka i pulsujące pierścienie. Efekt nasila się przy ARRIVAL/MISSION_COMPLETE. Tło zawiera warstwy odległych płaskowyżów, światła bazy i duży sierp planety; warstwy zachowują parallax i reagują na zapylenie.

## Depth sorting i kamera

`SceneRenderer` buduje ponownie wykorzystywaną listę widocznych obiektów i sortuje ją według ekranowego Y punktu styku z gruntem: `(x + y) * 16`. Rover korzysta z ciągłej pozycji, bez zaokrąglania do kratki. Skały, dekoracje i beacon uczestniczą w tym samym sortowaniu. Równe głębokości mają deterministyczne reguły kolejności. Cienie i ślady są rysowane przed obiektami.

W AUTO CAMERA zwiększono zbliżenie podczas jazdy, przestrzeń przed pojazdem i nieco obniżono jego pozycję w kadrze. FIT TO MAP uwzględnia wizualne urwisko. Projekcja 2:1, screen-to-grid, płynne śledzenie i ręczne sterowanie pozostały zachowane.

## Nawigacja, HUD i minimapa

OPEN ma cyjanowy obrys, miękki glow i pulsowanie; CLOSED — słabszą niebiesko-fioletową projekcję. CURRENT ma jasny pulsujący obrys. Aktywna trasa otrzymała ciepły glow, jasny rdzeń, strzałki i przemieszczające się światło. Renderer i minimapa respektują liczbę odkrytych segmentów PATH_REVEAL; stara trasa po DROP ROCK pozostaje przygaszona.

HUD zachowuje układ i funkcje. Zastosowano ciemny granat, delikatniejsze wypełnienia, cienkie obramowania i pomarańczowe akcenty. Minimapę rysuje `SceneRenderer::minimap`; pokazuje przeszkody, OPEN/CLOSED/CURRENT, START, GOAL, odsłoniętą trasę, rover i obszar kamery. Jest tylko odczytem — kliknięcie lub zoom nad nią nie edytuje mapy ani nie przesuwa kamery świata.

## Parametry do strojenia

W `src/visual_config.h`, w przestrzeni `VisualConfig`:

| Parametry | Znaczenie |
| --- | --- |
| `regolith`, `rockTop`, `rockShade`, `sunRim` | Paleta gruntu, światła i ścian skał |
| `shadowDirection`, `shadowLength`, `shadowAlpha` | Kierunek, długość i siła cieni |
| `rimAlpha` | Jasność oświetlonych krawędzi skał |
| `rockHeight`, `cliffHeight`, `cliffVariation`, `plateauDepth` | Wysokości obiektów i skalnego przekroju mapy |
| `gridAlpha`, `activeGridAlpha` | Siatka normalna i podczas pracy z algorytmem/edytorem |
| `openAlpha`, `closedAlpha`, `openGlow` | Warstwy OPEN/CLOSED i poświata OPEN |
| `gold`, `pathCore`, `pathGlow`, `pathFlowSpeed` | Kolor, glow i przepływ aktywnej trasy |
| `beaconGlow`, `beaconSignalSpeed` | Poświata i pierścienie celu |
| `detailZoomThreshold`, `terrainDetailCount` | Automatyczne LOD i detale gruntu |

Kadrowanie: `CameraFraming` w `src/world_camera.h` (`followZoom`, `lookAhead`, `screenBias`, `fitPadding`, `skirtSpace`). Parametry burzy i sonaru pozostają w `sandstorm.h` i `sonar.h`.

## Gdzie podłączyć PNG

- `TerrainArt::ground` — warianty podłoża.
- `TerrainArt::obstacle` — głazy, kratery i części formacji, z zachowaniem sąsiedztwa klifów.
- Rover korzysta teraz z modelu 3D; punkt podmiany GLB/GLTF opisano w `ROVER_3D.md`.
- `WorldArt::prop` — moduły bazy i pozostałe dekoracje.
- `WorldArt::beacon` — nadajnik; hologram może pozostać proceduralny.
- `WorldArt::background` — dalsze warstwy tła.

Pliki można umieścić w `assets/textures/`. Przyszłe sprite'y powinny używać punktu styku z gruntem jako kotwicy; wówczas zachowają istniejący depth sorting. Tekstury należy ładować raz i zwalniać przed zamknięciem okna.

## Weryfikacja i wydajność

Każdy z trzech etapów został skompilowany. Pełny zestaw CTest: 11/11. Po zmianie grupowania rysowania powtórzono testy kamery, edycji i efektów.

Test renderowania obejmuje SEARCHING, koszty, PATH_REVEAL, DRIVING, skręt, DROP ROCK, RECALCULATING, ARRIVAL/MISSION_COMPLETE, burzę LIGHT/MEDIUM/HEAVY, ręczną kamerę, edycję oraz dwie sceny zasłaniania rovera przez te same skały. Zrzuty: `build/visual-match/`; `15a-depth-behind.png` i `15b-depth-front.png` pokazują kolejność obiektów.

Pomiar Release, AMD Radeon 780M, 1280×800, szeroki widok i HEAVY, 150 klatek po rozgrzewce:

| Mapa | Mediana | 95. percentyl |
| --- | --- | --- |
| 20×20 | 7,42 ms | 11,17 ms |
| 50×50 | 3,28 ms | 3,87 ms |
| 80×60 | 6,13 ms | 7,21 ms |

20×20 ma pełniejsze detale przy większym zoomie, stąd większy koszt niż 50×50. Ograniczono przełączanie trybów rysowania: powierzchnie i linie siatki są osobnymi przebiegami, a siatka używa długich linii zamiast czterech odcinków dla każdego kafla. Przy oddaleniu znika drobna faktura i część krawędzi, pozostają główne bryły i oświetlenie. Zachowano culling, stałe pule burzy, limity kurzu i śladów oraz ponownie wykorzystywane bufory renderera.
