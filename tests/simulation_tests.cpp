#include "simulation.h"
#include <cstdlib>
#include <iostream>

void check(bool condition) {
    if (!condition) { std::cerr << "Simulation test failed\n"; std::exit(1); }
}
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
    check(sim.status()==SimulationStatus::PATH_READY && !sim.playing());
    check(sim.revealed()==56 && sim.visible({32,24}));
    sim.step(); sim.play(); sim.update(10);
    check(sim.revealed()==56 && !sim.playing());
    sim.reset();
    check(sim.status()==SimulationStatus::READY && sim.revealed()==0 && sim.pathLength()==0);
    // Reset during active reveal clears both the animation and A* state.
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
    Grid single(1,1);
    Simulation trivial(single,{0,0},{0,0});
    trivial.step();
    check(trivial.status()==SimulationStatus::PATH_READY && trivial.pathLength()==0);
    check(trivial.visible({0,0}));
    Grid blocked(3,1); blocked.setBlocked({1,0});
    Simulation noPath(blocked,{0,0},{2,0});
    noPath.play(); noPath.update(1);
    check(noPath.status()==SimulationStatus::NO_PATH && !noPath.playing());
    check(!noPath.visible({0,0}));
    std::cout << "Simulation tests passed\n";
}
