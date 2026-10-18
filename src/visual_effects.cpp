#include "visual_effects.h"
#include "visual_config.h"
#include <algorithm>
#include <cmath>
float VisualEffects::randomFloat() { return (random_()%10000)/10000.0f; }
void VisualEffects::reset(std::uint32_t seed) {
    tracks_.clear();dust_.clear();scans_.clear();random_.seed(seed);
    hadPose_=false;wasMoving_=false;time_=emission_=scanTime_=0;previous_=SimulationStatus::READY;
    sandstorm_.reset(seed);
}
void VisualEffects::update(float dt,const Simulation& sim,Cell start) {
    const auto& rover=sim.rover();
    Vector2 pose{static_cast<float>(rover.available()?rover.x():start.x)+0.5f,
                 static_cast<float>(rover.available()?rover.y():start.y)+0.5f};
    const float dx=pose.x-lastPose_.x,dy=pose.y-lastPose_.y;
    const float distance=hadPose_?std::hypot(dx,dy):0;
    const bool moved=distance>0.00001f;
    const bool searching=sim.status()==SimulationStatus::SEARCHING || sim.status()==SimulationStatus::RECALCULATING;
    const bool phaseChanged=previous_!=sim.status();
    const bool running=sim.playing() || sim.status()==SimulationStatus::READY || sim.status()==SimulationStatus::EDIT_MAP;
    float tick=running?std::clamp(dt,0.0f,0.1f):(moved || (searching && phaseChanged) ? 1.0f/60:0);
    time_+=tick;
    sandstorm_.update(tick);
    if (!hadPose_) { trackPose_=pose;lastPose_=pose;hadPose_=true; }
    if (moved && rover.available()) {
        float remaining=std::hypot(pose.x-trackPose_.x,pose.y-trackPose_.y);
        int samples=0;
        while(remaining>=VisualConfig::trackSpacing && samples++<64) {
            Vector2 next{trackPose_.x+(pose.x-trackPose_.x)/remaining*VisualConfig::trackSpacing,
                         trackPose_.y+(pose.y-trackPose_.y)/remaining*VisualConfig::trackSpacing};
            tracks_.push_back({trackPose_,next,static_cast<float>(rover.heading())});
            if(tracks_.size()>VisualConfig::maxTracks) tracks_.pop_front();
            trackPose_=next;remaining=std::hypot(pose.x-trackPose_.x,pose.y-trackPose_.y);
        }
    }
    for(auto& p:dust_) { p.age+=tick;p.position.x+=p.velocity.x*tick;p.position.y+=p.velocity.y*tick;p.height+=7*tick; }
    dust_.erase(std::remove_if(dust_.begin(),dust_.end(),[](const auto& p){return p.age>=p.life;}),dust_.end());
    const bool turning=rover.available() && rover.turning() && sim.status()==SimulationStatus::DRIVING;
    if(tick>0 && (moved || turning)) {
        emission_+=tick*(turning?VisualConfig::turnDustRate:VisualConfig::dustRate)*(1+sandstorm_.strength()*0.5f);
        if(moved && !wasMoving_) emission_+=8;
        int spawn=std::min(32,static_cast<int>(emission_));emission_-=spawn;
        for(int i=0;i<spawn && dust_.size()<VisualConfig::maxDust;++i) {
            float c=static_cast<float>(std::cos(rover.heading())),s=static_cast<float>(std::sin(rover.heading()));
            float side=(i%2?1.0f:-1.0f)*0.3f;
            Vector2 pos{pose.x-c*0.28f-s*side,pose.y-s*0.28f+c*side};
            // Particles are in projected world pixels, separate from logical grid coordinates.
            pos={(pos.x-pos.y)*32,(pos.x+pos.y)*16};
            dust_.push_back({pos,{VisualConfig::windX*0.3f+(randomFloat()-0.5f)*14,
                                  -4+(randomFloat()-0.5f)*8},0,0,
                             VisualConfig::dustLife*(0.7f+randomFloat()*0.6f),2+randomFloat()*3});
        }
    }
    for(auto& scan:scans_) scan.age+=tick;
    scans_.erase(std::remove_if(scans_.begin(),scans_.end(),[](const auto& s){return s.age>=VisualConfig::scanLife;}),scans_.end());
    if(searching) {
        scanTime_+=tick;
        if(phaseChanged || scanTime_>=VisualConfig::scanInterval) {
            if(scans_.size()<4) scans_.push_back({{(pose.x-pose.y)*32,(pose.x+pose.y)*16},0,phaseChanged?1.0f:0.65f});
            scanTime_=0;
        }
    } else scanTime_=0;
    wasMoving_=moved;lastPose_=pose;previous_=sim.status();
}
