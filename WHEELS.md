# Koła rovera

Koła zostały przeniesione do prawdziwego renderera 3D. Nie używają już ręcznego rzutowania ścian na ekran. Siatkę tworzy `src/rover_mesh.cpp`, macierze sześciu kół wylicza `src/rover_3d_pose.cpp`, a rysowanie GPU i bufor głębi obsługuje `src/rover_renderer.cpp`.

Parametry pozostają w `src/rover_geometry.h`: promień 0.17, szerokość 0.14, rozstaw osi przód–tył 0.86, rozstaw lewo–prawo 0.98, skręt przodu 0.38 rad, skręt tyłu przeciwny × 0.55, 12 żeber bieżnika i odsunięcie zwrotnicy 0.04. Długości skaluje `roverScale`, domyślnie 1.

Pełna dokumentacja modelu, obrotu, kontaktów, śladów, kamery i testów: [ROVER_3D.md](ROVER_3D.md).
