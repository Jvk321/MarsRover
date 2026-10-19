# Rover 3D w świecie 2.5D

## Podział odpowiedzialności

- `src/rover_3d_pose.h/.cpp` przelicza dane `Rover` na pozycję `Vector3`, macierze korpusu i sześciu kół oraz pozycję i azymut anteny. To obliczenia renderera, bez modyfikacji trasy i stanów misji.
- `src/rover_mesh.h/.cpp` buduje jedną siatkę opony, bieżnika, felg, ramion i piast. Sześć kół używa tego samego bufora GPU z różnymi macierzami.
- `src/rover_renderer.h/.cpp` posiada geometrię, materiał, shader i opcjonalny model korpusu. Rysuje rzeczywisty model przez `BeginMode3D` i `DrawMesh`, z testem głębi. Zasoby powstają raz, przy pierwszym rysowaniu; `shutdown()` zwalnia je przed `CloseWindow()`.
- `SceneRenderer` pozostaje kompozytorem świata 2.5D. Wstawia przebieg 3D w dotychczasowe sortowanie obiektów według punktu styku z ziemią. Skały i beacon mogą zasłaniać pojazd; wzajemną widoczność części rovera rozstrzyga bufor głębi 3D.
- `VisualEffects` obserwuje ruch: przechowuje delikatny pitch/bob, ślady i ograniczoną pulę pyłu. PAUSE zamraża animacje, RESET je czyści. Hamowanie dodaje pojedynczy krótki wyrzut kurzu.

Stary renderer kół oparty na ręcznym rzutowaniu i sortowaniu ścian został usunięty. `Rover`, `Simulation`, A*, generator i audio zachowują dotychczasową logikę.

## Układ współrzędnych i kamera

`grid (x,y)` odpowiada `world (x * worldGridScale, height * worldGridScale, y * worldGridScale)`. Oś Y świata jest wysokością wyłącznie wizualną. Przód modelu to +X, góra +Y, prawa strona +Z.

Rover ma kamerę `Camera3D` z `CAMERA_ORTHOGRAPHIC`. Stały azymut 45° i elewacja 30° odwzorowują dotychczasowy rzut diamentu 2:1. Elewacja wynika z proporcji `halfHeight / halfWidth`; nie wymuszamy matematycznej izometrii 35.264°, która zmieniłaby wygląd mapy. Skala i położenie kamery 3D wynikają z bieżącej macierzy kamery świata 2D. Po rysowaniu przywracana jest macierz 2D. Dzięki temu pan, zoom, AUTO CAMERA i arrival zachowują dotychczasowe zachowanie, a UI pozostaje ekranowe.

Koła są walcami z sześcioma pierścieniami profilu, osobnymi bokami, felgami, piastami i 12 wystającymi żebrami. Obrót jest wokół lokalnej osi Z, następnie wykonywany jest skręt i yaw pojazdu. Faza pochodzi z dotychczasowego `distance / wheelRadius`, wraz z różnicą lewo/prawo przy obrocie. Nie jest zerowana między segmentami ani przy replanningu. Koła pozostają na ziemi; subtelne ruchy zawieszenia zachodzą w belkach i punktach mocowania do korpusu.

Ślady wykorzystują ten sam `RoverGeometry::contact` co pozycje kół 3D, z dwoma osobno próbkowanymi łukami tylnych kół. Pył powstaje przy tych samych tylnych kontaktach. Zegar sonaru jest współdzielonym źródłem fazy anteny i dźwięku. Azymut anteny to `scanAngle - pi/4`, co uwzględnia izometryczny rzut wiązki. Sweep wychodzi z rzutu anteny na powierzchnię. Przy MISSION_COMPLETE aktywne skanowanie ustaje.

## Parametry

Długości geometrii są w kaflach. Kąty są w radianach.

| Parametr | Miejsce | Wartość domyślna |
|---|---|---:|
| Rover scale | `RoverGeometry::roverScale`, `src/rover_geometry.h` | 1.0 |
| Wheel radius | `wheelRadius` w tym samym pliku | 0.17 × roverScale |
| Wheel width | `wheelWidth` | 0.14 × roverScale |
| Wheel spacing | `wheelSpacing` (środek–przód/tył) | 0.43 × roverScale |
| Wheelbase / track width | `wheelbase` / `trackWidth` | 0.86 / 0.98 × roverScale |
| Suspension amount | `suspensionAmount` | 0.012 |
| Suspension offset | `suspensionOffset` | 0.04 × roverScale |
| Steering angle | `steeringAngle` / `rearSteeringRatio` | przód 0.38 (~21.8°), tył przeciwnie × 0.55 |
| Rotation speed kół | istniejące `Rover::update`, `src/rover.cpp` | faktyczny dystans / wheelRadius, bez osobnego mnożnika |
| Rotation speed korpusu | istniejące `turnDuration`, początek `src/rover.cpp` | obrót przez 0.45 s |
| Antenna speed | `SonarConfig`, `src/sonar.h` | SEARCHING 3.4 s/obrót, RECALCULATING 2.7 s, DRIVING 7.2 s |
| Body bob | `Rover3DConfig::bodyBob`, `src/rover_3d_pose.h` | 0.005 |
| Pitch / wygładzanie | `bodyPitch` / `animationResponse` | limit 0.018 rad / 7 |
| World/grid scale | `Rover3DConfig::worldGridScale` | 1 jednostka 3D na kafel |
| Bieżnik / obwód | `treadSegmentCount` / `wheelSegments` | 12 / 16 |
| Wyrzut pyłu przy hamowaniu | `Rover3DConfig::brakeDustBurst` | 6, w ramach istniejącej puli |

Shader w `rover_renderer.cpp` zawiera stały kierunek ciepłego światła, światło otoczenia i delikatny rim light. Nie korzysta z PBR ani dodatkowych tekstur. Cienie pozostają lekkimi miękkimi plamami na gruncie, obliczanymi z pozycji modelu i jego kół.

## Opcjonalny GLB / GLTF

`Rover3DConfig::bodyAsset` wskazuje `assets/models/rover_body.glb`. Można zmienić rozszerzenie na `.gltf`. Plik jest sprawdzany i ładowany tylko raz. Brak pliku oznacza proceduralny korpus; nieudany import również zachowuje fallback.

Obecny punkt podmiany dotyczy korpusu: model powinien mieć środek na poziomie gruntu, przód +X i górę +Y, rozmiar korpusu około 1 × 0.6 kafla, wysokość około 0.3–0.73. Nie powinien zawierać dodatkowych kół i sensorów, bo są animowane osobno. Tekstury materiałów są obsługiwane. Późniejszy model całego pojazdu z riggiem należy podłączyć w `RoverRenderer::body/draw`; nie wymaga to zmian w `Rover` ani `Simulation`.

## Weryfikacja

Test `spatial_rover_wheels` sprawdza sześć macierzy w czterech kierunkach, właściwą oś obrotu, kontakt z gruntem, prześwit od korpusu, obroty zależne od dystansu przy 0.25 i 5 kaflach/s, skręt obu znaków, ciągłość fazy po zmianie trasy, zgodność projekcji 2D/3D przy kilku rozdzielczościach i zoomach, azymut anteny, PAUSE i RESET.

`visual_smoke` sprawdza prawdziwy przebieg GPU, w tym misję na 80×60, PATH_REVEAL → DRIVING, DROP ROCK → RECALCULATING, kontynuację jazdy, MISSION_COMPLETE, sonar, efekty, kamerę ręczną i AUTO, głębię przy skałach oraz cztery orientacje i skręcanie. Zrzuty i pomiary trafiają do podanego katalogu:

```powershell
.\build\rover3d-clean\visual_smoke.exe build/rover3d-qa
```
