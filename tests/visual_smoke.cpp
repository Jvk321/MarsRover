#include "scene_renderer.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    std::filesystem::path output=argc>1?argv[1]:"build/visual-qa";
    std::filesystem::create_directories(output);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(1280,800,"Mars visual QA");
    if(!IsWindowReady()) return 1;
    SetTargetFPS(0);
    Terrain terrain({80,60,18,47});Simulation sim(terrain.grid,terrain.start,terrain.goal);
    sim.setSpeed(60);sim.setDrivingSpeed(5);
    VisualEffects effects;effects.reset(47);SceneRenderer renderer;
    Rectangle viewport{0,0,1280,800};WorldCamera camera(80,60,viewport);
    auto target=LoadRenderTexture(1280,800);
    auto frame=[&]() {
        sim.update(1.0/30);effects.update(1.0f/30,sim,terrain.start);
        CameraPhase phase=CameraPhase::WIDE;
        if(sim.status()==SimulationStatus::DRIVING) phase=CameraPhase::FOLLOW;
        if(sim.status()==SimulationStatus::PATH_REVEAL) phase=CameraPhase::REVEAL;
        if(sim.status()==SimulationStatus::ROUTE_BLOCKED) phase=CameraPhase::BLOCKED;
        if(sim.status()==SimulationStatus::RECALCULATING) phase=CameraPhase::REPLAN;
        if(sim.status()==SimulationStatus::ARRIVAL) phase=CameraPhase::ARRIVAL;
        const auto& r=sim.rover();
        camera.update(1.0f/30,phase,WorldCamera::project(r.x()+0.5,r.y()+0.5),r.heading(),sim.playing());
    };
    auto capture=[&](const char* name) {
        BeginTextureMode(target);ClearBackground(BLACK);
        renderer.draw(terrain,sim,camera,effects,viewport,std::nullopt);
        DrawText(name,20,20,22,RAYWHITE);EndTextureMode();
        Image capture=LoadImageFromTexture(target.texture);ImageFlipVertical(&capture);
        ExportImage(capture,(output/(std::string(name)+".png")).string().c_str());UnloadImage(capture);
        std::cout << "Captured " << name << '\n';
    };
    auto wait=[&](SimulationStatus status) {
        for(int i=0;sim.status()!=status;++i) {
            if(i>30000 || !sim.active()) throw std::runtime_error("Visual QA scenario failed");
            frame();
        }
    };
    sim.play();for(int i=0;i<15;++i) frame();capture("01-searching-80x60");
    wait(SimulationStatus::PATH_REVEAL);for(int i=0;i<20;++i) frame();capture("02-path-reveal");
    wait(SimulationStatus::DRIVING);for(int i=0;i<90;++i) frame();capture("03-driving");
    effects.setStorm(true);for(int i=0;i<20;++i) frame();capture("04-sandstorm");
    const char* levels[]{"04a-light","04b-medium","04c-heavy"};
    for(int level=0;level<3;++level) {
        effects.setStormIntensity(static_cast<StormIntensity>(level));
        for(int i=0;i<90;++i) frame();
        capture(levels[level]);
    }
    CameraInput input;input.worldHovered=true;input.wheel=2;input.drag={35,10};
    camera.update(0.1f,CameraPhase::FOLLOW,{},0,true,input);capture("05-manual-camera");
    effects.setStorm(false);camera.setAuto(true);
    auto rock=sim.dropRock(0);
    if(!rock) throw std::runtime_error("Expected DROP ROCK");
    terrain.edit(*rock,EditTool::ADD_ROCK);camera.noticeRock(WorldCamera::project(rock->x+0.5,rock->y+0.5));
    for(int i=0;i<8;++i) frame();capture("06-route-blocked");
    wait(SimulationStatus::RECALCULATING);effects.update(1.0f/30,sim,terrain.start);capture("07-recalculating");
    wait(SimulationStatus::PATH_REVEAL);wait(SimulationStatus::DRIVING);
    wait(SimulationStatus::MISSION_COMPLETE);capture("08-mission-complete");
    UnloadRenderTexture(target);CloseWindow();std::cout << "Visual smoke passed\n";
}
