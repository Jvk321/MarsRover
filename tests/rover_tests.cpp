#include "rover.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
void checkImpl(bool condition, int line) {
    if (!condition) { std::cerr << "Rover test failed at line " << line << '\n'; std::exit(1); }
}
#define check(...) checkImpl((__VA_ARGS__), __LINE__)
bool near(double a,double b) { return std::abs(a-b)<1e-8; }
int main() {
    constexpr double pi=3.14159265358979323846;
    Rover rover;
    rover.setSpeed(1);
    rover.reset({{0,0},{1,0},{1,1},{0,1},{0,0}});
    check(near(rover.heading(),0));
    rover.update(0.25);
    check(near(rover.x(),0.25) && near(rover.y(),0));
    check(rover.wheelAngle()>0 && !rover.turning());
    rover.update(0.95);
    check(near(rover.x(),1) && near(rover.y(),0) && rover.turning());
    const double wheel=rover.wheelAngle();
    rover.update(0.225);
    check(near(rover.heading(),pi/4) && near(rover.x(),1) && near(rover.y(),0));
    check(near(rover.wheelAngle(),wheel) && std::abs(rover.tilt())>0);
    rover.update(0.225);
    check(near(rover.heading(),pi/2) && !rover.turning() && near(rover.tilt(),0));
    rover.update(0.5);
    check(near(rover.x(),1) && near(rover.y(),0.5));
    rover.setSpeed(2); rover.update(0.35);
    check(near(rover.y(),1) && rover.turning());
    rover.update(0.45); check(near(rover.heading(),pi));
    rover.update(0.6); rover.update(0.45);
    check(near(std::remainder(rover.heading(),2*pi),-pi/2));
    rover.update(0.5);
    check(!rover.complete() && near(rover.y(),0.25));
    rover.update(0.5);
    check(rover.complete() && near(rover.x(),0) && near(rover.y(),0));
    double finalWheel=rover.wheelAngle(); rover.update(100);
    check(near(rover.wheelAngle(),finalWheel) && near(rover.travelled(),4));
    Rover a,b;
    a.reset({{0,0},{1,0},{1,1}}); b.reset({{0,0},{1,0},{1,1}});
    a.update(1.25);
    for (int i=0;i<125;++i) b.update(0.01);
    check(near(a.x(),b.x()) && near(a.y(),b.y()) && near(a.heading(),b.heading()));
    for (Cell goal : {Cell{2,1},Cell{0,1},Cell{1,0},Cell{1,2}}) {
        rover.reset({{1,1},goal}); rover.setSpeed(1); rover.update(0.1);
        check(near(rover.x(),1+(goal.x-1)*0.0975));
        check(near(rover.y(),1+(goal.y-1)*0.0975));
    }
    // Rolling phase is distance based at both speed limits and after braking.
    for(double speed:{0.25,5.0}) {
        rover.reset({{0,0},{1,0},{2,0}});rover.setSpeed(speed);
        rover.update(0.1);
        check(near(rover.wheelAngle(),rover.x()/RoverGeometry::wheelRadius));
        const double phase=rover.wheelAngle();rover.update(0);
        check(near(phase,rover.wheelAngle()));
        rover.brake();rover.updateBrake(100);
        check(near(rover.wheelAngle(),1/RoverGeometry::wheelRadius));
    }
    rover.reset({{0,0},{1,0},{1,1}});rover.setSpeed(1);rover.update(1.24);
    check(rover.steering()>0 && near(rover.heading(),0));
    rover.reset();check(near(rover.steering(),0));
    rover.reset(); check(!rover.available() && near(rover.wheelAngle(),0));
    std::cout << "Rover tests passed\n";
}


