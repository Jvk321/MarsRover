#include "astar.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace {
const char* statusName(SearchStatus status) {
    switch (status) {
    case SearchStatus::READY: return "READY";
    case SearchStatus::SEARCHING: return "SEARCHING";
    case SearchStatus::PATH_FOUND: return "PATH_FOUND";
    case SearchStatus::NO_PATH: return "NO_PATH";
    case SearchStatus::INVALID_ENDPOINTS: return "INVALID_ENDPOINTS";
    }
    return "UNKNOWN";
}
bool active(const AStarSearch& search) {
    return search.status() == SearchStatus::READY || search.status() == SearchStatus::SEARCHING;
}
bool button(Rectangle rect, const char* label, bool enabled = true) {
    bool hover = enabled && CheckCollisionPointRec(GetMousePosition(), rect);
    DrawRectangleRec(rect, !enabled ? Color{40,40,45,255} : hover ? Color{85,85,105,255} : Color{58,58,73,255});
    DrawText(label, static_cast<int>(rect.x + 10), static_cast<int>(rect.y + 11), 18, enabled ? RAYWHITE : GRAY);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
}
int main() {
    Grid grid(24, 14);
    for (int y = 2; y < grid.height() - 2; ++y)
        if (y != 8) grid.setBlocked({grid.width() / 2, y});
    const Cell start{1,1}, goal{grid.width()-2,grid.height()-2};
    AStarSearch search(grid, start, goal);
    bool playing = false;
    float speed = 8.0f;
    double elapsed = 0;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 800, "Mars Rover A* Navigation Simulator 2.5D");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(920, 600);
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground({28,24,25,255});
        DrawRectangle(0,0,300,GetScreenHeight(),{32,31,40,255});
        DrawText("A* NAVIGATION",20,20,24,RAYWHITE);
        // Process controls before the playback tick: PAUSE/RESET take effect immediately.
        bool play = button({20,65,120,42},"PLAY",active(search));
        bool pause = button({152,65,120,42},"PAUSE",playing);
        bool step = button({20,117,120,42},"STEP",active(search));
        bool reset = button({152,117,120,42},"RESET");
        play = play || (IsKeyPressed(KEY_SPACE) && !playing);
        pause = pause || (IsKeyPressed(KEY_SPACE) && playing);
        step = step || IsKeyPressed(KEY_N);
        reset = reset || IsKeyPressed(KEY_R);
        if (reset) { search.reset(); playing = false; elapsed = 0; }
        else if (step) { playing = false; elapsed = 0; search.step(); }
        else if (pause) { playing = false; elapsed = 0; }
        else if (play && active(search)) { playing = true; elapsed = 0; }
        DrawText(TextFormat("SPEED: %.0f iterations/s",speed),20,180,18,RAYWHITE);
        if (button({20,207,40,36},"-")) speed = std::max(1.0f,speed-1);
        if (button({232,207,40,36},"+")) speed = std::min(60.0f,speed+1);
        Rectangle track{76,220,140,10};
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),{70,207,152,36})) {
            float fraction = std::clamp((GetMouseX()-track.x)/track.width,0.0f,1.0f);
            speed = std::round(1 + fraction*59);
        }
        DrawRectangleRec(track,GRAY);
        DrawCircle(static_cast<int>(track.x + (speed-1)/59*track.width),225,7,SKYBLUE);
        if (playing && active(search)) {
            elapsed += GetFrameTime();
            const double interval = 1.0 / speed;
            while (elapsed >= interval && active(search)) { search.step(); elapsed -= interval; }
        }
        if (!active(search)) { playing = false; elapsed = 0; }
        DrawText(TextFormat("STATUS: %s",statusName(search.status())),20,270,18,RAYWHITE);
        DrawText(playing ? "PLAYING" : "PAUSED",20,298,18,LIGHTGRAY);
        if (search.current()) {
            const auto& n = *search.current();
            DrawText(TextFormat("CURRENT x/y: %d / %d",n.cell.x,n.cell.y),20,334,18,RAYWHITE);
            DrawText(TextFormat("g: %d   h: %.2f",n.g,n.h),20,362,18,RAYWHITE);
            DrawText(TextFormat("f: %.2f",n.f),20,390,18,RAYWHITE);
        } else {
            DrawText("CURRENT x/y: - / -",20,334,18,RAYWHITE);
            DrawText("g: -   h: -   f: -",20,362,18,RAYWHITE);
        }
        DrawText(TextFormat("OPEN: %d",static_cast<int>(search.open().size())),20,430,18,SKYBLUE);
        DrawText(TextFormat("CLOSED: %d",static_cast<int>(search.closed().size())),20,458,18,Color{170,130,210,255});
        DrawText(TextFormat("ITERATIONS: %d",static_cast<int>(search.iterations())),20,486,18,RAYWHITE);
        DrawText("Space: play/pause\nN: step   R: reset\nCLOSED includes CURRENT",20,530,16,LIGHTGRAY);

        const float sum = static_cast<float>(grid.width()) + grid.height();
        const float availableWidth = GetScreenWidth()-340.0f;
        const float availableHeight = GetScreenHeight()-130.0f;
        const float tileWidth = std::min(availableWidth*2/sum,availableHeight*4/sum);
        const float halfW = tileWidth/2, halfH = tileWidth/4;
        const Vector2 origin{300 + (GetScreenWidth()-300)/2.0f - (grid.width()-grid.height())*halfW/2,
                             90 + availableHeight/2 - sum*halfH/2};
        auto project = [=](int x,int y) {
            return Vector2{origin.x+(x-y)*halfW,origin.y+(x+y)*halfH};
        };
        DrawText("OPEN: blue | CLOSED: dark purple | CURRENT: bright outline",320,20,17,LIGHTGRAY);
        DrawText("S: start | G: goal | Path appears only after PATH_FOUND",320,46,17,LIGHTGRAY);
        for (int y=0;y<grid.height();++y) for (int x=0;x<grid.width();++x) {
            Cell cell{x,y};
            Color color = grid.blocked(cell) ? Color{65,49,45,255} : Color{161,94,64,255};
            CellState state = search.state(cell);
            if (state == CellState::OPEN) color = Color{45,115,210,255};
            if (state == CellState::CLOSED || state == CellState::CURRENT) color = Color{60,32,85,255};
            if (search.status() == SearchStatus::PATH_FOUND &&
                std::find(search.path().begin(),search.path().end(),cell) != search.path().end()) color = GOLD;
            Vector2 top=project(x,y), right=project(x+1,y), bottom=project(x+1,y+1), left=project(x,y+1);
            DrawTriangle(top,left,bottom,color); DrawTriangle(top,bottom,right,color);
            Color edge{55,39,34,255};
            DrawLineV(top,right,edge); DrawLineV(right,bottom,edge);
            DrawLineV(bottom,left,edge); DrawLineV(left,top,edge);
            if (cell == start || cell == goal) {
                const char* label = cell == start ? "S" : "G";
                DrawText(label,static_cast<int>((top.x+bottom.x)/2-4),static_cast<int>((top.y+bottom.y)/2-7),14,RAYWHITE);
            }
        }
        // Draw CURRENT last so adjacent tile edges cannot cover its outline.
        if (search.current()) {
            Cell c=search.current()->cell;
            Vector2 corners[]{project(c.x,c.y),project(c.x+1,c.y),project(c.x+1,c.y+1),project(c.x,c.y+1)};
            float pulse=static_cast<float>((std::sin(GetTime()*5)+1)/2);
            Color bright{255,255,static_cast<unsigned char>(180+75*pulse),static_cast<unsigned char>(160+95*pulse)};
            for (int i=0;i<4;++i) DrawLineEx(corners[i],corners[(i+1)%4],2+2*pulse,bright);
        }
        EndDrawing();
    }
    CloseWindow();
}
