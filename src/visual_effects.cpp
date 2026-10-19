#include "visual_effects.h"
#include "visual_config.h"
#include <algorithm>
#include <cmath>
float VisualEffects::randomFloat() { return (random_()%10000)/10000.0f; }
void VisualEffects::reset(std::uint32_t seed) {
    tracks_.clear();dust_.clear();dust_.reserve(VisualConfig::maxDust);sonar_.reset();random_.seed(seed);
    hadPose_=false;hadRover_=false;wasMoving_=false;trackRemainder_[0]=trackRemainder_[1]=0;lastHeading_=lastSteering_=0;dustSequence_=0;time_=emission_=0;previous_=SimulationStatus::READY;
    sandstorm_.reset(seed);
    roverMotion_={};lastSpeed_=0;wasBraking_=false;
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
    const bool running=sim.playing() || sim.status()==SimulationStatus::READY || sim.status()==SimulationStatus::EDIT_MAP || sim.status()==SimulationStatus::MISSION_COMPLETE;
    float tick=running?std::clamp(dt,0.0f,0.1f):(moved || (searching && phaseChanged) ? 1.0f/60:0);
    time_+=tick;
    sandstorm_.update(tick);
    const float heading=static_cast<float>(rover.heading());
    const float steering=static_cast<float>(rover.steering());
    if(tick>0) {
        const float speed=moved?distance/std::max(tick,0.0001f):0;
        const float acceleration=std::clamp((speed-lastSpeed_)/tick,-8.0f,8.0f);
        const float pitch=std::clamp(acceleration*0.003f,-Rover3DConfig::bodyPitch,Rover3DConfig::bodyPitch);
        roverMotion_.pitch+=(pitch-roverMotion_.pitch)*(1-std::exp(-Rover3DConfig::animationResponse*tick));
        roverMotion_.bob=Rover3DConfig::bodyBob*(std::sin(rover.wheelAngle()*0.8)+0.35*std::sin(rover.wheelAngle()*2.3))*std::clamp(speed*0.4f,0.0f,1.0f);
        lastSpeed_=speed;
    }
    const float delta=hadPose_ && hadRover_?std::remainder(heading-lastHeading_,6.283185307f):0;
    auto contact=[](Vector2 center,float angle,float side) {
        const auto p=RoverGeometry::contact(center.x,center.y,angle,-RoverGeometry::wheelSpacing,side);
        return Vector2{static_cast<float>(p.x),static_cast<float>(p.y)};
    };
    if (!hadPose_ || (rover.available() && !hadRover_)) {
        lastPose_=pose;lastHeading_=heading;lastSteering_=steering;hadPose_=true;
        for(int k=0;k<2;++k) trackPose_[k]=contact(pose,heading,(k?1:-1)*RoverGeometry::halfTrack);
    }
    // Subdivide contact arcs. Independent distance accumulators give the outer
    // tire more impressions without tying density to frame rate.
    const int steps=std::max(1,static_cast<int>(std::ceil((distance+std::abs(delta)*0.66f)/0.015f)));
    if(rover.available()) for(int j=1;j<=steps;++j) {
        const float t=float(j)/steps;
        Vector2 center{lastPose_.x+(pose.x-lastPose_.x)*t,lastPose_.y+(pose.y-lastPose_.y)*t};
        for(int k=0;k<2;++k) {
            Vector2 end=contact(center,lastHeading_+delta*t,(k?1:-1)*RoverGeometry::halfTrack);
            Vector2 begin=trackPose_[k];
            float length=std::hypot(end.x-begin.x,end.y-begin.y);
            while(length>0 && length+trackRemainder_[k]>=VisualConfig::TRACK_SAMPLE_DISTANCE) {
                float part=(VisualConfig::TRACK_SAMPLE_DISTANCE-trackRemainder_[k])/length;
                Vector2 next{begin.x+(end.x-begin.x)*part,begin.y+(end.y-begin.y)*part};
                const double toe=RoverGeometry::wheelSteering(-RoverGeometry::wheelSpacing,lastSteering_+(steering-lastSteering_)*t);
                tracks_.push_back({next,next,static_cast<float>(lastHeading_+delta*t+toe)});
                if(tracks_.size()>VisualConfig::maxTracks) tracks_.pop_front();
                begin=next;length=std::hypot(end.x-begin.x,end.y-begin.y);trackRemainder_[k]=0;
            }
            trackRemainder_[k]+=length;trackPose_[k]=end;
        }
    }
    for(auto& p:dust_) { p.age+=tick;p.position.x+=p.velocity.x*tick;p.position.y+=p.velocity.y*tick;p.height+=7*tick; }
    dust_.erase(std::remove_if(dust_.begin(),dust_.end(),[](const auto& p){return p.age>=p.life;}),dust_.end());
    const bool turning=rover.available() && rover.turning() && sim.status()==SimulationStatus::DRIVING;
    if(tick>0 && sim.playing() && (moved || turning)) {
        emission_+=tick*(turning?VisualConfig::turnDustRate:VisualConfig::dustRate*std::clamp(distance/std::max(tick,0.0001f)/2.0f,0.0f,1.5f))*(1+sandstorm_.strength()*0.5f);
        if(moved && !wasMoving_) emission_+=2;
        if(rover.braking() && !wasBraking_) emission_+=Rover3DConfig::brakeDustBurst;
        int spawn=std::min(32,static_cast<int>(emission_));emission_-=spawn;
        for(int i=0;i<spawn && dust_.size()<VisualConfig::maxDust;++i) {
            const auto rear=RoverGeometry::contact(pose.x,pose.y,rover.heading(),-RoverGeometry::wheelSpacing,
                (dustSequence_++%2?1:-1)*RoverGeometry::halfTrack);
            Vector2 pos=Rover3D::worldToMap(Rover3D::gridToWorld(rear.x,rear.y));
            // Particles are in projected world pixels, separate from logical grid coordinates.
            dust_.push_back({pos,{VisualConfig::windX*0.3f+(randomFloat()-0.5f)*14,
                                  -4+(randomFloat()-0.5f)*8},0,0,
                             VisualConfig::dustLife*(0.7f+randomFloat()*0.6f),2+randomFloat()*3});
        }
    }
    float period=SonarConfig::period(sim.status());
    if(period>0) sonar_.update(dt,period,sim.playing());else sonar_.inactive();
    hadRover_=rover.available();wasMoving_=moved;lastPose_=pose;lastHeading_=heading;lastSteering_=steering;previous_=sim.status();
    if(tick>0) wasBraking_=rover.braking();
}
