#include "simulation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
void checkImpl(bool condition,int line) {
    if (!condition) { std::cerr << "Replanning test failed: " << line << '\n'; std::exit(1); }
}
#define check(...) checkImpl((__VA_ARGS__),__LINE__)
bool near(double a,double b) { return std::abs(a-b)<1e-8; }
void initialDrive(Simulation& sim) {
    for(int i=0;sim.status()!=SimulationStatus::PATH_READY;++i) { check(i<20000);sim.step(); }
    sim.play();sim.update(0.01);
    check(sim.status()==SimulationStatus::DRIVING);
}
void waitFor(Simulation& sim,SimulationStatus target) {
    for(int i=0;sim.status()!=target;++i) {
        check(i<30000 && sim.active());sim.update(0.02);
    }
}
int main() {
    Grid grid(80,60);
    Simulation sim(grid,{1,1},{78,58});
    sim.setSpeed(60);sim.setDrivingSpeed(2);
    initialDrive(sim);
    sim.update(0.15);
    for(int round=0;round<3;++round) {
        const double x=sim.rover().x(),y=sim.rover().y();
        const auto oldPath=sim.search().path();
        const auto oldLength=sim.pathLength();
        const auto previousSegment=sim.rover().segment();
        auto rock=sim.dropRock(0);
        check(rock && !(*rock==Cell{1,1}) && !(*rock==Cell{78,58}));
        check(*rock==oldPath[previousSegment+4]);
        check(sim.status()==SimulationStatus::ROUTE_BLOCKED);
        check(near(sim.rover().x(),x) && near(sim.rover().y(),y));
        check(!sim.dropRock(1)); // No second drop while the first is being handled.
        if (sim.rover().braking()) {
            double before=sim.rover().travelled();
            sim.update(0.01);double d1=sim.rover().travelled()-before;
            before=sim.rover().travelled();sim.update(0.01);
            check(sim.rover().travelled()-before<=d1+1e-9); // Deceleration.
        }
        waitFor(sim,SimulationStatus::RECALCULATING);
        const Cell stopped=sim.rover().stoppedCell();
        check(!(stopped==Cell{1,1}));
        check(near(sim.rover().x(),stopped.x) && near(sim.rover().y(),stopped.y));
        check(sim.replans()==static_cast<std::size_t>(round+1) && sim.oldLength()==oldLength);
        check(sim.search().iterations()==0 && sim.search().closed().empty());
        check(sim.search().open().size()==1 && sim.search().open()[0].cell==stopped);
        sim.pause(); auto wheels=sim.rover().wheelAngle();
        sim.update(100);
        check(sim.search().iterations()==0 && near(sim.rover().wheelAngle(),wheels));
        sim.step();check(sim.search().iterations()==1 && !sim.playing());
        sim.update(100);check(sim.search().iterations()==1);
        sim.play(); waitFor(sim,SimulationStatus::PATH_REVEAL);
        check(sim.search().path().front()==stopped && sim.search().path().back()==Cell{78,58});
        check(sim.revealed()==0 && near(sim.rover().x(),stopped.x));
        for(Cell c:sim.search().path()) for(Cell blocked:sim.droppedRocks()) check(!(c==blocked));
        waitFor(sim,SimulationStatus::PATH_READY);
        sim.update(0.01);check(sim.status()==SimulationStatus::DRIVING);
        check(near(sim.rover().x(),stopped.x) && near(sim.rover().y(),stopped.y));
        check(near(sim.rover().wheelAngle(),wheels));
        sim.update(0.1); // May turn before moving, but cannot teleport.
        check(std::hypot(sim.rover().x()-stopped.x,sim.rover().y()-stopped.y)<=0.2+1e-8);
    }
    sim.reset();
    check(sim.status()==SimulationStatus::READY && sim.replans()==0 && sim.oldPath().empty());
    check(!sim.rover().available() && sim.search().open()[0].cell==Cell{1,1});
    // Single-cell corridor: blocking any future tile leaves no alternative.
    Grid corridor(18,3);
    for(int x=0;x<18;++x) { corridor.setBlocked({x,0});corridor.setBlocked({x,2}); }
    Simulation trapped(corridor,{0,1},{17,1});
    trapped.setSpeed(60);initialDrive(trapped);trapped.update(0.1);
    check(trapped.dropRock(0).has_value());
    waitFor(trapped,SimulationStatus::RECALCULATING);
    waitFor(trapped,SimulationStatus::NO_ROUTE);
    check(!trapped.playing() && trapped.rover().available());
    double stop=trapped.rover().x();trapped.update(100);check(near(trapped.rover().x(),stop));
    trapped.reset();trapped.step();
    check(trapped.search().current()->cell==Cell{0,1});
    // Very short remaining routes cannot accept a safe DROP ROCK.
    Simulation shortRoute(Grid(3,1),{0,0},{2,0});initialDrive(shortRoute);
    check(!shortRoute.canDropRock() && !shortRoute.dropRock(1));
    std::cout << "Replanning tests passed\n";
}
