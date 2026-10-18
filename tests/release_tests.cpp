#include "presentation.h"
#include "mission_feedback.h"
#include "visual_effects.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void checkImpl(bool value,int line) {
    if(!value) {std::cerr<<"Release QA failed at line "<<line<<'\n';std::exit(1);}
}
#define check(...) checkImpl((__VA_ARGS__),__LINE__)
void waitFor(Simulation& sim,SimulationStatus target,MissionFeedback& feedback) {
    for(int frame=0;sim.status()!=target;++frame) {
        check(frame<100000 && sim.active());
        sim.update(1.0/30);feedback.update(1.0f/30,sim);
    }
}
void fresh(const Simulation& sim) {
    check(sim.status()==SimulationStatus::READY && !sim.playing());
    check(sim.search().iterations()==0 && sim.search().path().empty() && sim.search().closed().empty());
    check(sim.replans()==0 && sim.revealed()==0 && !sim.rover().available() && sim.oldPath().empty());
}
}
int main() {
    for(const auto& preset:presentationPresets) {
        Terrain terrain(preset.terrain),same(preset.terrain);
        for(int y=0;y<terrain.grid.height();++y) for(int x=0;x<terrain.grid.width();++x)
            check(terrain.grid.blocked({x,y})==same.grid.blocked({x,y}));
        Simulation sim(terrain.grid,terrain.start,terrain.goal);
        MissionFeedback feedback;feedback.helpOpen=false;
        sim.setSpeed(preset.searchSpeed);sim.setDrivingSpeed(preset.drivingSpeed);
        sim.step();feedback.update(1.0f/60,sim);
        check(sim.search().iterations()==1 && !sim.playing());
        sim.update(10);check(sim.search().iterations()==1);
        double searchTime=feedback.searchTime();feedback.update(10,sim);
        check(feedback.searchTime()==searchTime);
        sim.play();waitFor(sim,SimulationStatus::PATH_REVEAL,feedback);
        check(sim.search().status()==SearchStatus::PATH_FOUND && sim.revealed()==0);
        check(sim.pathLength()>=static_cast<std::size_t>(terrain.grid.width()+terrain.grid.height()-6));
        int unsafe=0;
        for(std::size_t i=4;i+1<sim.search().path().size();++i) {
            Grid blocked=terrain.grid;blocked.setBlocked(sim.search().path()[i]);
            if(!connected(blocked,terrain.start,terrain.goal)) ++unsafe;
        }
        std::cout<<preset.name<<": initial-route bottlenecks "<<unsafe<<'\n';
        check(unsafe==0);
        sim.step();check(sim.revealed()==1);
        sim.update(10);check(sim.revealed()==1);
        sim.play();waitFor(sim,SimulationStatus::DRIVING,feedback);
        // Two deliberate operator actions; nothing drops a rock automatically in the application.
        for(int replan=0;replan<2;++replan) {
            for(int i=0;i<20;++i) {sim.update(1.0/30);feedback.update(1.0f/30,sim);}
            check(sim.canDropRock());
            auto rock=sim.dropRock(0);check(rock.has_value());
            check(sim.status()==SimulationStatus::ROUTE_BLOCKED);
            waitFor(sim,SimulationStatus::RECALCULATING,feedback);
            Cell stopped=sim.rover().stoppedCell();
            check(sim.search().open().front().cell==stopped);
            sim.pause();sim.step();feedback.update(1.0f/60,sim);
            auto count=sim.search().iterations();check(count==1);
            sim.update(10);check(sim.search().iterations()==count);
            sim.play();waitFor(sim,SimulationStatus::PATH_REVEAL,feedback);
            check(sim.search().path().front()==stopped && sim.search().path().back()==terrain.goal);
            waitFor(sim,SimulationStatus::DRIVING,feedback);
        }
        waitFor(sim,SimulationStatus::MISSION_COMPLETE,feedback);
        check(sim.rover().stoppedCell()==terrain.goal && sim.replans()==2 && feedback.terminal());
        check(feedback.searchTime()>0 && feedback.missionTime()>feedback.searchTime());
        // REPLAY restores the baseline map, removing rocks injected during the mission.
        sim.replaceMap(terrain.grid,terrain.start,terrain.goal,false);feedback.reset();fresh(sim);
        check(feedback.searchTime()==0 && feedback.missionTime()==0);
        sim.play();waitFor(sim,SimulationStatus::MISSION_COMPLETE,feedback);
        check(sim.replans()==0 && sim.rover().stoppedCell()==terrain.goal);
        std::cout<<preset.name<<": deterministic map, two replans, completion and replay passed\n";
    }
    for(auto phase:{SimulationStatus::SEARCHING,SimulationStatus::PATH_REVEAL,SimulationStatus::DRIVING,SimulationStatus::RECALCULATING}) {
        Terrain baseline({20,20,0,42});Simulation sim(baseline.grid,baseline.start,baseline.goal);
        MissionFeedback feedback;feedback.helpOpen=false;sim.setSpeed(60);sim.play();
        if(phase==SimulationStatus::RECALCULATING) {
            waitFor(sim,SimulationStatus::DRIVING,feedback);check(sim.dropRock(0).has_value());
        }
        waitFor(sim,phase,feedback);
        VisualEffects effects;effects.reset(42);effects.update(0.1f,sim,baseline.start);
        sim.replaceMap(baseline.grid,baseline.start,baseline.goal,false);effects.reset(42);feedback.reset();
        fresh(sim);check(effects.tracks().empty() && effects.dust().empty() && effects.sonar().strength()==0);
    }
    for(auto config:{TerrainConfig{2,2,95,0},TerrainConfig{3,2,95,123},TerrainConfig{80,60,25,47}}) {
        auto begin=std::chrono::steady_clock::now();Terrain terrain(config);
        auto path=findPath(terrain.grid,terrain.start,terrain.goal);check(!path.empty());
        std::cout<<config.width<<'x'<<config.height<<" generator + A*: "
                 <<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()<<" ms\n";
    }
    Grid surrounded(5,5);for(Cell c:{Cell{1,2},Cell{3,2},Cell{2,1},Cell{2,3}}) surrounded.setBlocked(c);
    check(findPath(surrounded,{2,2},{4,4}).empty());
    check(findPath(surrounded,{0,0},{2,2}).empty());
    Simulation invalid(surrounded,{-1,0},{4,4});MissionFeedback feedback;feedback.helpOpen=false;
    feedback.update(0.1f,invalid);check(feedback.terminal() && !invalid.active());
    bool rejected=false;try {Terrain invalidMap({0,80,25,1});} catch(const std::invalid_argument&) {rejected=true;}
    check(rejected);
    std::cout<<"Release QA passed\n";
}
