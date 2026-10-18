#include "mission_feedback.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
void checkImpl(bool value,int line) {
    if(!value) {std::cerr<<"Feedback test failed at "<<line<<'\n';std::exit(1);}
}
#define check(...) checkImpl((__VA_ARGS__),__LINE__)
int main() {
    Sandstorm storm;storm.reset(42);AudioState audio;MissionFeedback feedback;feedback.helpOpen=false;
    Simulation sim(Grid(80,60),{1,1},{78,58});sim.setSpeed(60);sim.setDrivingSpeed(5);
    int found=0,success=0,blocked=0,recalc=0,scans=0;
    auto frame=[&]() {
        sim.update(1.0/30);storm.update(1.0f/30);
        feedback.update(1.0f/30,sim);audio.update(1.0f/30,sim,storm);
        found+=audio.cue(AudioCue::PATH_FOUND);success+=audio.cue(AudioCue::SUCCESS);
        blocked+=audio.cue(AudioCue::BLOCKED);recalc+=audio.cue(AudioCue::RECALCULATING);scans+=audio.cue(AudioCue::SCAN);
    };
    auto wait=[&](SimulationStatus status) {
        for(int i=0;sim.status()!=status;++i) {check(i<30000 && sim.active());frame();}
    };
    sim.play();wait(SimulationStatus::DRIVING);
    check(found==1 && scans>0);
    for(int i=0;i<30;++i) frame();check(audio.engine()>0);
    sim.pause();double clock=feedback.missionTime(),travel=feedback.travelled();
    for(int i=0;i<60;++i) {feedback.update(1.0f/30,sim);audio.update(1.0f/30,sim,storm);}
    check(feedback.missionTime()==clock && feedback.travelled()==travel && audio.engine()<0.001f);
    sim.play();
    for(int repeat=0;repeat<3;++repeat) {
        check(sim.dropRock(0).has_value());
        feedback.update(1.0f/30,sim);audio.update(1.0f/30,sim,storm);
        check(audio.cue(AudioCue::BLOCKED));
        audio.update(1.0f/30,sim,storm);check(!audio.cue(AudioCue::BLOCKED));
        wait(SimulationStatus::RECALCULATING);
        sim.pause();audio.update(1.0f/30,sim,storm);check(!audio.cue(AudioCue::RECALCULATING));
        sim.step();feedback.update(1.0f/30,sim);audio.update(1.0f/30,sim,storm);
        sim.play();wait(SimulationStatus::DRIVING);for(int i=0;i<10;++i) frame();
    }
    check(found==4 && recalc==3);
    wait(SimulationStatus::MISSION_COMPLETE);
    check(success==1 && feedback.terminal() && feedback.travelled()>100 && feedback.iterations()>0);
    for(int i=0;i<60;++i) {audio.update(1.0f/30,sim,storm);check(!audio.cue(AudioCue::SUCCESS));}
    for(auto intensity:{StormIntensity::LIGHT,StormIntensity::MEDIUM,StormIntensity::HEAVY}) {
        storm.setIntensity(intensity);storm.setEnabled(true);
        for(int i=0;i<180;++i) {storm.update(1.0f/30);audio.update(1.0f/30,sim,storm);}
        check(audio.wind()>AudioConfig::windGain && audio.windPitch()==1);
    }
    sim.reset();feedback.reset();audio.reset();
    check(feedback.missionTime()==0 && feedback.travelled()==0 && feedback.iterations()==0 && !feedback.terminal());
    check(audio.engine()==0 && !audio.cue(AudioCue::SUCCESS));
    // Reset during recalculation, and a separate no-alternative corridor.
    sim.play();wait(SimulationStatus::DRIVING);check(sim.dropRock(0).has_value());wait(SimulationStatus::RECALCULATING);
    sim.reset();audio.reset();feedback.reset();frame();check(sim.replans()==0 && feedback.travelled()==0);
    Grid narrow(18,3);for(int x=0;x<18;++x) {narrow.setBlocked({x,0});narrow.setBlocked({x,2});}
    sim.replaceMap(narrow,{0,1},{17,1},false);audio.reset();feedback.reset();sim.play();wait(SimulationStatus::DRIVING);
    check(sim.dropRock(0).has_value());wait(SimulationStatus::NO_ROUTE);
    check(audio.cue(AudioCue::ERROR) && feedback.terminal() && audio.engine()<0.01f);
    audio.update(1.0f/30,sim,storm);check(!audio.cue(AudioCue::ERROR));
    // Normal mission without replans.
    sim.replaceMap(Grid(20,20),{1,1},{18,18},false);audio.reset();feedback.reset();sim.play();wait(SimulationStatus::MISSION_COMPLETE);
    check(sim.replans()==0 && feedback.terminal() && std::abs(feedback.travelled()-34)<0.01);
    std::cout<<"Audio events, pause, reset, summary and mission scenarios passed\n";
}
