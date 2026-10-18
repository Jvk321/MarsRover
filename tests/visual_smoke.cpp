#include "scene_renderer.h"
#include "mission_feedback.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
int main(int argc,char** argv) {
    std::filesystem::path output=argc>1?argv[1]:"build/visual-qa";
    std::filesystem::create_directories(output);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(1280,800,"Mars visual QA");
    if(!IsWindowReady()) return 1;
    SetTargetFPS(0);
    Terrain terrain({80,60,18,47});Simulation sim(terrain.grid,terrain.start,terrain.goal);
    sim.setSpeed(60);sim.setDrivingSpeed(5);
    VisualEffects effects;effects.reset(47);SceneRenderer renderer;
    MissionFeedback feedback;feedback.helpOpen=false;AudioSystem audio;audio.init("assets/audio");audio.settings.enabled=false;
    Rectangle viewport{0,0,1280,800};WorldCamera camera(80,60,viewport);
    auto target=LoadRenderTexture(1280,800);
    auto frame=[&]() {
        sim.update(1.0/30);effects.update(1.0f/30,sim,terrain.start);
        feedback.update(1.0f/30,sim);
        CameraPhase phase=CameraPhase::WIDE;
        if(sim.status()==SimulationStatus::DRIVING) phase=CameraPhase::FOLLOW;
        if(sim.status()==SimulationStatus::PATH_REVEAL) phase=CameraPhase::REVEAL;
        if(sim.status()==SimulationStatus::ROUTE_BLOCKED) phase=CameraPhase::BLOCKED;
        if(sim.status()==SimulationStatus::RECALCULATING) phase=CameraPhase::REPLAN;
        if(sim.status()==SimulationStatus::ARRIVAL) phase=CameraPhase::ARRIVAL;
        const auto& r=sim.rover();
        camera.update(1.0f/30,phase,WorldCamera::project(r.x()+0.5,r.y()+0.5),r.heading(),sim.playing());
    };
    auto capture=[&](const char* name,bool costs=false) {
        BeginTextureMode(target);ClearBackground(BLACK);
        renderer.draw(terrain,sim,camera,effects,viewport,sim.search().current()?std::optional<Cell>(sim.search().current()->cell):std::nullopt,costs);
        feedback.draw(viewport,terrain,sim,audio);
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
    sim.pause();
    auto node=sim.search().current()->cell;
    for(int i=0;i<80;++i) camera.update(0.05f,CameraPhase::FOLLOW,WorldCamera::project(node.x+0.5,node.y+0.5),0,true);
    capture("01a-show-costs",true);camera.reset();sim.play();
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
    for(int i=0;i<30;++i) feedback.update(1.0f/30,sim);
    capture("08a-mission-summary");
    feedback.helpOpen=true;capture("09-help");feedback.helpOpen=false;
    feedback.audioOpen=true;capture("10-audio");feedback.audioOpen=false;
    feedback.presentationOpen=true;capture("10a-presentation");feedback.presentationOpen=false;
    terrain=Terrain({18,3,0,47});
    for(int x=0;x<18;++x) {terrain.grid.setBlocked({x,0});terrain.grid.setBlocked({x,2});}
    terrain.start={0,1};terrain.goal={17,1};sim.replaceMap(terrain.grid,terrain.start,terrain.goal,false);
    effects.reset(47);feedback.reset();camera=WorldCamera(18,3,viewport);sim.play();
    wait(SimulationStatus::DRIVING);auto blocked=sim.dropRock(0);if(!blocked) throw std::runtime_error("No-route DROP failed");
    terrain.edit(*blocked,EditTool::ADD_ROCK);
    wait(SimulationStatus::NO_ROUTE);capture("11-no-alternative");
    // Uncapped real frame submission, including effects, culling and maximum storm pools.
    // Report timing rather than asserting an FPS threshold dependent on the test machine.
    for(auto config:{TerrainConfig{20,20,20,42},TerrainConfig{50,50,20,42},TerrainConfig{80,60,18,47}}) {
        terrain=Terrain(config);sim.replaceMap(terrain.grid,terrain.start,terrain.goal,false);
        effects.reset(config.seed);feedback.reset();camera=WorldCamera(config.width,config.height,viewport);
        effects.setStormIntensity(StormIntensity::HEAVY);effects.setStorm(true);
        for(int i=0;i<90;++i) effects.update(1.0f/30,sim,terrain.start);
        std::vector<double> timings;timings.reserve(150);
        for(int i=0;i<170;++i) {
            auto begin=std::chrono::steady_clock::now();
            effects.update(1.0f/60,sim,terrain.start);
            BeginDrawing();ClearBackground(BLACK);
            renderer.draw(terrain,sim,camera,effects,viewport,std::nullopt);EndDrawing();
            if(i>=20) timings.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
        }
        std::sort(timings.begin(),timings.end());
        std::cout<<"Render "<<config.width<<'x'<<config.height<<" HEAVY: median "<<timings[timings.size()/2]
                 <<" ms, p95 "<<timings[timings.size()*95/100]<<" ms\n";
    }
    audio.shutdown();UnloadRenderTexture(target);CloseWindow();std::cout << "Visual smoke passed\n";
}
