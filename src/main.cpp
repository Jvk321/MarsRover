#include "simulation.h"
#include "scene_renderer.h"
#include "visual_config.h"
#include "world_camera.h"
#include "terrain_panel.h"
#include "mission_feedback.h"
#include "presentation.h"
#include "ui_controls.h"
#include "raylib.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <exception>
#include <random>
#include <string>

namespace {
bool uiBlocked=false;
AudioSystem* uiAudio=nullptr;
Color statusColor(SimulationStatus s) {
    switch(s) {
    case SimulationStatus::SEARCHING: return SKYBLUE;
    case SimulationStatus::RECALCULATING: return Color{184,142,255,255};
    case SimulationStatus::PATH_REVEAL: case SimulationStatus::PATH_READY: return VisualConfig::gold;
    case SimulationStatus::DRIVING: return VisualConfig::cyan;
    case SimulationStatus::MISSION_COMPLETE: return Color{127,227,167,255};
    case SimulationStatus::ROUTE_BLOCKED: case SimulationStatus::NO_ROUTE: case SimulationStatus::NO_PATH: return Color{255,125,88,255};
    default: return Color{157,178,190,255};
    }
}
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
    bool pressed=Ui::button(rect,label,enabled,uiBlocked);
    if(pressed && uiAudio) uiAudio->click();
    return pressed;
}
}
int main(int argc,char** argv) {
    const bool snapshot=argc==3 && std::string(argv[1])=="--snapshot";
    int snapshotFrames=0;
    Terrain terrain(TerrainConfig{});
    Terrain missionTerrain=terrain; // Replay/reset restore the generated or edited map, before DROP ROCK.
    const auto& grid=terrain.grid;
    const auto& start=terrain.start;
    const auto& goal=terrain.goal;
    TerrainPanel terrainPanel(terrain.config);
    std::optional<TerrainConfig> pendingTerrain;
    bool playGenerated=false,pendingDemo=false,presentationMode=false,showCosts=false;
    EditTool editTool=EditTool::SET_START;
    std::optional<Cell> lastPaint;
    bool rightPaintValue=true;
    int obstacleCount=terrain.obstacleCount();
    std::mt19937 rockRandom(terrain.config.seed);
    Simulation simulation(grid, start, goal);
    const auto& search = simulation.search();

    float speed = 8.0f;
    float drivingSpeed = 1.5f;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | (snapshot ? FLAG_WINDOW_HIDDEN : 0));
    InitWindow(1280, 960, "Mars Rover A* Navigation Simulator 2.5D");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(1000, 940);
    SetTargetFPS(60);
    AudioSystem audio;audio.init((std::filesystem::path(GetApplicationDirectory())/"assets/audio").string());
    audio.settings.enabled=!snapshot;uiAudio=&audio;
    MissionFeedback feedback;
    WorldCamera worldCamera(grid.width(),grid.height(),{300,140,980,790});
    SceneRenderer scene;
    VisualEffects effects; effects.reset(terrain.config.seed);
    std::size_t cameraPathSize = 0;
    auto resetScene=[&](bool fitCamera) {
        audio.reset();feedback.reset();effects.reset(terrain.config.seed);
        effects.setStorm(false);effects.setStormIntensity(StormIntensity::MEDIUM);
        cameraPathSize=0;rockRandom.seed(terrain.config.seed);
        if(fitCamera) {worldCamera.reset();lastPaint.reset();}
    };
    auto resetMission=[&]() {
        terrain=missionTerrain;obstacleCount=terrain.obstacleCount();
        simulation.replaceMap(grid,start,goal,false);resetScene(true);
    };
    while (!WindowShouldClose()) {
        const Rectangle viewport{300,140,static_cast<float>(GetScreenWidth()-300),static_cast<float>(GetScreenHeight()-170)};
        if (pendingTerrain) {
            try {
                terrain=Terrain(*pendingTerrain);missionTerrain=terrain;
                simulation.replaceMap(grid,start,goal,!playGenerated);
                bool autoCamera=pendingDemo || worldCamera.automatic();
                worldCamera=WorldCamera(grid.width(),grid.height(),viewport);
                worldCamera.setAuto(autoCamera);
                obstacleCount=terrain.obstacleCount();terrainPanel.setConfig(terrain.config);
                resetScene(true);presentationMode=pendingDemo;
                if(playGenerated) simulation.play();
            } catch(const std::exception&) {
                terrainPanel.showError("Cannot generate terrain. Check size or reduce dimensions.");
            }
            pendingTerrain.reset();playGenerated=false;pendingDemo=false;
        }
        if(IsKeyPressed(KEY_F1) && !terrainPanel.open) { feedback.helpOpen=!feedback.helpOpen;feedback.audioOpen=feedback.presentationOpen=false;simulation.pause(); }
        if(IsKeyPressed(KEY_F2) && !terrainPanel.open) { feedback.audioOpen=!feedback.audioOpen;feedback.helpOpen=feedback.presentationOpen=false;simulation.pause(); }
        if(IsKeyPressed(KEY_M)) audio.settings.enabled=!audio.settings.enabled;
        uiBlocked=terrainPanel.open || feedback.blocksInput();
        BeginDrawing();
        ClearBackground({10,16,24,255});
        DrawRectangleGradientV(0,0,300,GetScreenHeight(),Color{13,25,36,248},Color{13,18,28,248});
        DrawLine(299,0,299,GetScreenHeight(),Fade(VisualConfig::cyan,0.35f));
        for(Rectangle group : {Rectangle{10,51,280,194},Rectangle{10,266,280,139},Rectangle{10,420,280,145},Rectangle{10,580,280,178},Rectangle{10,771,280,99}}) {
            DrawRectangleRounded(group,0.04f,4,Color{20,32,43,190});
            DrawRectangleRoundedLines(group,0.04f,4,Fade(VisualConfig::cyan,0.13f));
        }
        DrawRectangleRounded({308,0,static_cast<float>(GetScreenWidth()-316),132},0.03f,4,Color{16,28,39,235});
        DrawRectangleLinesEx({308,0,static_cast<float>(GetScreenWidth()-316),132},1,Fade(VisualConfig::cyan,0.23f));
        DrawText("ARES / NAVIGATION",20,19,23,VisualConfig::cyan);
        DrawText("NAVIGATION",20,250,10,VisualConfig::cyan);
        DrawText("TELEMETRY",20,408,10,VisualConfig::cyan);
        DrawText("ROVER / DRIVE CONTROL",20,568,10,VisualConfig::cyan);
        DrawText("MAP / TERRAIN",320,2,10,VisualConfig::cyan);
        DrawText("CAMERA",20,870,8,VisualConfig::cyan);
        DrawText("EFFECTS / AUDIO",480,80,8,VisualConfig::cyan);
        // Process controls before the playback tick: PAUSE/RESET take effect immediately.
        bool play = button({20,65,120,42},"PLAY",simulation.active());
        bool pause = button({152,65,120,42},"PAUSE",simulation.playing());
        bool step = button({20,117,120,42},"STEP",simulation.active());
        bool reset = button({152,117,120,42},"RESET");
        play = play || (!uiBlocked && IsKeyPressed(KEY_SPACE) && !simulation.playing());
        pause = pause || (!uiBlocked && IsKeyPressed(KEY_SPACE) && simulation.playing());
        step = step || (!uiBlocked && IsKeyPressed(KEY_N));
        reset = reset || (!uiBlocked && IsKeyPressed(KEY_R));
        if (reset) resetMission();
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
        DrawRectangle(11,268,3,44,statusColor(simulation.status()));
        if(feedback.flash()>0) DrawRectangleRounded({16,268,270,46},0.05f,4,Fade(statusColor(simulation.status()),feedback.flash()*0.16f));
        DrawText(statusName(simulation.status()),22,273,18,statusColor(simulation.status()));
        DrawText(simulation.playing() ? "LIVE  /  PLAYING" : "STANDBY  /  PAUSED",22,298,12,Color{124,151,166,255});
        if (search.current()) {
            const auto& n = *search.current();
            DrawText(TextFormat("CURRENT x/y: %d / %d",n.cell.x,n.cell.y),20,328,18,RAYWHITE);
            DrawText(TextFormat("g: %d   h: %.2f",n.g,n.h),20,355,18,RAYWHITE);
            DrawText(TextFormat("f: %.2f",n.f),20,382,18,RAYWHITE);
        } else {
            DrawText("CURRENT x/y: - / -",20,328,18,RAYWHITE);
            DrawText("g: -   h: -   f: -",20,355,18,RAYWHITE);
        }
        DrawText(TextFormat("OPEN: %d",static_cast<int>(search.open().size())),20,430,18,SKYBLUE);
        DrawText(TextFormat("CLOSED: %d",static_cast<int>(search.closed().size())),20,458,18,Color{170,130,210,255});
        DrawText(TextFormat("ITERATIONS: %d",static_cast<int>(search.iterations())),20,486,18,RAYWHITE);
        DrawText(TextFormat("PATH: %d  REVEALED: %d/%d",static_cast<int>(simulation.pathLength()),
                           static_cast<int>(simulation.revealed()),static_cast<int>(simulation.pathLength())),20,520,17,GOLD);
        DrawText(TextFormat("REPLANS: %d",static_cast<int>(simulation.replans())),20,548,18,ORANGE);
        const auto& rover = simulation.rover();
        if (rover.available()) {
            DrawText(TextFormat("ROVER x/y: %.2f / %.2f",rover.x(),rover.y()),20,680,18,RAYWHITE);
            const auto segment = std::min(rover.segment()+1,simulation.pathLength());
            DrawText(TextFormat("SEGMENT: %d / %d",static_cast<int>(segment),static_cast<int>(simulation.pathLength())),20,710,18,RAYWHITE);
            DrawText(TextFormat("TRAVELLED: %.2f / %d",rover.travelled(),static_cast<int>(simulation.pathLength())),20,740,18,GOLD);
        } else DrawText("ROVER: waiting for path",20,680,18,LIGHTGRAY);
        DrawText(TextFormat("MISSION TIME: %.1f s",feedback.missionTime()),20,780,18,RAYWHITE);
        DrawText(TextFormat("OLD PATH: %d",static_cast<int>(simulation.oldLength())),20,810,18,LIGHTGRAY);
        DrawText(search.path().empty() || simulation.status()==SimulationStatus::ROUTE_BLOCKED ? "NEW PATH: -" : TextFormat("NEW PATH: %d",static_cast<int>(simulation.pathLength())),20,840,18,GOLD);
        if (button({20,880,252,38},worldCamera.automatic() ? "AUTO CAMERA: ON" : "AUTO CAMERA: OFF"))
            worldCamera.setAuto(!worldCamera.automatic());
        if (button({320,18,100,36},"NEW MAP")) {
            resetMission();terrainPanel.open=true;
        }
        if (button({430,18,100,36},"GENERATE")) { pendingTerrain=terrain.config; simulation.pause(); }
        if (button({540,18,100,36},"EDIT MAP")) { simulation.enterEdit();missionTerrain=terrain;resetScene(false); }
        if (button({650,18,110,36},"CLEAR PATH")) {
            simulation.replaceMap(grid,start,goal,simulation.status()==SimulationStatus::EDIT_MAP);missionTerrain=terrain;resetScene(false);
        }
        if(button({770,18,130,36},"PRESENTATION")) {feedback.presentationOpen=true;simulation.pause();}
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
            if(button({890,90,90,34},"AUDIO")) {feedback.audioOpen=true;simulation.pause();}
        } else {
            if (button({320,90,180,34},presentationMode?"DEMO BLOCK ROUTE":"DROP ROCK",simulation.canDropRock())) {
                if (auto rock=simulation.dropRock(rockRandom())) {
                    terrain.edit(*rock,EditTool::ADD_ROCK); obstacleCount=terrain.obstacleCount();
                    worldCamera.noticeRock(WorldCamera::project(rock->x+0.5,rock->y+0.5));
                    cameraPathSize=0;
                }
            }
            if(button({510,90,90,34},"AUDIO")) {feedback.audioOpen=true;simulation.pause();}
            if(button({610,90,150,34},audio.settings.enabled?"SOUND: ON":"SOUND: OFF")) audio.settings.enabled=!audio.settings.enabled;
            if(button({770,90,210,34},showCosts?"SHOW COSTS: ON":"SHOW COSTS: OFF")) showCosts=!showCosts;
        }
        if(button({910,18,70,36},"HELP")) {feedback.helpOpen=true;simulation.pause();}
        if(!uiBlocked && IsKeyPressed(KEY_C)) showCosts=!showCosts;
        worldCamera.resize(viewport);
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
        cameraInput.worldHovered=!terrainPanel.open && !feedback.blocksInput() && CheckCollisionPointRec(GetMousePosition(),viewport);
        cameraInput.wheel=GetMouseWheelMove();
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) cameraInput.drag=GetMouseDelta();
        cameraInput.keys={static_cast<float>(IsKeyDown(KEY_D)-IsKeyDown(KEY_A)),
                          static_cast<float>(IsKeyDown(KEY_S)-IsKeyDown(KEY_W))};
        worldCamera.update(GetFrameTime(),phase,WorldCamera::project(rover.x()+0.5,rover.y()+0.5),rover.heading(),
                           simulation.playing() || simulation.status()==SimulationStatus::READY,cameraInput);
        const auto hovered=worldCamera.pick(GetMousePosition(),grid.width(),grid.height());
        if (!hovered) lastPaint.reset();
        if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) lastPaint.reset();
        if (editing && !terrainPanel.open && !feedback.blocksInput() && hovered && !IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
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
                simulation.replaceMap(grid,start,goal,true);missionTerrain=terrain;
                resetScene(false);obstacleCount=terrain.obstacleCount();presentationMode=false;
            }
        }
        effects.update(GetFrameTime(),simulation,start);
        feedback.update(GetFrameTime(),simulation);
        audio.update(GetFrameTime(),simulation,effects.sandstorm());
        scene.draw(terrain,simulation,worldCamera,effects,viewport,
                   !uiBlocked ? hovered : std::nullopt,showCosts);
        DrawText("OPEN: blue  /  CLOSED: violet  /  CURRENT: bright outline",320,GetScreenHeight()-48,14,Color{177,200,211,255});
        DrawText(editing ? "LMB: tool  /  RMB: paint" : "WASD / MMB: pan   WHEEL: zoom",320,GetScreenHeight()-22,15,Color{150,175,186,255});
        const char* stormNames[]{"LIGHT","MEDIUM","HEAVY"};
        if(button({static_cast<float>(GetScreenWidth()-370),static_cast<float>(GetScreenHeight()-28),120,24},
                  stormNames[static_cast<int>(effects.sandstorm().intensity())]))
            effects.setStormIntensity(static_cast<StormIntensity>((static_cast<int>(effects.sandstorm().intensity())+1)%3));
        if(button({static_cast<float>(GetScreenWidth()-240),static_cast<float>(GetScreenHeight()-28),220,24},
                  effects.storm() ? "SANDSTORM: ON" : "SANDSTORM: OFF")) effects.setStorm(!effects.storm());
        if (simulation.status()==SimulationStatus::ROUTE_BLOCKED || simulation.status()==SimulationStatus::NO_ROUTE) {
            DrawRectangle(320,150,GetScreenWidth()-340,44,{45,26,27,235});
            DrawText(simulation.status()==SimulationStatus::NO_ROUTE ? "NO ALTERNATIVE ROUTE" : "ROUTE BLOCKED",340,160,24,ORANGE);
        }
        if(terrainPanel.open && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) audio.click();
        if (auto config=terrainPanel.draw()) pendingTerrain=*config;
        auto feedbackAction=feedback.draw(viewport,terrain,simulation,audio);
        if(feedbackAction==FeedbackAction::START_MISSION) simulation.play();
        if(feedbackAction==FeedbackAction::START_DEMO) {
            const auto& preset=presentationPresets[feedback.selectedPreset];
            pendingTerrain=preset.terrain;playGenerated=pendingDemo=true;
            speed=static_cast<float>(preset.searchSpeed);drivingSpeed=static_cast<float>(preset.drivingSpeed);
        }
        if(feedbackAction==FeedbackAction::NEW_MISSION) {resetMission();terrainPanel.open=true;}
        if(feedbackAction==FeedbackAction::REPLAY) {resetMission();simulation.play();}
        if(feedbackAction==FeedbackAction::RESET) resetMission();
        if(feedbackAction==FeedbackAction::EDIT) {
            simulation.enterEdit();missionTerrain=terrain;resetScene(true);
        }
        if(snapshot && ++snapshotFrames==8) {
            rlDrawRenderBatchActive();
            Image screenshot=LoadImageFromScreen();ExportImage(screenshot,argv[2]);UnloadImage(screenshot);
            EndDrawing();break;
        }
        EndDrawing();
    }
    uiAudio=nullptr;audio.shutdown();CloseWindow();
}
