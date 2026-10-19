#include "rover_mesh.h"
#include "rover_geometry.h"
#include "raymath.h"
#include <vector>
#include <cstring>
#include <cmath>

namespace {
class Builder {
public:
    Builder() {positions.reserve(12000);normals.reserve(12000);colors.reserve(16000);}
    void triangle(Vector3 a,Vector3 b,Vector3 c,Vector3 normal,Color color) {
        if(Vector3DotProduct(Vector3CrossProduct(Vector3Subtract(b,a),Vector3Subtract(c,a)),normal)<0) std::swap(b,c);
        normal=Vector3Normalize(normal);
        for(auto p:{a,b,c}) {
            positions.insert(positions.end(),{p.x,p.y,p.z});
            normals.insert(normals.end(),{normal.x,normal.y,normal.z});
            colors.insert(colors.end(),{color.r,color.g,color.b,color.a});
        }
    }
    void quad(Vector3 a,Vector3 b,Vector3 c,Vector3 d,Vector3 normal,Color color) {
        triangle(a,b,c,normal,color);triangle(a,c,d,normal,color);
    }
    void box(Vector3 center,Vector3 size,Matrix rotation,Color color) {
        Vector3 points[8];
        for(int i=0;i<8;++i) points[i]=Vector3Transform({center.x+(i&1?1:-1)*size.x/2,
            center.y+(i&2?1:-1)*size.y/2,center.z+(i&4?1:-1)*size.z/2},rotation);
        const int faces[][4]{{0,2,6,4},{1,5,7,3},{0,4,5,1},{2,3,7,6},{0,1,3,2},{4,6,7,5}};
        const Vector3 ns[]{{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
        for(int i=0;i<6;++i) quad(points[faces[i][0]],points[faces[i][1]],points[faces[i][2]],points[faces[i][3]],Vector3Transform(ns[i],rotation),color);
    }
    Mesh upload() const {
        Mesh mesh{};mesh.vertexCount=static_cast<int>(positions.size()/3);mesh.triangleCount=mesh.vertexCount/3;
        mesh.vertices=static_cast<float*>(MemAlloc(positions.size()*sizeof(float)));
        mesh.normals=static_cast<float*>(MemAlloc(normals.size()*sizeof(float)));
        mesh.colors=static_cast<unsigned char*>(MemAlloc(colors.size()));
        mesh.texcoords=static_cast<float*>(MemAlloc(mesh.vertexCount*2*sizeof(float)));
        std::memcpy(mesh.vertices,positions.data(),positions.size()*sizeof(float));
        std::memcpy(mesh.normals,normals.data(),normals.size()*sizeof(float));
        std::memcpy(mesh.colors,colors.data(),colors.size());
        std::memset(mesh.texcoords,0,mesh.vertexCount*2*sizeof(float));
        UploadMesh(&mesh,false);return mesh;
    }
private:
    std::vector<float> positions,normals;
    std::vector<unsigned char> colors;
};
Vector3 point(float axial,float r,float angle) {return {r*std::sin(angle),r*std::cos(angle),axial};}
void ring(Builder& b,float axial,float outer,float inner,float sign,Color color) {
    for(int i=0;i<RoverGeometry::wheelSegments;++i) {
        const float a=i*2*PI/RoverGeometry::wheelSegments,c=(i+1)*2*PI/RoverGeometry::wheelSegments;
        b.quad(point(axial,outer,a),point(axial,outer,c),point(axial,inner,c),point(axial,inner,a),{0,0,sign},color);
    }
}
}
Mesh buildRoverWheelMesh() {
    const float r=RoverGeometry::wheelRadius,body=r-RoverGeometry::treadHeight,half=RoverGeometry::wheelWidth/2;
    Builder mesh;
    const float x[]{-half,-half*0.88f,-half*0.60f,half*0.60f,half*0.88f,half};
    const float radius[]{body*0.82f,body*0.95f,body,body,body*0.95f,body*0.82f};
    for(int band=0;band<5;++band) for(int i=0;i<RoverGeometry::wheelSegments;++i) {
        const float a=i*2*PI/RoverGeometry::wheelSegments,b=(i+1)*2*PI/RoverGeometry::wheelSegments,m=(a+b)/2;
        Vector3 normal{std::sin(m),std::cos(m),-(radius[band+1]-radius[band])/(x[band+1]-x[band])};
        mesh.quad(point(x[band],radius[band],a),point(x[band],radius[band],b),
                  point(x[band+1],radius[band+1],b),point(x[band+1],radius[band+1],a),normal,{41,48,54,255});
    }
    const float treadX[]{-half*0.90f,-half*0.57f,half*0.57f,half*0.90f};
    const float treadR[]{body*0.94f,body,body,body*0.94f};
    for(int i=0;i<RoverGeometry::treadSegmentCount;++i) {
        float m=i*2*PI/RoverGeometry::treadSegmentCount;
        float a=m-PI/RoverGeometry::treadSegmentCount*0.32f,b=m+PI/RoverGeometry::treadSegmentCount*0.32f;
        for(int band=0;band<3;++band) {
            float u=treadX[band],v=treadX[band+1],lo=treadR[band],hi=treadR[band+1],raise=RoverGeometry::treadHeight;
            Vector3 radial{std::sin(m),std::cos(m),-(hi-lo)/(v-u)},tangent{std::cos(m),-std::sin(m),0};
            mesh.quad(point(u,lo+raise,a),point(u,lo+raise,b),point(v,hi+raise,b),point(v,hi+raise,a),radial,{75,82,84,255});
            mesh.quad(point(u,lo,a),point(u,lo+raise,a),point(v,hi+raise,a),point(v,hi,a),Vector3Negate(tangent),{43,49,55,255});
            mesh.quad(point(u,lo,b),point(u,lo+raise,b),point(v,hi+raise,b),point(v,hi,b),tangent,{54,62,66,255});
        }
        for(int end=0;end<2;++end) {
            int j=end?3:0;
            mesh.quad(point(treadX[j],treadR[j],a),point(treadX[j],treadR[j],b),
                      point(treadX[j],treadR[j]+RoverGeometry::treadHeight,b),point(treadX[j],treadR[j]+RoverGeometry::treadHeight,a),
                      {0,0,end?1.0f:-1.0f},{63,70,74,255});
        }
    }
    for(float sign:{-1.0f,1.0f}) {
        const float side=sign*half;
        ring(mesh,side,body*0.82f,r*0.61f,sign,{40,46,52,255});
        ring(mesh,side+sign*0.002f,r*0.61f,r*0.51f,sign,{188,201,205,255});
        ring(mesh,side+sign*0.003f,r*0.51f,0,sign,{42,55,66,255});
        for(int i=0;i<6;++i) mesh.box({0,r*0.32f,side+sign*0.008f},{r*0.095f,r*0.40f,r*0.045f},MatrixRotateZ(-i*2*PI/6),{199,209,207,255});
        const float cap=side+sign*0.024f;
        for(int i=0;i<12;++i) {
            float a=i*2*PI/12,b=(i+1)*2*PI/12,m=(a+b)/2;
            mesh.quad(point(side,r*0.20f,a),point(side,r*0.20f,b),point(cap,r*0.20f,b),point(cap,r*0.20f,a),
                      {std::sin(m),std::cos(m),0},{137,155,163,255});
        }
        ring(mesh,cap,r*0.20f,r*0.07f,sign,{203,212,207,255});
        ring(mesh,cap+sign*0.001f,r*0.07f,0,sign,{67,109,124,255});
    }
    return mesh.upload();
}
