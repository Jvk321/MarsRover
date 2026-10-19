#include "simulation.h"
#include "scene_renderer.h"
#include "world_camera.h"
#include "terrain_panel.h"
#include "mission_feedback.h"
#include "mission_hud.h"
#include "presentation.h"
#include "ui_glass.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <exception>
#include <cstdlib>
#include <random>
#include <string>

int main(int argc,char** argv) {
    const bool snapshot=argc>=3 && std::string(argv[1])=="--snapshot";
    int snapshotFrames=0;
    int initialWidth=1600,initialHeight=1000;
    if(snapshot && argc==6 && std::string(argv[3])=="--size") {
        initialWidth=std::clamp(std::atoi(argv[4]),1280,3840);initialHeight=std::clamp(std::atoi(argv[5]),800,2160);
    }
    Terrain terrain(TerrainConfig{});
    Terrain missionTerrain=terrain;
    const auto& grid=terrain.grid;const auto& start=terrain.start;const auto& goal=terrain.goal;
    TerrainPanel terrainPanel(terrain.config);
    std::optional<TerrainConfig> pendingTerrain;
    bool playGenerated=false,pendingDemo=false,presentationMode=false;
    MissionHud hud;
    auto& speed=hud.settings.searchSpeed;auto& drivingSpeed=hud.settings.driveSpeed;
    auto& showCosts=hud.settings.showCosts;auto& editTool=hud.settings.tool;
    std::optional<Cell> lastPaint;bool rightPaintValue=true;

    std::mt19937 rockRandom(terrain.config.seed);
    Simulation simulation(grid,start,goal);const auto& search=simulation.search();
    // UI scale is defined in native client pixels, so Windows DPI does not
    // multiply an already resolution-scaled HUD a second time.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | (snapshot?FLAG_WINDOW_HIDDEN:0));
    InitWindow(initialWidth,initialHeight,"Mars Rover A* Navigation Simulator 2.5D");
    if(!IsWindowReady()) return 1;
    SetWindowMinSize(1280,800);SetTargetFPS(60);Ui::init();
    AudioSystem audio;audio.init((std::filesystem::path(GetApplicationDirectory())/"assets/audio").string());
    audio.settings.enabled=!snapshot;
    MissionFeedback feedback;
    WorldCamera worldCamera(grid.width(),grid.height(),Ui::layout(GetScreenWidth(),GetScreenHeight()).world);
    SceneRenderer scene;VisualEffects effects;effects.reset(terrain.config.seed);
    std::size_t cameraPathSize=0;
    auto resetScene=[&](bool fitCamera) {
        audio.reset();feedback.reset();effects.reset(terrain.config.seed);
        effects.setStorm(false);effects.setStormIntensity(StormIntensity::MEDIUM);
        cameraPathSize=0;rockRandom.seed(terrain.config.seed);
        if(fitCamera) {worldCamera.reset();lastPaint.reset();}
    };
    auto resetMission=[&]() {
        terrain=missionTerrain;
        simulation.replaceMap(grid,start,goal,false);resetScene(true);
    };
    while(!WindowShouldClose()) {
        Ui::beginFrame(GetFrameTime(),GetScreenWidth(),GetScreenHeight());
        const auto layout=Ui::layout(GetScreenWidth(),GetScreenHeight());const Rectangle viewport=layout.world;
        if(pendingTerrain) {
            try {
                terrain=Terrain(*pendingTerrain);missionTerrain=terrain;
                simulation.replaceMap(grid,start,goal,!playGenerated);
                bool autoCamera=pendingDemo || worldCamera.automatic();
                worldCamera=WorldCamera(grid.width(),grid.height(),viewport);worldCamera.setAuto(autoCamera);
                terrainPanel.setConfig(terrain.config);
                resetScene(true);presentationMode=pendingDemo;if(playGenerated) simulation.play();
            } catch(const std::exception&) {terrainPanel.showError("Cannot generate terrain. Check size or reduce dimensions.");}
            pendingTerrain.reset();playGenerated=false;pendingDemo=false;
        }
        if(IsKeyPressed(KEY_F1) && !terrainPanel.open) {feedback.helpOpen=!feedback.helpOpen;feedback.audioOpen=feedback.presentationOpen=false;simulation.pause();}
        if(IsKeyPressed(KEY_F2) && !terrainPanel.open) {feedback.audioOpen=!feedback.audioOpen;feedback.helpOpen=feedback.presentationOpen=false;simulation.pause();}
        if(IsKeyPressed(KEY_M)) audio.settings.enabled=!audio.settings.enabled;
        const bool uiBlocked=terrainPanel.open || feedback.blocksInput();
        const auto action=hud.input(layout,simulation,terrain,worldCamera,effects,audio,uiBlocked,presentationMode);
        // Keep the existing action semantics; input is resolved before this frame's tick.
        bool play=action.has(HudEvent::PLAY) || (!uiBlocked && IsKeyPressed(KEY_SPACE) && !simulation.playing());
        bool pause=action.has(HudEvent::PAUSE) || (!uiBlocked && IsKeyPressed(KEY_SPACE) && simulation.playing());
        bool step=action.has(HudEvent::STEP) || (!uiBlocked && IsKeyPressed(KEY_N));
        bool reset=action.has(HudEvent::RESET) || (!uiBlocked && IsKeyPressed(KEY_R));
        if(reset) resetMission();else if(step) simulation.step();else if(pause) simulation.pause();else if(play) simulation.play();
        simulation.setSpeed(speed);simulation.setDrivingSpeed(drivingSpeed);simulation.update(GetFrameTime());
        if(action.has(HudEvent::AUTO_CAMERA)) worldCamera.setAuto(!worldCamera.automatic());
        if(action.has(HudEvent::FIT_CAMERA)) {worldCamera.setAuto(false);worldCamera.reset();}
        if(action.has(HudEvent::NEW_MAP)) {resetMission();terrainPanel.open=true;}
        if(action.has(HudEvent::GENERATE)) {pendingTerrain=terrain.config;simulation.pause();}
        if(action.has(HudEvent::EDIT)) {simulation.enterEdit();missionTerrain=terrain;resetScene(false);}
        if(action.has(HudEvent::CLEAR)) {simulation.replaceMap(grid,start,goal,simulation.status()==SimulationStatus::EDIT_MAP);missionTerrain=terrain;resetScene(false);}
        if(action.has(HudEvent::PRESENTATION)) {feedback.presentationOpen=true;simulation.pause();}
        if(action.has(HudEvent::HELP)) {feedback.helpOpen=true;simulation.pause();}
        if(action.has(HudEvent::STORM)) effects.setStorm(!effects.storm());
        if(action.stormLevel>=0) effects.setStormIntensity(static_cast<StormIntensity>(action.stormLevel));
        if(action.has(HudEvent::DROP_ROCK)) if(auto rock=simulation.dropRock(rockRandom())) {
            terrain.edit(*rock,EditTool::ADD_ROCK);
            worldCamera.noticeRock(WorldCamera::project(rock->x+0.5,rock->y+0.5));cameraPathSize=0;
        }
        if(!uiBlocked && IsKeyPressed(KEY_C)) showCosts=!showCosts;
        const bool editing=simulation.status()==SimulationStatus::EDIT_MAP;
        worldCamera.resize(viewport);
        if(search.path().size()!=cameraPathSize) {
            std::vector<Vector2> points;for(Cell c:search.path()) points.push_back(WorldCamera::project(c.x+0.5,c.y+0.5));
            worldCamera.setPath(points);cameraPathSize=points.size();
        }
        CameraPhase phase=CameraPhase::WIDE;
        if(simulation.status()==SimulationStatus::PATH_REVEAL || simulation.status()==SimulationStatus::PATH_READY) phase=CameraPhase::REVEAL;
        if(simulation.status()==SimulationStatus::DRIVING) phase=CameraPhase::FOLLOW;
        if(simulation.status()==SimulationStatus::ROUTE_BLOCKED) phase=CameraPhase::BLOCKED;
        if(simulation.status()==SimulationStatus::RECALCULATING) phase=CameraPhase::REPLAN;
        if(simulation.status()==SimulationStatus::ARRIVAL) phase=CameraPhase::ARRIVAL;
        if(simulation.status()==SimulationStatus::MISSION_COMPLETE) phase=CameraPhase::COMPLETE;
        const auto& rover=simulation.rover();
        CameraInput cameraInput;
        cameraInput.worldHovered=!terrainPanel.open && !feedback.blocksInput() &&
            CheckCollisionPointRec(GetMousePosition(),viewport) && !MissionHud::overControls(layout,GetMousePosition(),editing);
        cameraInput.wheel=GetMouseWheelMove();
        if(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) cameraInput.drag=GetMouseDelta();
        cameraInput.keys={static_cast<float>(IsKeyDown(KEY_D)-IsKeyDown(KEY_A)),static_cast<float>(IsKeyDown(KEY_S)-IsKeyDown(KEY_W))};
        if(action.zoomWheel!=0) {cameraInput.worldHovered=true;cameraInput.wheel=action.zoomWheel;}
        worldCamera.update(GetFrameTime(),phase,WorldCamera::project(rover.x()+0.5,rover.y()+0.5),rover.heading(),
                           simulation.playing() || simulation.status()==SimulationStatus::READY,cameraInput);
        const auto hovered=cameraInput.worldHovered ? worldCamera.pick(GetMousePosition(),grid.width(),grid.height()) : std::nullopt;
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
                resetScene(false);presentationMode=false;
            }
        }
        effects.update(GetFrameTime(),simulation,start);feedback.update(GetFrameTime(),simulation);
        audio.update(GetFrameTime(),simulation,effects.sandstorm());
        BeginDrawing();ClearBackground({9,17,28,255});
        Ui::beginWorld();
        scene.draw(terrain,simulation,worldCamera,effects,layout.canvas,!uiBlocked?hovered:std::nullopt,showCosts,false);
        Ui::endWorld();Ui::drawWorld();
        hud.draw(layout,simulation,terrain,worldCamera,effects,audio,feedback,uiBlocked,presentationMode);
        scene.drawMinimap(terrain,simulation,worldCamera,viewport,layout.minimap);
        if(terrainPanel.open && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) audio.click();
        if(auto config=terrainPanel.draw()) pendingTerrain=*config;
        auto feedbackAction=feedback.draw(viewport,terrain,simulation,audio);
        if(feedbackAction==FeedbackAction::START_MISSION) simulation.play();
        if(feedbackAction==FeedbackAction::START_DEMO) {
            const auto& preset=presentationPresets[feedback.selectedPreset];pendingTerrain=preset.terrain;playGenerated=pendingDemo=true;
            speed=static_cast<float>(preset.searchSpeed);drivingSpeed=static_cast<float>(preset.drivingSpeed);
        }
        if(feedbackAction==FeedbackAction::NEW_MISSION) {resetMission();terrainPanel.open=true;}
        if(feedbackAction==FeedbackAction::REPLAY) {resetMission();simulation.play();}
        if(feedbackAction==FeedbackAction::RESET) resetMission();
        if(feedbackAction==FeedbackAction::EDIT) {simulation.enterEdit();missionTerrain=terrain;resetScene(true);}
        if(snapshot && ++snapshotFrames==24) {
            rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,argv[2]);UnloadImage(image);EndDrawing();break;
        }
        EndDrawing();
    }
    scene.shutdown();audio.shutdown();Ui::shutdown();CloseWindow();
}
