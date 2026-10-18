#include "simulation.h"
#include "rover_renderer.h"
#include "world_camera.h"
#include "terrain_panel.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {
bool uiBlocked=false;
const char* statusName(SimulationStatus status) {
    switch (status) {
    case SimulationStatus::EDIT_MAP: return "EDIT_MAP";
    case SimulationStatus::READY: return "READY";
    case SimulationStatus::SEARCHING: return "SEARCHING";
    case SimulationStatus::PATH_REVEAL: return "REVEALING PATH...";
    case SimulationStatus::PATH_READY: return "PATH_READY";
    case SimulationStatus::DRIVING: return "DRIVING";
    case SimulationStatus::ROUTE_BLOCKED: return "ROUTE BLOCKED";
    case SimulationStatus::RECALCULATING: return "RECALCULATING...";
    case SimulationStatus::NO_ROUTE: return "NO ALTERNATIVE ROUTE";
    case SimulationStatus::ARRIVAL: return "CINEMATIC ARRIVAL";
    case SimulationStatus::MISSION_COMPLETE: return "MISSION COMPLETE";
    case SimulationStatus::NO_PATH: return "NO_PATH";
    case SimulationStatus::INVALID_ENDPOINTS: return "INVALID_ENDPOINTS";
    }
    return "UNKNOWN";
}
bool button(Rectangle rect, const char* label, bool enabled = true) {
    bool hover = enabled && !uiBlocked && CheckCollisionPointRec(GetMousePosition(), rect);
    DrawRectangleRec(rect, !enabled ? Color{40,40,45,255} : hover ? Color{85,85,105,255} : Color{58,58,73,255});
    DrawText(label, static_cast<int>(rect.x + 10), static_cast<int>(rect.y + 11), 18, enabled ? RAYWHITE : GRAY);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
}
int main() {
    Terrain terrain(TerrainConfig{});
    const auto& grid=terrain.grid;
    const auto& start=terrain.start;
    const auto& goal=terrain.goal;
    TerrainPanel terrainPanel(terrain.config);
    std::optional<TerrainConfig> pendingTerrain;
    EditTool editTool=EditTool::SET_START;
    std::optional<Cell> lastPaint;
    bool rightPaintValue=true;
    int obstacleCount=terrain.obstacleCount();
    std::mt19937 rockRandom(terrain.config.seed);
    Simulation simulation(grid, start, goal);
    const auto& search = simulation.search();

    float speed = 8.0f;
    float drivingSpeed = 1.5f;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 960, "Mars Rover A* Navigation Simulator 2.5D");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(1000, 940);
    SetTargetFPS(60);
    WorldCamera worldCamera(grid.width(),grid.height(),{300,140,980,790});
    std::size_t cameraPathSize = 0;
    while (!WindowShouldClose()) {
        const Rectangle viewport{300,140,static_cast<float>(GetScreenWidth()-300),static_cast<float>(GetScreenHeight()-170)};
        if (pendingTerrain) {
            terrain=Terrain(*pendingTerrain); pendingTerrain.reset();
            simulation.replaceMap(grid,start,goal,true);
            bool autoCamera=worldCamera.automatic();
            worldCamera=WorldCamera(grid.width(),grid.height(),viewport);
            worldCamera.setAuto(autoCamera);
            obstacleCount=terrain.obstacleCount(); cameraPathSize=0; lastPaint.reset();
            rockRandom.seed(terrain.config.seed);
        }
        uiBlocked=terrainPanel.open;
        BeginDrawing();
        ClearBackground({28,24,25,255});
        DrawRectangle(0,0,300,GetScreenHeight(),{32,31,40,255});
        DrawText("A* NAVIGATION",20,20,24,RAYWHITE);
        // Process controls before the playback tick: PAUSE/RESET take effect immediately.
        bool play = button({20,65,120,42},"PLAY",simulation.active());
        bool pause = button({152,65,120,42},"PAUSE",simulation.playing());
        bool step = button({20,117,120,42},"STEP",simulation.active());
        bool reset = button({152,117,120,42},"RESET");
        play = play || (!uiBlocked && IsKeyPressed(KEY_SPACE) && !simulation.playing());
        pause = pause || (!uiBlocked && IsKeyPressed(KEY_SPACE) && simulation.playing());
        step = step || (!uiBlocked && IsKeyPressed(KEY_N));
        reset = reset || (!uiBlocked && IsKeyPressed(KEY_R));
        if (reset) simulation.reset();
        else if (step) simulation.step();
        else if (pause) simulation.pause();
        else if (play) simulation.play();
        DrawText(TextFormat("SPEED: %.0f steps/s",speed),20,180,18,RAYWHITE);
        if (button({20,207,40,36},"-")) speed = std::max(1.0f,speed-1);
        if (button({232,207,40,36},"+")) speed = std::min(60.0f,speed+1);
        Rectangle track{76,220,140,10};
        if (!uiBlocked && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),{70,207,152,36})) {
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
        if (!uiBlocked && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),{70,620,152,36}))
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
        DrawText(TextFormat("REPLANS: %d",static_cast<int>(simulation.replans())),20,780,18,ORANGE);
        DrawText(TextFormat("OLD PATH: %d",static_cast<int>(simulation.oldLength())),20,810,18,LIGHTGRAY);
        DrawText(search.path().empty() || simulation.status()==SimulationStatus::ROUTE_BLOCKED ? "NEW PATH: -" : TextFormat("NEW PATH: %d",static_cast<int>(simulation.pathLength())),20,840,18,GOLD);
        if (button({20,880,252,38},worldCamera.automatic() ? "AUTO CAMERA: ON" : "AUTO CAMERA: OFF"))
            worldCamera.setAuto(!worldCamera.automatic());
        if (button({320,12,130,36},"NEW MAP")) { terrainPanel.open=true; simulation.pause(); }
        if (button({460,12,130,36},"GENERATE")) { pendingTerrain=terrain.config; simulation.pause(); }
        if (button({600,12,130,36},"EDIT MAP")) { simulation.enterEdit(); cameraPathSize=0; }
        if (button({740,12,145,36},"CLEAR PATH")) {
            simulation.replaceMap(grid,start,goal,simulation.status()==SimulationStatus::EDIT_MAP); cameraPathSize=0;
        }
        DrawText(TextFormat("MAP: %d x %d   OBSTACLES: %d / %.1f%%   SEED: %u",grid.width(),grid.height(),obstacleCount,
                            100.0*obstacleCount/(grid.width()*grid.height()),terrain.config.seed),320,59,17,LIGHTGRAY);
        const bool editing=simulation.status()==SimulationStatus::EDIT_MAP;
        if (editing) {
            const char* tools[]{"SET START","SET GOAL","ADD ROCK","ERASER"};
            for (int i=0;i<4;++i) {
                Rectangle rect{320+i*140.0f,90,130,34};
                if (button(rect,tools[i])) editTool=static_cast<EditTool>(i);
                if (editTool==static_cast<EditTool>(i)) DrawRectangleLinesEx(rect,2,GOLD);
            }
        } else {
            if (button({320,90,145,34},"DROP ROCK",simulation.canDropRock())) {
                if (auto rock=simulation.dropRock(rockRandom())) {
                    terrain.edit(*rock,EditTool::ADD_ROCK); obstacleCount=terrain.obstacleCount();
                    worldCamera.noticeRock(WorldCamera::project(rock->x+0.5,rock->y+0.5));
                    cameraPathSize=0;
                }
            }
            DrawText("OPEN: blue | CLOSED: purple | Gold: active path",480,99,16,LIGHTGRAY);
        }
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
        if (simulation.status()==SimulationStatus::ROUTE_BLOCKED) phase=CameraPhase::BLOCKED;
        if (simulation.status()==SimulationStatus::RECALCULATING) phase=CameraPhase::REPLAN;
        if (simulation.status()==SimulationStatus::ARRIVAL) phase=CameraPhase::ARRIVAL;
        if (simulation.status()==SimulationStatus::MISSION_COMPLETE) phase=CameraPhase::COMPLETE;
        CameraInput cameraInput;
        cameraInput.worldHovered=!terrainPanel.open && CheckCollisionPointRec(GetMousePosition(),viewport);
        cameraInput.wheel=GetMouseWheelMove();
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) cameraInput.drag=GetMouseDelta();
        cameraInput.keys={static_cast<float>(IsKeyDown(KEY_D)-IsKeyDown(KEY_A)),
                          static_cast<float>(IsKeyDown(KEY_S)-IsKeyDown(KEY_W))};
        worldCamera.update(GetFrameTime(),phase,WorldCamera::project(rover.x()+0.5,rover.y()+0.5),rover.heading(),
                           simulation.playing() || simulation.status()==SimulationStatus::READY,cameraInput);
        const auto hovered=worldCamera.pick(GetMousePosition(),grid.width(),grid.height());
        if (!hovered) lastPaint.reset();
        if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) lastPaint.reset();
        if (editing && !terrainPanel.open && hovered && !IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            bool changed=false;
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                rightPaintValue=!grid.blocked(*hovered);
                lastPaint.reset();
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && (!lastPaint || !(*lastPaint==*hovered))) {
                changed=terrain.stroke(lastPaint.value_or(*hovered),*hovered,rightPaintValue ? EditTool::ADD_ROCK : EditTool::ERASER);
                lastPaint=*hovered;
            } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                if ((editTool==EditTool::SET_START || editTool==EditTool::SET_GOAL) ? IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                    : (!lastPaint || !(*lastPaint==*hovered))) {
                    if (editTool==EditTool::ADD_ROCK || editTool==EditTool::ERASER)
                        changed=terrain.stroke(lastPaint.value_or(*hovered),*hovered,editTool);
                    else changed=terrain.edit(*hovered,editTool);
                    lastPaint=*hovered;
                }
            }
            if (changed) {
                simulation.replaceMap(grid,start,goal,true); cameraPathSize=0; obstacleCount=terrain.obstacleCount();
            }
        }
        constexpr float halfW=32,halfH=16;
        const Vector2 origin{0,0};
        auto project = [=](int x,int y) {
            return Vector2{origin.x+(x-y)*halfW,origin.y+(x+y)*halfH};
        };
        DrawText(editing ? "LMB: selected tool | RMB drag: toggle/paint | PLAY: run A*" : "Wheel: zoom | MMB / WASD: pan (manual camera)",320,GetScreenHeight()-24,16,LIGHTGRAY);
        BeginScissorMode(300,140,GetScreenWidth()-300,GetScreenHeight()-170);
        BeginMode2D(worldCamera.view());
        // Build renderer-only masks once per frame, keeping large maps responsive.
        const auto count=static_cast<std::size_t>(grid.width())*grid.height();
        std::vector<CellState> states(count,CellState::UNVISITED);
        std::vector<bool> pathMask(count,false);
        std::vector<bool> oldMask(count,false);
        auto index=[&](Cell c){ return static_cast<std::size_t>(c.y)*grid.width()+c.x; };
        const bool blocked=simulation.status()==SimulationStatus::ROUTE_BLOCKED;
        if (!editing) {
            for (Cell c:simulation.oldPath()) oldMask[index(c)]=true;
        }
        if (!editing && !blocked) {
            for (const auto& node:search.open()) states[index(node.cell)]=CellState::OPEN;
            for (const auto& node:search.closed()) states[index(node.cell)]=CellState::CLOSED;
            if (search.current()) states[index(search.current()->cell)]=CellState::CURRENT;
            if (!search.path().empty())
                for (std::size_t i=0;i<=simulation.revealed();++i) pathMask[index(search.path()[i])]=true;
        }
        for (int y=0;y<grid.height();++y) for (int x=0;x<grid.width();++x) {
            Cell cell{x,y};
            Color color = grid.blocked(cell) ? Color{65,49,45,255} : Color{161,94,64,255};
            if (oldMask[index(cell)] && !grid.blocked(cell)) color=Color{100,86,62,255};
            CellState state = states[index(cell)];
            if (state == CellState::OPEN) color = Color{45,115,210,255};
            if (state == CellState::CLOSED || state == CellState::CURRENT) color = Color{60,32,85,255};
            if (pathMask[index(cell)]) color = Color{255,218,55,255};
            if (editing && cell==start) color=Color{60,160,100,255};
            if (editing && cell==goal) color=Color{80,160,220,255};
            Vector2 top=project(x,y), right=project(x+1,y), bottom=project(x+1,y+1), left=project(x,y+1);
            DrawTriangle(top,left,bottom,color); DrawTriangle(top,bottom,right,color);
            Color edge{55,39,34,255};
            DrawLineV(top,right,edge); DrawLineV(right,bottom,edge);
            DrawLineV(bottom,left,edge); DrawLineV(left,top,edge);
            if (editing && hovered && *hovered==cell && !terrainPanel.open) {
                DrawLineEx(top,right,2,WHITE); DrawLineEx(right,bottom,2,WHITE);
                DrawLineEx(bottom,left,2,WHITE); DrawLineEx(left,top,2,WHITE);
            }
            if (cell == start || cell == goal) {
                const char* label = cell == start ? "S" : "G";
                DrawText(label,static_cast<int>((top.x+bottom.x)/2-4),static_cast<int>((top.y+bottom.y)/2-7),14,RAYWHITE);
            }
        }
        // Dropped boulders: raised dark silhouettes, no particle effects.
        for (Cell rock:simulation.droppedRocks()) {
            Vector2 center=WorldCamera::project(rock.x+0.5,rock.y+0.5);
            Vector2 peak{center.x-3,center.y-18},left{center.x-15,center.y+3},right{center.x+15,center.y+3};
            DrawTriangle(peak,left,right,Color{43,35,35,255});
            DrawTriangle(peak,{center.x,center.y+8},right,Color{78,56,48,255});
            DrawLineEx(peak,left,2,Color{150,100,75,255});
        }
        // Draw CURRENT last so adjacent tile edges cannot cover its outline.
        const bool revealing = simulation.status() == SimulationStatus::PATH_REVEAL;
        const bool searching = simulation.status() == SimulationStatus::SEARCHING || simulation.status()==SimulationStatus::RECALCULATING;
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
        if (simulation.status()==SimulationStatus::ROUTE_BLOCKED || simulation.status()==SimulationStatus::NO_ROUTE) {
            DrawRectangle(320,150,GetScreenWidth()-340,44,{45,26,27,235});
            DrawText(simulation.status()==SimulationStatus::NO_ROUTE ? "NO ALTERNATIVE ROUTE" : "ROUTE BLOCKED",340,160,24,ORANGE);
        }
        if (auto config=terrainPanel.draw()) pendingTerrain=*config;
        EndDrawing();
    }
    CloseWindow();
}
