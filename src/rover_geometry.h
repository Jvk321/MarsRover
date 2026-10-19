#pragma once
#include <cmath>
namespace RoverGeometry {
// Lengths are in logical grid tiles; angles are in radians.
constexpr double roverScale=1.0;
constexpr double wheelRadius=0.17*roverScale, wheelSpacing=0.43*roverScale, halfTrack=0.49*roverScale;
constexpr double suspensionAmount=0.012, steeringAngle=0.38;
// Wheelbase spans front to rear axle; trackWidth spans the two tire centerlines.
constexpr double wheelWidth=0.14*roverScale, wheelbase=2*wheelSpacing, trackWidth=2*halfTrack;
// Suspension knuckle inset from the inner tire face (visual geometry only).
constexpr double treadHeight=0.010*roverScale, suspensionOffset=0.04*roverScale, rearSteeringRatio=0.55;
constexpr int wheelSegments=16, treadSegmentCount=12;
inline double wheelSteering(double forward,double steer) {
    return forward>0?steer:forward<0?-steer*rearSteeringRatio:0;
}
struct Contact { double x,y; };
inline Contact contact(double centerX,double centerY,double heading,double forward,double side) {
    return {centerX+std::cos(heading)*forward-std::sin(heading)*side,
            centerY+std::sin(heading)*forward+std::cos(heading)*side};
}
inline double articulation(double phase,double f,double s) { return suspensionAmount*std::sin(phase*0.73+f*4+s*2); }
}
