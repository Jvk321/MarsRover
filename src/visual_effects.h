#pragma once
#include "simulation.h"
#include "raylib.h"
#include "sandstorm.h"
#include "sonar.h"
#include <deque>
#include <random>
struct TrackMark { Vector2 from,to; float heading; };
struct DustParticle { Vector2 position,velocity; float height,age,life,size; };
class VisualEffects {
public:
    void reset(std::uint32_t seed);
    void update(float dt,const Simulation& sim,Cell start);
    void setStorm(bool enabled) { sandstorm_.setEnabled(enabled); }
    bool storm() const { return sandstorm_.enabled(); }
    void setStormIntensity(StormIntensity level) { sandstorm_.setIntensity(level); }
    const Sandstorm& sandstorm() const { return sandstorm_; }
    float time() const { return time_; }
    const std::deque<TrackMark>& tracks() const { return tracks_; }
    const std::vector<DustParticle>& dust() const { return dust_; }
    const SonarClock& sonar() const { return sonar_; }
private:
    std::deque<TrackMark> tracks_;
    std::vector<DustParticle> dust_;
    SonarClock sonar_;
    std::mt19937 random_{42};
    Sandstorm sandstorm_;
    bool hadPose_=false,wasMoving_=false;
    Vector2 lastPose_{},trackPose_{};
    float time_=0,emission_=0;
    SimulationStatus previous_=SimulationStatus::READY;
    float randomFloat();
};
