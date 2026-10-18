#pragma once
#include "simulation.h"
#include "raylib.h"
#include <deque>
#include <random>
struct TrackMark { Vector2 from,to; float heading; };
struct DustParticle { Vector2 position,velocity; float height,age,life,size; };
struct ScanPulse { Vector2 center; float age,strength; };
class VisualEffects {
public:
    void reset(std::uint32_t seed);
    void update(float dt,const Simulation& sim,Cell start);
    void setStorm(bool enabled) { storm_=enabled; }
    bool storm() const { return storm_; }
    float time() const { return time_; }
    const std::deque<TrackMark>& tracks() const { return tracks_; }
    const std::vector<DustParticle>& dust() const { return dust_; }
    const std::vector<ScanPulse>& scans() const { return scans_; }
private:
    std::deque<TrackMark> tracks_;
    std::vector<DustParticle> dust_;
    std::vector<ScanPulse> scans_;
    std::mt19937 random_{42};
    bool storm_=false,hadPose_=false,wasMoving_=false;
    Vector2 lastPose_{},trackPose_{};
    float time_=0,emission_=0,scanTime_=0;
    SimulationStatus previous_=SimulationStatus::READY;
    float randomFloat();
};
