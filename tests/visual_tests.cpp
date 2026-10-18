#include "visual_effects.h"
#include "visual_config.h"
#include "scene_renderer.h"
#include <cstdlib>
#include <iostream>
void checkImpl(bool value,int line) {
    if(!value) { std::cerr << "Visual test failed at " << line << '\n';std::exit(1); }
}
#define check(...) checkImpl((__VA_ARGS__),__LINE__)
int main() {
    check(SceneRenderer::tileHash(3,8,42)==SceneRenderer::tileHash(3,8,42));
    check(SceneRenderer::tileHash(3,8,42)!=SceneRenderer::tileHash(3,8,43));
    Simulation sim(Grid(80,60),{1,1},{78,58});sim.setSpeed(60);sim.setDrivingSpeed(3);
    VisualEffects effects;effects.reset(42);sim.play();
    sim.update(1.0/60);effects.update(1.0f/60,sim,{1,1});
    check(!effects.scans().empty() && effects.scans()[0].strength==1);
    sim.pause();float time=effects.time(),age=effects.scans()[0].age;
    effects.update(1,sim,{1,1});
    check(effects.time()==time && effects.scans()[0].age==age);
    sim.play();
    for(int i=0;sim.status()!=SimulationStatus::DRIVING;++i) {
        check(i<20000);sim.update(1.0/60);effects.update(1.0f/60,sim,{1,1});
    }
    for(int i=0;i<120;++i) { sim.update(1.0/60);effects.update(1.0f/60,sim,{1,1}); }
    check(!effects.tracks().empty() && !effects.dust().empty());
    sim.pause();auto tracks=effects.tracks().size(),dust=effects.dust().size();
    time=effects.time();age=effects.dust()[0].age;
    effects.update(100,sim,{1,1});
    check(effects.time()==time && effects.tracks().size()==tracks && effects.dust().size()==dust);
    check(effects.dust()[0].age==age);
    effects.setStorm(true);check(effects.storm());sim.play();
    for(int i=0;sim.status()!=SimulationStatus::MISSION_COMPLETE;++i) {
        check(i<30000);sim.update(1.0/60);effects.update(1.0f/60,sim,{1,1});
        check(effects.dust().size()<=VisualConfig::maxDust);
        check(effects.tracks().size()<=VisualConfig::maxTracks);
    }
    effects.setStorm(false);check(!effects.storm());
    effects.reset(42);check(effects.dust().empty() && effects.tracks().empty() && effects.scans().empty());
    std::cout << "Visual state, deterministic art, pause and limits passed\n";
}
