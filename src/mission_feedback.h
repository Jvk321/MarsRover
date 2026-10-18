#pragma once
#include "simulation.h"
#include "terrain.h"
#include "audio_system.h"
enum class FeedbackAction { NONE, START_MISSION, START_DEMO, NEW_MISSION, REPLAY, EDIT, RESET };
class MissionFeedback {
public:
    bool helpOpen=true,audioOpen=false,presentationOpen=false;
    std::size_t selectedPreset=0;
    void reset();
    void update(float dt,const Simulation& sim);
    bool terminal() const;
    bool blocksInput() const { return helpOpen || audioOpen || presentationOpen || terminal(); }
    float flash() const;
    double missionTime() const { return missionTime_; }
    double searchTime() const { return searchTime_; }
    double travelled() const { return travelled_; }
    std::size_t iterations() const { return iterations_; }
    FeedbackAction draw(Rectangle viewport,const Terrain& terrain,const Simulation& sim,AudioSystem& audio);
private:
    SimulationStatus previous_=SimulationStatus::READY,status_=SimulationStatus::READY;
    float statusAge_=100,overlayAge_=0;
    double missionTime_=0,searchTime_=0,travelled_=0,lastX_=0,lastY_=0;
    bool hadPose_=false;
    std::size_t iterations_=0,lastIterations_=0;
};
