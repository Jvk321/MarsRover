#include "rover_3d_pose.h"
#include "visual_effects.h"
#include "visual_config.h"
#include "raymath.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
constexpr double pi=3.14159265358979323846;
void require(bool value,int line) {
    if(!value) {std::cerr<<"3D rover test failed at "<<line<<'\n';std::exit(1);}
}
#define check(...) require((__VA_ARGS__),__LINE__)
bool near(double a,double b,double tolerance=1e-5) {return std::abs(a-b)<tolerance;}
void same(Vector3 a,Vector3 b) {check(Vector3Distance(a,b)<0.00005f);}
}
int main() {
    using namespace RoverGeometry;
    const float world=Rover3DConfig::worldGridScale;
    check(near(wheelbase,2*wheelSpacing) && near(trackWidth,2*halfTrack));
    check(near(VisualConfig::trackWidth,wheelWidth));
    for(Cell direction:{Cell{1,0},Cell{0,1},Cell{-1,0},Cell{0,-1}}) {
        Rover rover;rover.reset({{5,5},{5+direction.x,5+direction.y}});
        auto pose=Rover3D::pose(rover);
        check(pose.wheels.size()==6);
        int index=0;
        for(double side:{-halfTrack,halfTrack}) for(double f:{-wheelSpacing,0.0,wheelSpacing}) {
            const auto& wheel=pose.wheels[index++];
            same(wheel.rolling,{static_cast<float>(direction.x),0,static_cast<float>(direction.y)});
            check(near(Vector3DotProduct(wheel.rolling,wheel.axle),0));
            auto contact=RoverGeometry::contact(5.5,5.5,rover.heading(),f,side);
            same(wheel.contact,Rover3D::gridToWorld(contact.x,contact.y));
            check(near(wheel.center.y-wheelRadius*world,0));
            same(Vector3Transform({0,0,0},wheel.transform),wheel.center);
            auto inner=Vector3Transform({0,0,-static_cast<float>(wheelWidth)/2},wheel.transform);
            auto outer=Vector3Transform({0,0,static_cast<float>(wheelWidth)/2},wheel.transform);
            check(near(Vector3Distance(inner,outer),wheelWidth*world));
            same(Vector3Normalize(Vector3Subtract(outer,inner)),wheel.axle);
            check(halfTrack-std::abs(std::sin(wheel.steering))*wheelRadius-std::abs(std::cos(wheel.steering))*wheelWidth/2>0.33*roverScale);
        }
    }
    for(double speed:{0.25,5.0}) {
        Rover rover;rover.reset({{0,0},{1,0},{2,0}});rover.setSpeed(speed);
        for(int frame=0;frame<50;++frame) {
            const double before=rover.x(),phase=Rover3D::pose(rover).wheels[4].phase;
            rover.update(1.0/120);
            const auto wheel=Rover3D::pose(rover).wheels[4];
            check(near(wheel.phase-phase,(rover.x()-before)/wheelRadius,1e-8));
            // Undo the spin to select the geometric bottom point, then apply the
            // complete spin/steering/body yaw transform: it must meet the ground.
            auto bottom=Vector3Transform({0,-static_cast<float>(wheelRadius),0},MatrixRotateZ(static_cast<float>(wheel.phase)));
            same(Vector3Transform(bottom,wheel.transform),wheel.contact);
        }
    }
    for(int turn:{-1,1}) {
        Rover rover;rover.reset({{0,0},{1,0},{1,turn}});rover.setSpeed(1);
        while(!rover.turning()) rover.update(0.005);
        rover.update(0.04);
        check(near(rover.heading(),0) && rover.steering()*turn>0);
        auto pose=Rover3D::pose(rover,1.3f,{0.01f,0.004f});
        check(near(pose.wheels[2].steering,rover.steering()));
        check(near(pose.wheels[0].steering,-rearSteeringRatio*rover.steering()));
        check(near(pose.wheels[1].steering,0));
        for(const auto& wheel:pose.wheels) check(near(wheel.contact.y,0));
        rover.update(0.185);
        check(near(rover.heading(),turn*pi/4,0.03));
        check(near(rover.wheelAngle(halfTrack)-rover.wheelAngle(),-halfTrack*rover.heading()/wheelRadius));
        check(near(rover.wheelAngle(-halfTrack)-rover.wheelAngle(),halfTrack*rover.heading()/wheelRadius));
        rover.update(1);rover.brake();rover.updateBrake(100);
        const auto phase=rover.wheelAngle(halfTrack),heading=rover.heading();
        auto cell=rover.stoppedCell();rover.followPath({cell,{cell.x+1,cell.y}});
        check(near(rover.wheelAngle(halfTrack),phase) && near(rover.heading(),heading));
    }
    // Project actual world-space contacts through Camera3D at several window
    // sizes, pans and zooms. Agreement with Camera2D keeps tires/tracks aligned.
    for(Vector2 size:{Vector2{1280,960},Vector2{1000,940},Vector2{1920,1080}})
        for(float zoom:{0.12f,1.0f,2.7f,4.0f}) {
            Vector2 anchor{size.x*0.63f,size.y*0.7f};
            auto camera=Rover3D::camera(anchor,32*std::sqrt(2.0f)*zoom,size);
            check(camera.projection==CAMERA_ORTHOGRAPHIC);
            for(Vector3 logical:{Vector3{0,0,0},Vector3{0.49f,0,0.43f},Vector3{-0.49f,0,-0.43f},Vector3{0,0.7f,0}}) {
                auto p=Rover3D::gridToWorld(logical.x,logical.z,logical.y);
                auto actual=GetWorldToScreenEx(p,camera,static_cast<int>(size.x),static_cast<int>(size.y));
                auto expected=Rover3D::worldToMap(p);
                check(near(actual.x,anchor.x+expected.x*zoom,0.003));
                check(near(actual.y,anchor.y+expected.y*zoom,0.003));
            }
        }
    for(float scan:{0.0f,PI/2,PI,PI*1.5f}) {
        float yaw=Rover3D::antennaYaw(scan);
        check(near((std::cos(yaw)-std::sin(yaw))/std::sqrt(2.0f),std::cos(scan)));
        check(near((std::cos(yaw)+std::sin(yaw))/std::sqrt(2.0f),std::sin(scan)));
    }
    Simulation sim(Grid(20,20),{1,1},{18,18});sim.setSpeed(60);sim.play();
    VisualEffects effects;effects.reset(42);
    for(int frame=0;sim.status()!=SimulationStatus::DRIVING;++frame) {
        check(frame<10000);sim.update(0.1);effects.update(0.1f,sim,{1,1});
    }
    sim.update(0.13);effects.update(0.1f,sim,{1,1});sim.pause();
    auto before=Rover3D::pose(sim.rover(),effects.sonar().angle(),effects.roverMotion());
    const auto tracks=effects.tracks().size();
    sim.update(50);effects.update(50,sim,{1,1});
    auto after=Rover3D::pose(sim.rover(),effects.sonar().angle(),effects.roverMotion());
    for(std::size_t i=0;i<before.wheels.size();++i) {
        same(before.wheels[i].center,after.wheels[i].center);
        same(before.wheels[i].rolling,after.wheels[i].rolling);
        check(near(before.wheels[i].phase,after.wheels[i].phase));
    }
    same(before.antenna,after.antenna);check(near(before.antennaYaw,after.antennaYaw));
    check(effects.tracks().size()==tracks);
    same(Vector3Transform({0.2f,0.7f,0.3f},before.chassis),Vector3Transform({0.2f,0.7f,0.3f},after.chassis));
    sim.reset();effects.reset(42);
    check(near(sim.rover().wheelAngle(),0) && near(effects.roverMotion().pitch,0) && near(effects.roverMotion().bob,0));
    std::cout<<"True 3D rover: six wheel transforms, contacts, rolling, steering, orthographic alignment, antenna, pause and reset passed\n";
}
