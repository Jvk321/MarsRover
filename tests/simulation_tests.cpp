#include "simulation.h"
#include <cstdlib>
#include <iostream>
#include <cmath>

void checkImpl(bool condition, int line) {
    if (!condition) { std::cerr << "Simulation test failed at line " << line << '\n'; std::exit(1); }
}
#define check(...) checkImpl((__VA_ARGS__), __LINE__)
int main() {
    Grid grid(33,25);
    Simulation sim(grid,{0,0},{32,24});
    sim.setSpeed(4);
    sim.play();
    for (int i=0; sim.status()!=SimulationStatus::PATH_REVEAL; ++i) {
        check(i<33*25);
        sim.update(0.25);
    }
    check(sim.playing() && sim.revealed()==0 && sim.pathLength()==56);
    check(sim.search().path().front()==Cell{0,0} && sim.search().path().back()==Cell{32,24});
    check(sim.visible({0,0}) && !sim.visible({32,24}));
    const auto iterations=sim.search().iterations();
    const auto open=sim.search().open().size(), closed=sim.search().closed().size();
    for (int i=0;i<28;++i) sim.update(0.25);
    check(sim.revealed()==28 && sim.status()==SimulationStatus::PATH_REVEAL);
    sim.pause();
    sim.update(100);
    check(sim.revealed()==28 && !sim.playing());
    for (int i=0;i<5;++i) {
        sim.step();
        check(sim.revealed()==static_cast<std::size_t>(29+i));
        sim.update(10);
        check(sim.revealed()==static_cast<std::size_t>(29+i));
    }
    check(sim.search().iterations()==iterations);
    check(sim.search().open().size()==open && sim.search().closed().size()==closed);
    const auto& path=sim.search().path();
    for (std::size_t i=0;i<path.size();++i) check(sim.visible(path[i])==(i<=sim.revealed()));
    sim.play();
    while (sim.status()==SimulationStatus::PATH_REVEAL) sim.update(0.25);
    check(sim.status()==SimulationStatus::PATH_READY && sim.playing());
    check(sim.revealed()==56 && sim.visible({32,24}));
    sim.update(0.01);
    check(sim.status()==SimulationStatus::DRIVING && sim.playing());
    check(sim.rover().x()==0 && sim.rover().y()==0);
    sim.update(0.1);
    check(sim.rover().travelled()>0 && sim.rover().travelled()<1);
    sim.pause();
    const double distance=sim.rover().travelled(), wheel=sim.rover().wheelAngle();
    sim.update(100);
    check(sim.rover().travelled()==distance && sim.rover().wheelAngle()==wheel);
    sim.setDrivingSpeed(0.5);
    sim.play(); sim.update(0.1);
    check(std::abs(sim.rover().travelled()-distance-0.05)<1e-8);
    sim.setSpeed(60);
    sim.setDrivingSpeed(2);
    const double before=sim.rover().travelled();
    sim.update(0.1);
    check(std::abs(sim.rover().travelled()-before-0.2)<1e-8);
    for (int i=0;!sim.rover().turning();++i) { check(i<10000); sim.update(0.01); }
    sim.pause();
    const double angle=sim.rover().heading(), tilt=sim.rover().tilt();
    const double turnWheel=sim.rover().wheelAngle();
    sim.update(10);
    check(sim.rover().heading()==angle && sim.rover().tilt()==tilt && sim.rover().wheelAngle()==turnWheel);
    sim.play();
    for (int i=0;sim.status()!=SimulationStatus::MISSION_COMPLETE;++i) {
        check(i<10000); sim.update(0.05);
    }
    check(!sim.playing() && sim.rover().x()==32 && sim.rover().y()==24);
    check(sim.rover().travelled()==56 && sim.rover().complete());
    sim.step(); sim.play(); sim.update(100);
    check(sim.status()==SimulationStatus::MISSION_COMPLETE && !sim.playing());
    sim.reset();
    check(sim.status()==SimulationStatus::READY && sim.revealed()==0 && sim.pathLength()==0);
    // Reset during active reveal clears both the animation and A* state.
    sim.setSpeed(4);
    sim.play();
    while (sim.status()!=SimulationStatus::PATH_REVEAL) sim.update(0.25);
    sim.update(0.25);
    check(sim.revealed()==1);
    sim.reset(); sim.update(100);
    check(sim.status()==SimulationStatus::READY && !sim.playing());
    check(sim.revealed()==0 && sim.search().iterations()==0 && sim.search().path().empty());
    check(!sim.visible({0,0}));
    // Manual exploration transitions without also revealing a segment.
    while (sim.status()!=SimulationStatus::PATH_REVEAL) sim.step();
    check(sim.revealed()==0 && !sim.playing());
    sim.step(); check(sim.revealed()==1);
    while (sim.status()!=SimulationStatus::PATH_READY) sim.step();
    sim.play(); sim.update(0.01); sim.update(0.1);
    check(sim.status()==SimulationStatus::DRIVING && sim.rover().travelled()>0);
    sim.reset(); sim.update(100);
    check(sim.status()==SimulationStatus::READY && !sim.playing());
    check(!sim.rover().available() && sim.rover().travelled()==0 && sim.rover().wheelAngle()==0);
    Grid single(1,1);
    Simulation trivial(single,{0,0},{0,0});
    trivial.step();
    check(trivial.status()==SimulationStatus::PATH_READY && trivial.pathLength()==0);
    check(trivial.visible({0,0}));
    trivial.play(); trivial.update(0.1);
    check(trivial.status()==SimulationStatus::MISSION_COMPLETE && trivial.rover().complete());
    Grid blocked(3,1); blocked.setBlocked({1,0});
    Simulation noPath(blocked,{0,0},{2,0});
    noPath.play(); noPath.update(1);
    check(noPath.status()==SimulationStatus::NO_PATH && !noPath.playing());
    check(!noPath.visible({0,0}));
    std::cout << "Simulation tests passed\n";
}
