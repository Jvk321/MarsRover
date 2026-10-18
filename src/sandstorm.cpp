#include "sandstorm.h"
#include <algorithm>
#include <cmath>
namespace {
constexpr float pi=3.14159265358979323846f;
float wrap(float x) { return x-std::floor(x); }
float smooth(float x) { x=std::clamp(x,0.0f,1.0f);return x*x*(3-2*x); }
}
void Sandstorm::reset(std::uint32_t seed) {
    auto random=[&]() { seed=seed*1664525u+1013904223u;return (seed>>8)/16777216.0f; };
    auto init=[&](auto& pool) {
        for(auto& p:pool) p={{random(),random()},0.6f+random()*1.9f,0.3f+random()*0.7f,
                           0.6f+random()*1.3f,random()*2*pi};
    };
    init(near_);init(mid_);init(ground_);
    strength_=ramp_=gust_=time_=0;level_=static_cast<float>(intensity_);
}
StormConfig::Profile Sandstorm::profile() const {
    int lo=std::clamp(static_cast<int>(level_),0,2),hi=std::min(2,lo+1);
    float t=level_-lo;auto a=StormConfig::profiles[lo],b=StormConfig::profiles[hi];
    auto blend=[&](float x,float y){return x+(y-x)*t;};
    return {static_cast<int>(blend(a.nearCount,b.nearCount)),static_cast<int>(blend(a.midCount,b.midCount)),
            static_cast<int>(blend(a.groundCount,b.groundCount)),static_cast<int>(blend(a.nearGust,b.nearGust)),
            static_cast<int>(blend(a.midGust,b.midGust)),static_cast<int>(blend(a.groundGust,b.groundGust)),
            blend(a.haze,b.haze),blend(a.speed,b.speed)};
}
void Sandstorm::update(float dt) {
    if(!std::isfinite(dt) || dt<=0) return;
    dt=std::min(dt,0.1f);
    ramp_=std::clamp(ramp_+(enabled_?1:-1)*dt/StormConfig::transitionSeconds,0.0f,1.0f);
    strength_=smooth(ramp_);
    level_+=(static_cast<float>(intensity_)-level_)*(1-std::exp(-3*dt));
    if(strength_==0) { time_=0;gust_=0;return; }
    time_+=dt;
    float phase=std::fmod(time_,StormConfig::gustInterval);
    gust_=phase<StormConfig::gustDuration ? std::pow(std::sin(pi*phase/StormConfig::gustDuration),2.0f):0;
    float speed=profile().speed*(1+gust_*StormConfig::gustSpeedBoost);
    auto move=[&](auto& pool,float rate) {
        for(auto& p:pool) {
            p.position.x=wrap(p.position.x+StormConfig::windDirection.x*rate*p.speed*speed*dt);
            p.position.y=wrap(p.position.y+StormConfig::windDirection.y*rate*p.speed*speed*dt);
        }
    };
    move(near_,StormConfig::nearSpeed);move(mid_,StormConfig::midSpeed);
    for(auto& p:ground_) {
        p.position.x=wrap(p.position.x+StormConfig::windDirection.x*StormConfig::groundSpeed*p.speed*speed*dt);
        p.position.y=wrap(p.position.y+StormConfig::windDirection.y*2*StormConfig::groundSpeed*p.speed*speed*dt);
    }
}
int Sandstorm::nearCount() const { auto p=profile();return static_cast<int>((p.nearCount+(p.nearGust-p.nearCount)*gust_)*strength_); }
int Sandstorm::midCount() const { auto p=profile();return static_cast<int>((p.midCount+(p.midGust-p.midCount)*gust_)*strength_); }
int Sandstorm::groundCount() const { auto p=profile();return static_cast<int>((p.groundCount+(p.groundGust-p.groundCount)*gust_)*strength_); }
float Sandstorm::haze(Vector2 world,Vector2 focus,float zoom) const {
    // Camera-facing distance in projected space. Keep a clear pocket around the rover/path.
    float dx=(world.x-focus.x)*zoom,dy=(world.y-focus.y)*zoom;
    float distance=std::hypot(dx,dy);
    float depth=smooth((distance-StormConfig::clearRadius)/StormConfig::fadeDistance);
    float farSide=smooth((-dy+100)/450);
    return strength_*profile().haze*depth*(0.5f+0.5f*farSide)*(1+gust_*0.12f);
}
float Sandstorm::backgroundHaze(int layer) const {
    return strength_*profile().haze*(layer==0?0.95f:0.60f);
}
void Sandstorm::drawGround(int width,int height,Rectangle bounds) const {
    for(int i=0;i<groundCount();++i) {
        const auto& p=ground_[i];
        // Wrap in a projected square: both drift and streak share the screen-space wind angle.
        const float extent=(width+height)*32.0f;
        Vector2 center{p.position.x*extent-height*32.0f,p.position.y*extent*0.5f};
        float gx=center.x/64+center.y/32,gy=center.y/32-center.x/64;
        if(gx<0 || gy<0 || gx>=width || gy>=height) continue;
        if(!CheckCollisionPointRec(center,bounds)) continue;
        float envelope=std::pow(std::sin(p.position.x*pi),2.0f);
        float length=18+p.size*22+gust_*25;
        Vector2 wind=StormConfig::windDirection;
        // Projected world-space streaks lie on the same plane as the isometric grid.
        for(int band=0;band<3;++band) {
            Vector2 a{center.x-band*3,center.y+band*2};
            Vector2 b{a.x+wind.x*length,a.y+wind.y*length};
            DrawLineEx(a,b,1.0f+band*0.6f,Fade(Color{199,167,133,255},strength_*p.alpha*envelope*0.08f));
        }
    }
}
void Sandstorm::drawAir(Rectangle v) const {
    if(strength_<=0) return;
    auto draw=[&](const auto& pool,int count,bool near) {
        for(int i=0;i<count;++i) {
            const auto& p=pool[i];
            float edge=std::min({p.position.x,p.position.y,1-p.position.x,1-p.position.y})*18;
            float opacity=std::clamp(edge,0.0f,1.0f)*strength_*p.alpha*(near?0.25f:0.15f);
            float wave=near?0:std::sin(time_*1.4f+p.phase)*5;
            Vector2 head{v.x+p.position.x*v.width,v.y+p.position.y*v.height+wave};
            float length=near ? 10+p.speed*15+gust_*28 : 2+p.size*1.5f;
            Vector2 tail{head.x-length*StormConfig::windDirection.x,head.y-length*StormConfig::windDirection.y};
            if(near) DrawLineEx(tail,head,p.size+1.2f,Fade(Color{200,180,154,255},opacity*0.2f));
            DrawLineEx(tail,head,near?p.size*0.7f:0.75f,Fade(Color{220,201,171,255},opacity));
        }
    };
    draw(mid_,midCount(),false);draw(near_,nearCount(),true);
    // A handful of wide, feathered ribbons only during gusts, never a full-screen tint.
    if(gust_>0.05f) for(int i=0;i<4;++i) {
        const auto& p=near_[i];
        Vector2 a{v.x+p.position.x*v.width,v.y+p.position.y*v.height};
        Vector2 b{a.x+180,a.y+180*StormConfig::windDirection.y};
        for(int layer=3;layer>=1;--layer)
            DrawLineEx(a,b,static_cast<float>(layer*8),Fade(Color{186,163,139,255},gust_*strength_*0.012f));
    }
}
