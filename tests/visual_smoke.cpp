#include "scene_renderer.h"
#include "rover_renderer.h"
#include "mission_feedback.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
namespace {
void captureWheelViews(const std::filesystem::path& output) {
    RoverRenderer roverArt;
    auto target=LoadRenderTexture(1280,800);
    auto save=[&](const char* name) {
        EndTextureMode();
        Image image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);
        ExportImage(image,(output/name).string().c_str());UnloadImage(image);
    };
    auto place=[&](const Rover& rover,int i) {
        const float x=320.0f+(i%2)*640,ground=300.0f+(i/2)*380;
        Vector2 origin{x-static_cast<float>(rover.x()-rover.y())*125,
                       ground-static_cast<float>(rover.x()+rover.y()+1)*62.5f};
        DrawLineEx({x-170,ground+45},{x+170,ground-125},1,{193,101,59,255});
        roverArt.draw(rover,origin,125,62.5f);
    };
    BeginTextureMode(target);ClearBackground(Color{174,83,47,255});
    Cell directions[]{{1,0},{0,1},{-1,0},{0,-1}};
    for(int i=0;i<4;++i) {
        Rover rover;rover.reset({{0,0},directions[i]});place(rover,i);
        DrawText(TextFormat("GRID (%+d, %+d)",directions[i].x,directions[i].y),240+(i%2)*640,32+(i/2)*380,20,RAYWHITE);
    }
    save("17-six-wheel-directions.png");
    BeginTextureMode(target);ClearBackground(Color{174,83,47,255});
    const double times[]{0.04,0.225,0.41,0.45};
    const char* labels[]{"PRE-STEER","MID TURN / 45 DEG","STEERING SETTLES","TURN COMPLETE / 90 DEG"};
    for(int i=0;i<4;++i) {
        Rover rover;rover.reset({{0,0},{1,0},{1,1}});rover.setSpeed(1);
        while(!rover.turning()) rover.update(0.005);
        rover.update(times[i]);place(rover,i);
        DrawText(labels[i],210+(i%2)*640,32+(i/2)*380,20,RAYWHITE);
    }
    save("18-wheel-steering.png");
    BeginTextureMode(target);ClearBackground(Color{174,83,47,255});
    for(int i=0;i<4;++i) {
        Rover rover;rover.reset({{0,0},{1,0},{2,0}});rover.setSpeed(i<2?0.25:5);
        rover.update(i%2?0.08:0);
        place(rover,i);
        DrawText(TextFormat("%.2f TILE/S / PHASE %.3f",rover.speed(),rover.wheelAngle()),170+(i%2)*640,370+(i/2)*380,20,RAYWHITE);
    }
    save("19-wheel-rolling.png");
    UnloadRenderTexture(target);
}
}
int main(int argc,char** argv) {
    std::filesystem::path output=argc>1?argv[1]:"build/visual-qa";
    std::filesystem::create_directories(output);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(1280,800,"Mars visual QA");
    if(!IsWindowReady()) return 1;
    SetTargetFPS(0);
    if(argc>2 && std::string(argv[2])=="--wheels-only") {
        captureWheelViews(output);CloseWindow();return 0;
    }
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
    auto capture=[&](const char* name,bool costs=false,std::optional<Cell> hovered=std::nullopt) {
        BeginTextureMode(target);ClearBackground(BLACK);
        renderer.draw(terrain,sim,camera,effects,viewport,hovered?hovered:sim.search().current()?std::optional<Cell>(sim.search().current()->cell):std::nullopt,costs);
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
    const auto trackHistory=effects.tracks().size();
    const Vector2 oldestContact=effects.tracks().empty()?Vector2{}:effects.tracks().front().to;
    auto rock=sim.dropRock(0);
    if(!rock) throw std::runtime_error("Expected DROP ROCK");
    terrain.edit(*rock,EditTool::ADD_ROCK);camera.noticeRock(WorldCamera::project(rock->x+0.5,rock->y+0.5));
    for(int i=0;i<8;++i) frame();capture("06-route-blocked");
    wait(SimulationStatus::RECALCULATING);effects.update(1.0f/30,sim,terrain.start);capture("07-recalculating");
    wait(SimulationStatus::PATH_REVEAL);wait(SimulationStatus::DRIVING);
    if(effects.tracks().size()<trackHistory || (trackHistory &&
       (effects.tracks().front().to.x!=oldestContact.x || effects.tracks().front().to.y!=oldestContact.y)))
        throw std::runtime_error("Replanning erased wheel history");
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
    // Art-direction views on the real LAB DEMO, with all normal navigation phases intact.
    terrain=Terrain({20,20,20,42});sim.replaceMap(terrain.grid,terrain.start,terrain.goal,false);
    effects.reset(42);feedback.reset();camera=WorldCamera(20,20,viewport);sim.setDrivingSpeed(2.5);sim.play();
    for(int i=0;i<20;++i) frame();capture("12-lab-searching");
    wait(SimulationStatus::DRIVING);for(int i=0;i<60;++i) frame();capture("13-lab-driving");
    camera.reset();capture("14-lab-overview");
    for(int i=0;!sim.rover().turning();++i) {if(i>6000 || sim.status()!=SimulationStatus::DRIVING) throw std::runtime_error("Expected rover turn");frame();}
    for(int i=0;i<5;++i) frame();capture("14a-rover-turn");
    wait(SimulationStatus::ARRIVAL);for(int i=0;i<25;++i) frame();capture("14b-beacon-arrival");
    wait(SimulationStatus::MISSION_COMPLETE);
    // Identical rocks, viewport and zoom: the rover must change painter order continuously.
    terrain=Terrain({20,20,0,42});terrain.grid.setBlocked({9,10});terrain.grid.setBlocked({10,10});terrain.grid.setBlocked({11,10});
    terrain.goal={18,18};
    for(int side=0;side<2;++side) {
        terrain.start={10,side?11:9};sim.replaceMap(terrain.grid,terrain.start,terrain.goal,false);
        effects.reset(42);feedback.reset();camera=WorldCamera(20,20,viewport);
        for(int i=0;i<80;++i) camera.update(0.05f,CameraPhase::FOLLOW,WorldCamera::project(terrain.start.x+0.5,terrain.start.y+0.5),0,true);
        capture(side?"15b-depth-front":"15a-depth-behind");
    }
    sim.enterEdit();capture("16-edit-map",false,Cell{10,11});
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
    captureWheelViews(output);
    renderer.shutdown();audio.shutdown();UnloadRenderTexture(target);CloseWindow();std::cout << "Visual smoke passed\n";
}
