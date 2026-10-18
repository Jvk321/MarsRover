#pragma once
#include "simulation.h"
#include "terrain.h"
#include "audio_system.h"
#include <string>
enum class FeedbackAction { NONE, NEW_MISSION, REPLAY, EDIT, RESET };
class MissionFeedback {
public:
    bool helpOpen=true,audioOpen=false;
    void reset();
    void update(float dt,const Simulation& sim);
    bool terminal() const;
    bool blocksInput() const { return helpOpen || audioOpen || terminal(); }
    float flash() const;
    double missionTime() const { return missionTime_; }
    double travelled() const { return travelled_; }
    std::size_t iterations() const { return iterations_; }
    FeedbackAction draw(Rectangle viewport,const Terrain& terrain,const Simulation& sim,AudioSystem& audio);
private:
    SimulationStatus previous_=SimulationStatus::READY,status_=SimulationStatus::READY;
    float statusAge_=100,overlayAge_=0;
    double missionTime_=0,travelled_=0,lastX_=0,lastY_=0;
    bool hadPose_=false;
    std::size_t iterations_=0,lastIterations_=0;
};
