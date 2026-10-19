#pragma once
#include "rover.h"
#include "raylib.h"
#include <array>

namespace Rover3DConfig {
constexpr float worldGridScale=1.0f;
constexpr float bodyBob=0.005f,bodyPitch=0.018f;
constexpr float animationResponse=7.0f,brakeDustBurst=6;
constexpr float antennaForward=-0.30f,antennaSide=0.19f,antennaHeight=0.97f;
constexpr const char* bodyAsset="assets/models/rover_body.glb";
}
namespace Rover3D {
struct Motion { float pitch=0,bob=0; };
struct WheelPose {
    Vector3 center,contact,rolling,axle;
    Matrix transform;
    double phase;
    float steering;
};
struct Pose {
    Vector3 position;
    Matrix chassis;
    std::array<WheelPose,6> wheels;
    Vector3 antenna;
    float antennaYaw;
};
Vector3 gridToWorld(double x,double y,double height=0);
Vector2 worldToMap(Vector3 position,float halfWidth=32,float halfHeight=16);
float antennaYaw(float scanAngle);
Pose pose(const Rover& rover,float scanAngle=0,Motion motion={});
// Ground projection agrees exactly with the existing 2:1 diamond grid.
Camera3D camera(Vector2 screenAnchor,float pixelsPerGrid,Vector2 surfaceSize,float halfHeightRatio=0.5f);
}
