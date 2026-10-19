#include "mission_hud.h"
#include "scene_renderer.h"
#include "terrain_panel.h"
#include "rlgl.h"
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

int main(int argc,char** argv) {
    auto output=std::filesystem::path(argc>1?argv[1]:"build/ui-qa");std::filesystem::create_directories(output);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(1920,1080,"Liquid Glass HUD QA");
    if(!IsWindowReady()) return 1;SetTargetFPS(0);Ui::init();
    if(!Ui::glassReady()) throw std::runtime_error("HIGH glass shader must compile for visual QA");
    std::cout<<"Windows content scale "<<GetWindowScaleDPI().x<<" / native client-pixel UI\n";
    SceneRenderer scene;AudioSystem audio;audio.settings.enabled=false;
    for(auto size:{Vector2{1920,1080},Vector2{2560,1440}}) {
        SetWindowSize(int(size.x),int(size.y));
        if(GetScreenWidth()!=int(size.x) || GetScreenHeight()!=int(size.y)) throw std::runtime_error("Unexpected UI canvas size");
        const auto layout=Ui::layout(size.x,size.y);
        Terrain terrain({80,60,18,47});Simulation sim(terrain.grid,terrain.start,terrain.goal);
        sim.setSpeed(60);sim.setDrivingSpeed(5);
        WorldCamera camera(80,60,layout.world);VisualEffects effects;effects.reset(47);
        MissionFeedback feedback;feedback.helpOpen=false;MissionHud hud;
        hud.settings.searchSpeed=60;hud.settings.driveSpeed=5;
        auto tick=[&]() {
            sim.update(1.0/30);effects.update(1.0f/30,sim,terrain.start);feedback.update(1.0f/30,sim);
            CameraPhase phase=CameraPhase::WIDE;
            if(sim.status()==SimulationStatus::PATH_REVEAL) phase=CameraPhase::REVEAL;
            if(sim.status()==SimulationStatus::DRIVING) phase=CameraPhase::FOLLOW;
            if(sim.status()==SimulationStatus::ROUTE_BLOCKED) phase=CameraPhase::BLOCKED;
            if(sim.status()==SimulationStatus::RECALCULATING) phase=CameraPhase::REPLAN;
            if(sim.status()==SimulationStatus::ARRIVAL) phase=CameraPhase::ARRIVAL;
            if(sim.status()==SimulationStatus::MISSION_COMPLETE) phase=CameraPhase::COMPLETE;
            camera.update(1.0f/30,phase,WorldCamera::project(sim.rover().x()+0.5,sim.rover().y()+0.5),sim.rover().heading(),sim.playing());
        };
        auto frame=[&](const char* file=nullptr,TerrainPanel* generator=nullptr) {
            Ui::beginFrame(1.0f/60,int(size.x),int(size.y));BeginDrawing();ClearBackground(BLACK);
            Ui::beginWorld();scene.draw(terrain,sim,camera,effects,layout.canvas,std::nullopt,false,false);Ui::endWorld();Ui::drawWorld();
            hud.draw(layout,sim,terrain,camera,effects,audio,feedback,feedback.blocksInput() || generator,false);
            scene.drawMinimap(terrain,sim,camera,layout.world,layout.minimap);
            if(generator) generator->draw();else feedback.draw(layout.world,terrain,sim,audio);
            if(file) {
                rlDrawRenderBatchActive();Image image=LoadImageFromScreen();
                auto path=output/(std::to_string(int(size.y))+"-"+file+".png");ExportImage(image,path.string().c_str());UnloadImage(image);
                std::cout<<"Captured "<<path.string()<<'\n';
            }
            EndDrawing();
        };
        auto capture=[&](const char* file,TerrainPanel* generator=nullptr) {for(int i=0;i<24;++i) frame(i==23?file:nullptr,generator);};
        auto wait=[&](SimulationStatus target) {
            for(int i=0;sim.status()!=target;++i) {
                if(i>30000 || !sim.active()) throw std::runtime_error("UI scenario could not reach state");tick();
            }
        };
        // Exercise the actual widgets using raylib's in-process event playback,
        // without sending mouse or keyboard events to the Windows desktop.
        // AutomationEventType values from raylib 6.2 rcore.c.
        auto tap=[&](Vector2 point,bool blocked=false) {
            PlayAutomationEvent({0,5,{MOUSE_BUTTON_LEFT,0,0,0}});
            BeginDrawing();EndDrawing();
            PlayAutomationEvent({0,7,{int(point.x),int(point.y),0,0}});
            PlayAutomationEvent({0,6,{MOUSE_BUTTON_LEFT,0,0,0}});
            if(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) throw std::runtime_error("raylib automation mouse press unavailable");
            auto a=hud.input(layout,sim,terrain,camera,effects,audio,blocked,false);
            PlayAutomationEvent({0,5,{MOUSE_BUTTON_LEFT,0,0,0}});
            BeginDrawing();EndDrawing();return a;
        };
        const float s=layout.scale;
        auto playbackPoint=[&](int index) {return Vector2{layout.bar.x+(16+index*112+52)*s,layout.bar.y+48*s};};
        if(!tap(playbackPoint(0)).has(HudEvent::PLAY)) throw std::runtime_error("PLAY hit target");
        sim.play();tick();
        if(!tap(playbackPoint(1)).has(HudEvent::PAUSE)) throw std::runtime_error("PAUSE hit target");
        sim.pause();auto iterations=sim.search().iterations();
        if(!tap(playbackPoint(2)).has(HudEvent::STEP)) throw std::runtime_error("STEP hit target");
        sim.step();if(sim.search().iterations()!=iterations+1) throw std::runtime_error("STEP changed more than one A* iteration");
        if(tap(playbackPoint(0),true).has(HudEvent::PLAY)) throw std::runtime_error("Modal input leaked to PLAY");
        const float fieldX=layout.settings.x+20*s,fieldY=layout.settings.y+48*s,fieldWidth=layout.settings.width-40*s;
        auto tabPoint=[&](int index) {return Vector2{layout.tabs.x+6*s+(index+0.5f)*(layout.tabs.width-12*s)/4,layout.tabs.y+21*s};};
        tap(tabPoint(3));if(hud.selectedTab()!=HudTab::AUDIO) throw std::runtime_error("AUDIO tab hit target");
        tap({fieldX+fieldWidth-29,fieldY+22*s});if(!audio.settings.enabled) throw std::runtime_error("SOUND switch");
        tap({fieldX+fieldWidth-29,fieldY+22*s});
        tap({fieldX+fieldWidth*0.73f,fieldY+120*s});
        if(std::abs(audio.settings.master-0.73f)>0.015f) throw std::runtime_error("MASTER slider");
        tap(tabPoint(2));
        if(!tap({fieldX+fieldWidth-29,fieldY+22*s}).has(HudEvent::STORM)) throw std::runtime_error("SANDSTORM switch");
        auto intensity=tap({fieldX+fieldWidth*0.85f,fieldY+130*s});
        if(intensity.stormLevel!=2) throw std::runtime_error("Storm intensity control");
        tap({fieldX+fieldWidth*0.25f,fieldY+247*s});if(Ui::quality()!=Ui::Quality::LOW) throw std::runtime_error("LOW quality control");
        tap({fieldX+fieldWidth*0.75f,fieldY+247*s});if(Ui::quality()!=Ui::Quality::HIGH) throw std::runtime_error("HIGH quality control");
        tap(tabPoint(1));
        if(!tap({fieldX+fieldWidth-29,fieldY+22*s}).has(HudEvent::AUTO_CAMERA)) throw std::runtime_error("AUTO CAMERA switch");
        if(!tap({fieldX+fieldWidth/2,fieldY+190*s}).has(HudEvent::FIT_CAMERA)) throw std::runtime_error("FIT TO MAP control");
        tap(tabPoint(0));
        tap({fieldX+fieldWidth-29,fieldY+254*s});if(!hud.settings.showCosts) throw std::runtime_error("SHOW COSTS switch");
        hud.settings.showCosts=false;
        if(!tap(playbackPoint(3)).has(HudEvent::RESET)) throw std::runtime_error("RESET hit target");
        sim.reset();effects.reset(47);feedback.reset();camera.reset();
        std::cout<<int(size.y)<<"p widget input: playback, single STEP, sliders, switches, tabs and modal blocking passed\n";
        feedback.helpOpen=true;capture("01-help");feedback.helpOpen=false;
        sim.play();for(int i=0;i<15;++i) tick();capture("02-searching");
        sim.pause();capture("03-paused");sim.play();
        wait(SimulationStatus::PATH_REVEAL);for(int i=0;i<20;++i) tick();capture("04-path-reveal");
        wait(SimulationStatus::DRIVING);for(int i=0;i<90;++i) tick();capture("05-driving");
        // Follow the same verified drop timing as the world visual smoke test.
        for(int i=0;i<290;++i) tick();
        auto rock=sim.dropRock(0);if(!rock) throw std::runtime_error("DROP ROCK not available");
        terrain.edit(*rock,EditTool::ADD_ROCK);camera.noticeRock(WorldCamera::project(rock->x+0.5,rock->y+0.5));
        for(int i=0;i<8;++i) tick();capture("06-route-blocked");
        wait(SimulationStatus::RECALCULATING);for(int i=0;i<3;++i) tick();capture("07-recalculating");
        sim.pause();capture("08-recalculating-paused");sim.step();effects.update(1.0f/60,sim,terrain.start);sim.play();
        wait(SimulationStatus::PATH_REVEAL);wait(SimulationStatus::DRIVING);wait(SimulationStatus::MISSION_COMPLETE);
        for(int i=0;i<30;++i) feedback.update(1.0f/30,sim);capture("09-mission-complete");
        Ui::quality(Ui::Quality::LOW);capture("10-glass-low");Ui::quality(Ui::Quality::HIGH);
        sim.enterEdit();feedback.reset();effects.reset(47);camera.reset();capture("11-edit-map");
        TerrainPanel generator(terrain.config);generator.open=true;capture("12-generator",&generator);
        feedback.audioOpen=true;capture("13-audio");feedback.audioOpen=false;
        feedback.presentationOpen=true;capture("14-presentation");feedback.presentationOpen=false;
        hud.selectTab(HudTab::CAMERA);capture("15-camera");
        camera.setAuto(false);capture("15b-camera-manual");camera.setAuto(true);
        hud.selectTab(HudTab::ENVIRONMENT);effects.setStorm(true);effects.setStormIntensity(StormIntensity::HEAVY);
        for(int i=0;i<100;++i) effects.update(1.0f/30,sim,terrain.start);
        capture("16-environment-high");Ui::quality(Ui::Quality::LOW);capture("17-environment-low");Ui::quality(Ui::Quality::HIGH);
        effects.setStorm(false);for(int i=0;i<100;++i) effects.update(1.0f/30,sim,terrain.start);
        hud.selectTab(HudTab::AUDIO);capture("18-inline-audio");hud.selectTab(HudTab::MAP);
        // Measure complete frame composition: world texture, 3D rover, UI and local blur.
        for(auto level:{Ui::Quality::LOW,Ui::Quality::HIGH}) {
            Ui::quality(level);std::vector<double> times;times.reserve(80);
            for(int i=0;i<90;++i) {
                auto begin=std::chrono::steady_clock::now();frame();
                if(i>=10) times.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
            }
            std::sort(times.begin(),times.end());
            std::cout<<int(size.y)<<"p glass "<<(level==Ui::Quality::HIGH?"HIGH":"LOW")<<" median "<<times[times.size()/2]<<" ms, p95 "<<times[times.size()*95/100]<<" ms\n";
        }
        Ui::quality(Ui::Quality::HIGH);
        terrain=Terrain({18,3,0,47});
        for(int x=0;x<18;++x) {terrain.grid.setBlocked({x,0});terrain.grid.setBlocked({x,2});}
        terrain.start={0,1};terrain.goal={17,1};const Terrain corridorMission=terrain;
        sim.replaceMap(terrain.grid,terrain.start,terrain.goal,false);
        camera=WorldCamera(18,3,layout.world);effects.reset(47);feedback.reset();sim.play();
        wait(SimulationStatus::DRIVING);auto blocked=sim.dropRock(0);
        if(!blocked) throw std::runtime_error("NO_ROUTE test could not block route");
        terrain.edit(*blocked,EditTool::ADD_ROCK);wait(SimulationStatus::NO_ROUTE);capture("19-no-route");
        terrain=corridorMission;sim.replaceMap(terrain.grid,terrain.start,terrain.goal,false);
        effects.reset(47);feedback.reset();camera.reset();capture("20-reset");
    }
    scene.shutdown();Ui::shutdown();CloseWindow();std::cout<<"Liquid Glass UI smoke passed\n";
}
