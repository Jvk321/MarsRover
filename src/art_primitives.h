#pragma once
#include "world_camera.h"
#include "visual_config.h"
#include "sandstorm.h"
#include <algorithm>
#include <cmath>

// Procedural drawing primitives. All positions/heights are projected world pixels.
namespace Art {
constexpr float pi=3.14159265358979323846f;
inline Vector2 project(float x,float y) { return WorldCamera::project(x,y); }
inline Vector2 lerp(Vector2 a,Vector2 b,float t) { return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t}; }
inline Color mix(Color a,Color b,float t) {
    t=std::clamp(t,0.0f,1.0f);
    return {static_cast<unsigned char>(a.r+(b.r-a.r)*t),static_cast<unsigned char>(a.g+(b.g-a.g)*t),
            static_cast<unsigned char>(a.b+(b.b-a.b)*t),static_cast<unsigned char>(a.a+(b.a-a.a)*t)};
}
inline Color fog(Color c,float amount) {
    Color haze=StormConfig::hazeColor;haze.a=c.a;return mix(c,haze,amount);
}
inline std::uint32_t hash(int x,int y,std::uint32_t seed) {
    std::uint32_t h=seed ^ (static_cast<std::uint32_t>(x)*0x9e3779b9u) ^ (static_cast<std::uint32_t>(y)*0x85ebca6bu);
    h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;return h^(h>>16);
}
inline float noise(float x,float y,std::uint32_t seed,float scale=6) {
    x/=scale;y/=scale;int ix=static_cast<int>(std::floor(x)),iy=static_cast<int>(std::floor(y));
    float fx=x-ix,fy=y-iy;fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy);
    auto n=[&](int a,int b){return (hash(a,b,seed)%10000)/10000.0f;};
    return (n(ix,iy)*(1-fx)+n(ix+1,iy)*fx)*(1-fy)+(n(ix,iy+1)*(1-fx)+n(ix+1,iy+1)*fx)*fy;
}
inline void triangle(Vector2 a,Vector2 b,Vector2 c,Color color) {
    if((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x)>0) std::swap(b,c);
    DrawTriangle(a,b,c,color);
}
inline void quad(Vector2 a,Vector2 b,Vector2 c,Vector2 d,Color color) {
    triangle(a,b,c,color);triangle(a,c,d,color);
}
inline void diamond(Cell c,float inset,Color color,bool outline=false,float thickness=1) {
    Vector2 p[]{project(c.x+inset,c.y+inset),project(c.x+1-inset,c.y+inset),
                project(c.x+1-inset,c.y+1-inset),project(c.x+inset,c.y+1-inset)};
    if(outline) for(int i=0;i<4;++i) DrawLineEx(p[i],p[(i+1)%4],thickness,color);
    else quad(p[0],p[1],p[2],p[3],color);
}
inline void ring(Vector2 center,float radius,Color color,float thickness=1,int segments=40) {
    Vector2 last{center.x+radius,center.y};
    for(int i=1;i<=segments;++i) {
        float angle=i*2*pi/segments;
        Vector2 next{center.x+std::cos(angle)*radius,center.y+std::sin(angle)*radius*0.5f};
        DrawLineEx(last,next,thickness,color);last=next;
    }
}
inline void shadow(Vector2 p,float width,float height,float opacity=VisualConfig::shadowAlpha) {
    Vector2 tail{p.x+VisualConfig::shadowDirection.x*height*VisualConfig::shadowLength,
                 p.y+VisualConfig::shadowDirection.y*height*VisualConfig::shadowLength};
    for(int i=2;i>=0;--i) {
        float w=width*(1+i*0.14f);
        quad({p.x-w,p.y},{p.x+w,p.y},{tail.x+w*0.8f,tail.y},{tail.x-w*0.8f,tail.y},
             Fade(Color{32,17,25,255},opacity/(i?5.0f:2.5f)));
    }
    DrawEllipse(static_cast<int>(p.x),static_cast<int>(p.y+2),width,width*0.35f,Fade(Color{26,17,23,255},opacity*0.55f));
}
}
