#pragma once
#include "rover_3d_pose.h"

// GPU resources load once, on first draw. Call shutdown before CloseWindow.
class RoverRenderer {
public:
    RoverRenderer()=default;
    ~RoverRenderer();
    RoverRenderer(const RoverRenderer&)=delete;
    RoverRenderer& operator=(const RoverRenderer&)=delete;
    void draw(const Rover& rover,Vector2 origin,float halfWidth,float halfHeight,float scanAngle=0,Rover3D::Motion motion={});
    void shutdown();
    bool ready() const {return ready_;}
    bool usesBodyAsset() const {return externalReady_;}
private:
    Mesh cube_{},cylinder_{},wheel_{};
    Material material_{};
    Shader shader_{},defaultShader_{};
    Model externalBody_{};
    int eyeLocation_=-1,emissionLocation_=-1;
    float lastEmission_=-1;
    bool ready_=false,externalReady_=false;
    void init();
    void part(const Mesh& mesh,Matrix transform,Color color,float emission=0);
    void rod(Vector3 from,Vector3 to,float radius,Color color);
    void body(const Rover3D::Pose& pose);
    void suspension(const Rover& rover,const Rover3D::Pose& pose);
    void shadows(const Rover3D::Pose& pose,Vector2 origin,float halfWidth,float halfHeight);
};
