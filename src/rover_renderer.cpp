#include "rover_renderer.h"
#include "rover_mesh.h"
#include "art_primitives.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace {
constexpr const char* vertexShader=R"glsl(#version 330
in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
out vec3 normal;
out vec3 worldPosition;
out vec4 color;
out vec2 uv;
void main() {
    worldPosition=vec3(matModel*vec4(vertexPosition,1.0));
    normal=normalize(mat3(matNormal)*vertexNormal);
    color=vertexColor;
    uv=vertexTexCoord;
    gl_Position=mvp*vec4(vertexPosition,1.0);
})glsl";
constexpr const char* fragmentShader=R"glsl(#version 330
in vec3 normal;
in vec3 worldPosition;
in vec4 color;
in vec2 uv;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 viewPos;
uniform float emission;
out vec4 finalColor;
void main() {
    vec3 n=normalize(normal);
    vec3 sun=normalize(vec3(0.65,0.85,-0.35));
    float diffuse=max(dot(n,sun),0.0);
    vec3 ambient=mix(vec3(0.28,0.34,0.41),vec3(0.47,0.48,0.47),n.y*0.5+0.5);
    vec3 light=ambient+vec3(0.70,0.60,0.45)*diffuse;
    float rim=pow(1.0-max(dot(n,normalize(viewPos-worldPosition)),0.0),3.0);
    light+=vec3(0.29,0.17,0.07)*rim*(0.25+0.75*diffuse);
    vec4 base=texture(texture0,uv)*colDiffuse*color;
    finalColor=vec4(base.rgb*mix(light,vec3(1.18),emission),base.a);
})glsl";
Matrix shape(Vector3 size,Vector3 center,Matrix parent) {
    return MatrixMultiply(MatrixMultiply(MatrixScale(size.x,size.y,size.z),MatrixTranslate(center.x,center.y,center.z)),parent);
}
}
RoverRenderer::~RoverRenderer() {shutdown();}
void RoverRenderer::init() {
    if(ready_ || !IsWindowReady()) return;
    cube_=GenMeshCube(1,1,1);cylinder_=GenMeshCylinder(1,1,16);wheel_=buildRoverWheelMesh();
    material_=LoadMaterialDefault();defaultShader_=material_.shader;
    shader_=LoadShaderFromMemory(vertexShader,fragmentShader);
    if(shader_.id!=defaultShader_.id && IsShaderValid(shader_)) {
        material_.shader=shader_;
        eyeLocation_=GetShaderLocation(shader_,"viewPos");emissionLocation_=GetShaderLocation(shader_,"emission");
    }
    auto asset=std::filesystem::path(GetApplicationDirectory())/Rover3DConfig::bodyAsset;
    if(!FileExists(asset.string().c_str())) asset=Rover3DConfig::bodyAsset;
    if(FileExists(asset.string().c_str())) {
        externalBody_=LoadModel(asset.string().c_str());externalReady_=IsModelValid(externalBody_);
        if(!externalReady_) UnloadModel(externalBody_);
    }
    ready_=true;
}
void RoverRenderer::shutdown() {
    if(!ready_) return;
    if(externalReady_) UnloadModel(externalBody_);
    UnloadMesh(cube_);UnloadMesh(cylinder_);UnloadMesh(wheel_);
    material_.shader=defaultShader_;UnloadMaterial(material_);
    if(shader_.id!=defaultShader_.id) UnloadShader(shader_);
    ready_=externalReady_=false;lastEmission_=-1;
}
void RoverRenderer::part(const Mesh& mesh,Matrix transform,Color color,float emission) {
    if(emissionLocation_>=0 && emission!=lastEmission_) {
        SetShaderValue(shader_,emissionLocation_,&emission,SHADER_UNIFORM_FLOAT);lastEmission_=emission;
    }
    material_.maps[MATERIAL_MAP_DIFFUSE].color=color;DrawMesh(mesh,material_,transform);
}
void RoverRenderer::rod(Vector3 a,Vector3 b,float radius,Color color) {
    Vector3 delta=Vector3Subtract(b,a);float length=Vector3Length(delta);
    if(length<0.00001f) return;
    Matrix rotation=QuaternionToMatrix(QuaternionFromVector3ToVector3({0,1,0},Vector3Scale(delta,1/length)));
    Matrix transform=MatrixMultiply(MatrixMultiply(MatrixScale(radius,length,radius),rotation),MatrixTranslate(a.x,a.y,a.z));
    part(cylinder_,transform,color);
}
void RoverRenderer::body(const Rover3D::Pose& p) {
    auto box=[&](Vector3 size,Vector3 center,Color color,float glow=0) {part(cube_,shape(size,center,p.chassis),color,glow);};
    if(externalReady_) {
        for(int i=0;i<externalBody_.meshCount;++i) {
            Material material=externalBody_.materials[externalBody_.meshMaterial[i]];material.shader=material_.shader;
            DrawMesh(externalBody_.meshes[i],material,MatrixMultiply(externalBody_.transform,p.chassis));
        }
    } else {
        box({1.02f,0.13f,0.64f},{0,0.36f,0},{52,65,76,255});
        box({0.97f,0.18f,0.60f},{0,0.515f,0},{237,233,216,255});
        box({0.76f,0.025f,0.65f},{-0.02f,0.613f,0},{213,220,211,255});
        box({0.34f,0.095f,0.47f},{-0.19f,0.67f,0},{159,176,181,255});
        box({0.30f,0.014f,0.41f},{-0.19f,0.727f,0},{32,62,81,255});
        for(int i=0;i<5;++i) box({0.006f,0.004f,0.39f},{-0.31f+i*0.06f,0.737f,0},{101,163,180,255});
        for(float side:{-1.0f,1.0f}) {
            box({0.75f,0.018f,0.012f},{0,0.47f,side*0.306f},{67,80,88,255});
            for(int i=0;i<4;++i) box({0.038f,0.045f,0.012f},{-0.31f+i*0.065f,0.53f,side*0.308f},{66,83,91,255});
            box({0.07f,0.012f,0.014f},{0.32f,0.55f,side*0.310f},{240,135,49,255},0.25f);
        }
        box({0.022f,0.02f,0.62f},{0.50f,0.43f,0},{133,147,151,255});
        box({0.025f,0.10f,0.25f},{0.492f,0.54f,0},{65,81,90,255});
        box({0.029f,0.015f,0.13f},{0.508f,0.55f,0},{74,215,226,255},0.45f);
    }
    for(float side:{-0.23f,0.23f}) {
        box({0.045f,0.065f,0.082f},{0.495f,0.51f,side},{64,77,86,255});
        box({0.012f,0.042f,0.060f},{0.522f,0.515f,side},{255,239,191,255},0.80f);
    }
    // Tall 3D camera mast, bracing and two recessed forward-facing lenses.
    box({0.074f,0.045f,0.09f},{0.22f,0.65f,0},{72,89,99,255});
    box({0.041f,0.63f,0.041f},{0.22f,0.97f,0},{218,224,214,255});
    rod(Vector3Transform({0.11f,0.65f,0.06f},p.chassis),Vector3Transform({0.20f,1.12f,0.035f},p.chassis),0.009f*Rover3DConfig::worldGridScale,{61,75,85,255});
    box({0.28f,0.13f,0.28f},{0.25f,1.29f,0},{239,233,213,255});
    for(float side:{-0.080f,0.080f}) {
        box({0.018f,0.075f,0.074f},{0.395f,1.29f,side},{39,58,72,255});
        box({0.008f,0.043f,0.043f},{0.408f,1.29f,side},{45,121,150,255},0.28f);
    }
    box({0.08f,0.045f,0.075f},{0.24f,1.375f,0},{82,108,118,255});
    // Antenna is independent of body yaw. Its +X axis matches the sonar beam.
    Vector3 antennaBase=Vector3Transform({Rover3DConfig::antennaForward,0.66f,Rover3DConfig::antennaSide},p.chassis);
    rod(antennaBase,p.antenna,0.012f*Rover3DConfig::worldGridScale,{158,174,175,255});
    float scale=Rover3DConfig::worldGridScale*RoverGeometry::roverScale;
    Matrix antenna=MatrixMultiply(MatrixMultiply(MatrixScale(scale,scale,scale),MatrixRotateY(-p.antennaYaw)),MatrixTranslate(p.antenna.x,p.antenna.y,p.antenna.z));
    part(cube_,shape({0.07f,0.065f,0.19f},{0,0,0},antenna),{154,175,179,255});
    part(cube_,shape({0.015f,0.075f,0.23f},{0.035f,0,0},antenna),{210,220,209,255});
    part(cube_,shape({0.035f,0.024f,0.035f},{0.065f,0,0},antenna),{88,238,236,255},0.65f);
}
void RoverRenderer::suspension(const Rover& rover,const Rover3D::Pose& p) {
    const float world=Rover3DConfig::worldGridScale,scale=RoverGeometry::roverScale;
    for(int k=0;k<2;++k) {
        float sign=k?1:-1;
        float side=sign*(RoverGeometry::halfTrack-RoverGeometry::wheelWidth/2-RoverGeometry::suspensionOffset)/scale;
        Vector3 pivot=Vector3Transform({-0.04f,0.38f,side*0.82f},p.chassis);
        float compliance=static_cast<float>(RoverGeometry::articulation(rover.wheelAngle(),-0.22,side));
        Vector3 bogie=Vector3Transform({-0.22f,0.245f+compliance,side},p.chassis);
        rod(pivot,bogie,0.024f*world*scale,{118,140,147,255});
        for(int j=0;j<3;++j) {
            const auto& wheel=p.wheels[k*3+j];
            Vector3 hub=Vector3Add(wheel.center,Vector3Scale(wheel.axle,-sign*static_cast<float>(RoverGeometry::wheelWidth/2)*world));
            Vector3 knuckle=Vector3Add(wheel.center,Vector3Scale(wheel.axle,-sign*static_cast<float>(RoverGeometry::wheelWidth/2+RoverGeometry::suspensionOffset)*world));
            rod(j==2?pivot:bogie,knuckle,0.023f*world*scale,{143,158,159,255});
            rod(knuckle,hub,0.026f*world*scale,{63,80,92,255});
        }
        rod(Vector3Transform({-0.19f,0.40f,side*0.8f},p.chassis),bogie,0.010f*world*scale,{55,75,89,255});
    }
}
void RoverRenderer::shadows(const Rover3D::Pose& p,Vector2 origin,float w,float h) {
    auto screen=[&](Vector3 v) {auto a=Rover3D::worldToMap(v,w,h);return Vector2{origin.x+a.x,origin.y+a.y};};
    auto patch=[&](Vector3 center,Vector3 fore,Vector3 side,float length,float width,float alpha) {
        for(int layer=2;layer>=0;--layer) {
            float softness=1+layer*0.18f;
            for(int i=0;i<20;++i) {
                auto rim=[&](int j) {
                    float a=j*2*PI/20;
                    return screen(Vector3Add(center,Vector3Add(Vector3Scale(fore,std::sin(a)*length*softness),Vector3Scale(side,std::cos(a)*width*softness))));
                };
                Art::triangle(screen(center),rim(i),rim(i+1),Fade(Color{25,20,27,255},alpha/(layer?4:1)));
            }
        }
    };
    float scale=Rover3DConfig::worldGridScale*RoverGeometry::roverScale;
    Vector3 center=p.position;center.x-=0.09f*scale;center.z+=0.055f*scale;
    patch(center,p.wheels[1].rolling,p.wheels[1].axle,0.61f*scale,0.47f*scale,0.16f);
    for(const auto& wheel:p.wheels) patch(wheel.contact,wheel.rolling,wheel.axle,
        RoverGeometry::wheelRadius*0.55f*Rover3DConfig::worldGridScale,RoverGeometry::wheelWidth*0.58f*Rover3DConfig::worldGridScale,0.25f);
}
void RoverRenderer::draw(const Rover& rover,Vector2 origin,float w,float h,float scanAngle,Rover3D::Motion motion) {
    if(!rover.available()) return;
    init();if(!ready_) return;
    const auto pose=Rover3D::pose(rover,scanAngle,motion);
    shadows(pose,origin,w,h);
    // Enter a real depth-tested 3D pass, then restore the caller's 2D camera matrix.
    rlDrawRenderBatchActive();
    const Matrix saved=rlGetMatrixModelview();
    const auto ground=Rover3D::worldToMap(pose.position,w,h);
    Vector3 anchor=Vector3Transform({origin.x+ground.x,origin.y+ground.y,0},saved);
    const float zoom=std::hypot(saved.m0,saved.m1);
    Camera3D view=Rover3D::camera({anchor.x,anchor.y},w*std::sqrt(2.0f)*zoom,
                                {static_cast<float>(GetScreenWidth()),static_cast<float>(GetScreenHeight())},h/w);
    view.target=Vector3Add(view.target,pose.position);view.position=Vector3Add(view.position,pose.position);
    BeginMode3D(view);
    if(eyeLocation_>=0) SetShaderValue(shader_,eyeLocation_,&view.position.x,SHADER_UNIFORM_VEC3);
    body(pose);suspension(rover,pose);
    for(const auto& wheel:pose.wheels) part(wheel_,wheel.transform,WHITE);
    EndMode3D();rlSetMatrixModelview(saved);
}
