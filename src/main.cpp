#include "astar.h"
#include "raylib.h"
#include <algorithm>

int main() {
    Grid grid(24, 14);
    for (int y = 2; y < grid.height() - 2; ++y)
        if (y != 8) grid.setBlocked({grid.width() / 2, y});
    const Cell start{1, 1}, goal{grid.width() - 2, grid.height() - 2};
    const auto path = findPath(grid, start, goal);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 800, "Mars Rover A* Navigation Simulator 2.5D");
    if (!IsWindowReady()) return 1;
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        const float sum = static_cast<float>(grid.width()) + grid.height();
        const float tileWidth = std::min((GetScreenWidth() - 80.0f) * 2 / sum,
                                        (GetScreenHeight() - 140.0f) * 4 / sum);
        const float halfW = tileWidth / 2, halfH = tileWidth / 4;
        const Vector2 origin{GetScreenWidth() / 2.0f - (grid.width() - grid.height()) * halfW / 2,
                             GetScreenHeight() / 2.0f - sum * halfH / 2 + 25};
        auto project = [=](int x, int y) {
            return Vector2{origin.x + (x - y) * halfW, origin.y + (x + y) * halfH};
        };
        BeginDrawing();
        ClearBackground({28, 24, 25, 255});
        DrawText("Mars Rover - A*", 24, 20, 24, RAYWHITE);
        DrawText("Green: start | Blue: goal | Gold: path | Dark: obstacle", 24, 52, 18, LIGHTGRAY);
        for (int y = 0; y < grid.height(); ++y) {
            for (int x = 0; x < grid.width(); ++x) {
                const Cell cell{x,y};
                Color color = grid.blocked(cell) ? Color{70, 54, 49, 255} : Color{161, 94, 64, 255};
                if (std::find(path.begin(), path.end(), cell) != path.end()) color = GOLD;
                if (cell == start) color = GREEN;
                if (cell == goal) color = SKYBLUE;
                Vector2 top = project(x,y), right = project(x+1,y);
                Vector2 bottom = project(x+1,y+1), left = project(x,y+1);
                DrawTriangle(top, left, bottom, color);
                DrawTriangle(top, bottom, right, color);
                const Color edge{55, 39, 34, 255};
                DrawLineV(top, right, edge); DrawLineV(right, bottom, edge);
                DrawLineV(bottom, left, edge); DrawLineV(left, top, edge);
            }
        }
        if (path.empty()) DrawText("No route found", 24, 78, 18, RED);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
