#pragma once
#include "simulation.h"
#include <algorithm>
#include <cmath>
namespace SonarConfig {
constexpr float searchPeriod=3.4f,replanPeriod=2.7f,drivePeriod=7.2f;
constexpr float pingVolume=0.58f,pingPitch=1.0f,replanPingPitch=1.08f;
constexpr float sweepAlpha=0.55f,sweepRadius=115,trailRadians=0.22f,gridAlpha=0.075f;
constexpr float pingFrequency=780,pingDecay=0.105f,echoDelay=0.105f,echoGain=0.19f;
inline float period(SimulationStatus status) {
    if(status==SimulationStatus::SEARCHING) return searchPeriod;
    if(status==SimulationStatus::RECALCULATING) return replanPeriod;
    if(status==SimulationStatus::DRIVING) return drivePeriod;
    return 0;
}
}
// The visual and audio observers use the same deterministic clock.
class SonarClock {
public:
    void reset() { phase_=0;strength_=0; }
    bool update(float dt,float period,bool running) {
        if(!running || period<=0 || !std::isfinite(dt) || dt<=0) return false;
        strength_=1;phase_+=std::min(dt,0.1f)/period;
        bool ping=phase_>=1;
        if(ping) phase_-=std::floor(phase_);
        return ping;
    }
    void inactive() { strength_=0; }
    float angle() const { return phase_*6.28318530718f; }
    float strength() const { return strength_; }
private:
    float phase_=0,strength_=0;
};
