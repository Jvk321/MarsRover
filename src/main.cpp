#include "simulation.h"
#include "rover_renderer.h"
#include "world_camera.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace {
const char* statusName(SimulationStatus status) {
    switch (status) {
    case SimulationStatus::READY: return "READY";
    case SimulationStatus::SEARCHING: return "SEARCHING";
    case SimulationStatus::PATH_REVEAL: return "REVEALING PATH...";
    case SimulationStatus::PATH_READY: return "PATH_READY";
    case SimulationStatus::DRIVING: return "DRIVING";
    case SimulationStatus::ARRIVAL: return "CINEMATIC ARRIVAL";
    case SimulationStatus::MISSION_COMPLETE: return "MISSION COMPLETE";
    case SimulationStatus::NO_PATH: return "NO_PATH";
    case SimulationStatus::INVALID_ENDPOINTS: return "INVALID_ENDPOINTS";
    }
    return "UNKNOWN";
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
    Simulation simulation(grid, start, goal);
    const auto& search = simulation.search();

    float speed = 8.0f;
    float drivingSpeed = 1.5f;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 960, "Mars Rover A* Navigation Simulator 2.5D");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(1000, 940);
    SetTargetFPS(60);
    WorldCamera worldCamera(grid.width(),grid.height(),{300,80,980,880});
    std::size_t cameraPathSize = 0;
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground({28,24,25,255});
        DrawRectangle(0,0,300,GetScreenHeight(),{32,31,40,255});
        DrawText("A* NAVIGATION",20,20,24,RAYWHITE);
        // Process controls before the playback tick: PAUSE/RESET take effect immediately.
        bool play = button({20,65,120,42},"PLAY",simulation.active());
        bool pause = button({152,65,120,42},"PAUSE",simulation.playing());
        bool step = button({20,117,120,42},"STEP",simulation.active());
        bool reset = button({152,117,120,42},"RESET");
        play = play || (IsKeyPressed(KEY_SPACE) && !simulation.playing());
        pause = pause || (IsKeyPressed(KEY_SPACE) && simulation.playing());
        step = step || IsKeyPressed(KEY_N);
        reset = reset || IsKeyPressed(KEY_R);
        if (reset) simulation.reset();
        else if (step) simulation.step();
        else if (pause) simulation.pause();
        else if (play) simulation.play();
        DrawText(TextFormat("SPEED: %.0f steps/s",speed),20,180,18,RAYWHITE);
        if (button({20,207,40,36},"-")) speed = std::max(1.0f,speed-1);
        if (button({232,207,40,36},"+")) speed = std::min(60.0f,speed+1);
        Rectangle track{76,220,140,10};
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),{70,207,152,36})) {
            float fraction = std::clamp((GetMouseX()-track.x)/track.width,0.0f,1.0f);
            speed = std::round(1 + fraction*59);
        }
        DrawRectangleRec(track,GRAY);
        DrawCircle(static_cast<int>(track.x + (speed-1)/59*track.width),225,7,SKYBLUE);
        simulation.setSpeed(speed);
        DrawText(TextFormat("DRIVE: %.2f tiles/s",drivingSpeed),20,590,18,RAYWHITE);
        if (button({20,620,40,36},"-")) drivingSpeed = std::max(0.25f,drivingSpeed-0.25f);
        if (button({232,620,40,36},"+")) drivingSpeed = std::min(5.0f,drivingSpeed+0.25f);
        Rectangle driveTrack{76,633,140,10};
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),{70,620,152,36}))
            drivingSpeed = 0.25f + 4.75f*std::clamp((GetMouseX()-driveTrack.x)/driveTrack.width,0.0f,1.0f);
        DrawRectangleRec(driveTrack,GRAY);
        DrawCircle(static_cast<int>(driveTrack.x+(drivingSpeed-0.25f)/4.75f*driveTrack.width),638,7,ORANGE);
        simulation.setDrivingSpeed(drivingSpeed);
        simulation.update(GetFrameTime());
        DrawText(TextFormat("STATUS: %s",statusName(simulation.status())),20,270,17,RAYWHITE);
        DrawText(simulation.playing() ? "PLAYING" : "PAUSED",20,298,18,LIGHTGRAY);
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
        DrawText(TextFormat("PATH LENGTH: %d",static_cast<int>(simulation.pathLength())),20,520,18,GOLD);
        DrawText(TextFormat("REVEALED: %d / %d",static_cast<int>(simulation.revealed()),
                            static_cast<int>(simulation.pathLength())),20,548,18,GOLD);
        const auto& rover = simulation.rover();
        if (rover.available()) {
            DrawText(TextFormat("ROVER x/y: %.2f / %.2f",rover.x(),rover.y()),20,680,18,RAYWHITE);
            const auto segment = std::min(rover.segment()+1,simulation.pathLength());
            DrawText(TextFormat("SEGMENT: %d / %d",static_cast<int>(segment),static_cast<int>(simulation.pathLength())),20,710,18,RAYWHITE);
            DrawText(TextFormat("TRAVELLED: %.2f / %d",rover.travelled(),static_cast<int>(simulation.pathLength())),20,740,18,GOLD);
        } else DrawText("ROVER: waiting for path",20,680,18,LIGHTGRAY);
        DrawText("Space: play/pause\nN: step (drive: 1/60 s)\nR: reset\nPath length = segments",20,790,16,LIGHTGRAY);
        if (button({20,880,252,38},worldCamera.automatic() ? "AUTO CAMERA: ON" : "AUTO CAMERA: OFF"))
            worldCamera.setAuto(!worldCamera.automatic());
        const Rectangle viewport{300,80,static_cast<float>(GetScreenWidth()-300),static_cast<float>(GetScreenHeight()-80)};
        worldCamera.resize(viewport);
        if (reset) { worldCamera.reset(); cameraPathSize = 0; }
        if (search.path().size()!=cameraPathSize) {
            std::vector<Vector2> points;
            for (Cell c:search.path()) points.push_back(WorldCamera::project(c.x+0.5,c.y+0.5));
            worldCamera.setPath(points); cameraPathSize=points.size();
        }
        CameraPhase phase=CameraPhase::WIDE;
        if (simulation.status()==SimulationStatus::PATH_REVEAL || simulation.status()==SimulationStatus::PATH_READY)
            phase=CameraPhase::REVEAL;
        if (simulation.status()==SimulationStatus::DRIVING) phase=CameraPhase::FOLLOW;
        if (simulation.status()==SimulationStatus::ARRIVAL) phase=CameraPhase::ARRIVAL;
        if (simulation.status()==SimulationStatus::MISSION_COMPLETE) phase=CameraPhase::COMPLETE;
        CameraInput cameraInput;
        cameraInput.worldHovered=CheckCollisionPointRec(GetMousePosition(),viewport);
        cameraInput.wheel=GetMouseWheelMove();
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) cameraInput.drag=GetMouseDelta();
        cameraInput.keys={static_cast<float>(IsKeyDown(KEY_D)-IsKeyDown(KEY_A)),
                          static_cast<float>(IsKeyDown(KEY_S)-IsKeyDown(KEY_W))};
        worldCamera.update(GetFrameTime(),phase,WorldCamera::project(rover.x()+0.5,rover.y()+0.5),rover.heading(),
                           simulation.playing() || simulation.status()==SimulationStatus::READY,cameraInput);
        constexpr float halfW=32,halfH=16;
        const Vector2 origin{0,0};
        auto project = [=](int x,int y) {
            return Vector2{origin.x+(x-y)*halfW,origin.y+(x+y)*halfH};
        };
        DrawText("OPEN: blue | CLOSED: dark purple | CURRENT: bright outline",320,20,17,LIGHTGRAY);
        DrawText("S: start | G: goal | Gold: revealed path from start to goal",320,46,17,LIGHTGRAY);
        DrawText("Wheel: zoom | MMB / WASD: pan (manual camera)",320,GetScreenHeight()-24,16,LIGHTGRAY);
        BeginScissorMode(300,80,GetScreenWidth()-300,GetScreenHeight()-110);
        BeginMode2D(worldCamera.view());
        for (int y=0;y<grid.height();++y) for (int x=0;x<grid.width();++x) {
            Cell cell{x,y};
            Color color = grid.blocked(cell) ? Color{65,49,45,255} : Color{161,94,64,255};
            CellState state = search.state(cell);
            if (state == CellState::OPEN) color = Color{45,115,210,255};
            if (state == CellState::CLOSED || state == CellState::CURRENT) color = Color{60,32,85,255};
            if (simulation.visible(cell)) color = Color{255,218,55,255};
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
        const bool revealing = simulation.status() == SimulationStatus::PATH_REVEAL;
        const bool searching = simulation.status() == SimulationStatus::SEARCHING;
        if ((searching && search.current()) || (revealing && simulation.revealed() > 0)) {
            Cell c = revealing ? search.path()[simulation.revealed()] : search.current()->cell;
            Vector2 corners[]{project(c.x,c.y),project(c.x+1,c.y),project(c.x+1,c.y+1),project(c.x,c.y+1)};
            float pulse=static_cast<float>((std::sin(GetTime()*5)+1)/2);
            Color bright{255,255,static_cast<unsigned char>(180+75*pulse),static_cast<unsigned char>(160+95*pulse)};
            for (int i=0;i<4;++i) DrawLineEx(corners[i],corners[(i+1)%4],2+2*pulse,bright);
        }
        drawRover(simulation.rover(),origin,halfW,halfH);
        EndMode2D();
        EndScissorMode();
        EndDrawing();
    }
    CloseWindow();
}
