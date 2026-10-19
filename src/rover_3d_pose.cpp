#include "rover_3d_pose.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace { constexpr float root2=1.41421356237f; }
Vector3 Rover3D::gridToWorld(double x,double y,double height) {
    return {static_cast<float>(x*Rover3DConfig::worldGridScale),static_cast<float>(height*Rover3DConfig::worldGridScale),
            static_cast<float>(y*Rover3DConfig::worldGridScale)};
}
Vector2 Rover3D::worldToMap(Vector3 p,float w,float h) {
    const float scale=Rover3DConfig::worldGridScale;
    const float elevation=std::clamp(h/w,0.01f,0.99f);
    return {(p.x-p.z)*w/scale,((p.x+p.z)*h-p.y*root2*w*std::sqrt(1-elevation*elevation))/scale};
}
float Rover3D::antennaYaw(float angle) {return angle-PI/4;}
Rover3D::Pose Rover3D::pose(const Rover& rover,float scanAngle,Motion motion) {
    const float scale=Rover3DConfig::worldGridScale;
    Pose result{};result.position=gridToWorld(rover.x()+0.5,rover.y()+0.5);
    Matrix root=MatrixMultiply(MatrixRotateY(-static_cast<float>(rover.heading())),MatrixTranslate(result.position.x,result.position.y,result.position.z));
    Matrix body=MatrixMultiply(MatrixRotateX(-static_cast<float>(rover.tilt())),MatrixRotateZ(motion.pitch));
    body=MatrixMultiply(body,MatrixTranslate(0,motion.bob*scale,0));
    const float bodyScale=scale*RoverGeometry::roverScale;
    result.chassis=MatrixMultiply(MatrixMultiply(MatrixScale(bodyScale,bodyScale,bodyScale),body),root);
    std::size_t i=0;
    for(double side:{-RoverGeometry::halfTrack,RoverGeometry::halfTrack})
        for(double f:{-RoverGeometry::wheelSpacing,0.0,RoverGeometry::wheelSpacing}) {
            const auto p=RoverGeometry::contact(rover.x()+0.5,rover.y()+0.5,rover.heading(),f,side);
            const float steer=static_cast<float>(RoverGeometry::wheelSteering(f,rover.steering()));
            const float angle=static_cast<float>(rover.heading())+steer;
            const double phase=rover.wheelAngle(side);
            Vector3 center=gridToWorld(p.x,p.y,RoverGeometry::wheelRadius);
            Matrix wheel=MatrixMultiply(MatrixScale(scale,scale,scale),MatrixRotateZ(-static_cast<float>(std::remainder(phase,2*PI))));
            wheel=MatrixMultiply(wheel,MatrixRotateY(-angle));
            wheel=MatrixMultiply(wheel,MatrixTranslate(center.x,center.y,center.z));
            result.wheels[i++]={center,gridToWorld(p.x,p.y),{std::cos(angle),0,std::sin(angle)},
                {-std::sin(angle),0,std::cos(angle)},wheel,phase,steer};
        }
    result.antenna=Vector3Transform({Rover3DConfig::antennaForward,Rover3DConfig::antennaHeight,Rover3DConfig::antennaSide},result.chassis);
    result.antennaYaw=antennaYaw(scanAngle);
    return result;
}
Camera3D Rover3D::camera(Vector2 anchor,float pixelsPerGrid,Vector2 surface,float ratio) {
    const float elevation=std::clamp(ratio,0.01f,0.99f),cosine=std::sqrt(1-elevation*elevation);
    const float ppu=std::max(0.001f,pixelsPerGrid)/Rover3DConfig::worldGridScale;
    const Vector3 right{1/root2,0,-1/root2},up{-elevation/root2,cosine,-elevation/root2};
    Vector3 target=Vector3Add(Vector3Scale(right,(surface.x/2-anchor.x)/ppu),Vector3Scale(up,(anchor.y-surface.y/2)/ppu));
    Vector3 eye=Vector3Add(target,{cosine*12/root2,elevation*12,cosine*12/root2});
    return {eye,target,{0,1,0},surface.y/ppu,CAMERA_ORTHOGRAPHIC};
}
