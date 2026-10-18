#pragma once
#include "astar.h"
#include "rover.h"

enum class SimulationStatus { EDIT_MAP, READY, SEARCHING, PATH_REVEAL, PATH_READY, DRIVING, ARRIVAL, MISSION_COMPLETE, NO_PATH, INVALID_ENDPOINTS };

// Playback and reveal state, independent of raylib and A* implementation details.
class Simulation {
public:
    Simulation(const Grid& grid, Cell start, Cell goal) : search_(grid, start, goal) { syncSearch(); }
    void reset();
    void replaceMap(const Grid& grid, Cell start, Cell goal, bool editing);
    void enterEdit();
    void play();
    void pause();
    void step();
    void update(double deltaSeconds);
    void setSpeed(double stepsPerSecond);
    void setDrivingSpeed(double speed) { rover_.setSpeed(speed); }
    const Rover& rover() const { return rover_; }
    bool active() const;
    bool playing() const { return playing_; }
    SimulationStatus status() const { return status_; }
    const AStarSearch& search() const { return search_; }
    std::size_t pathLength() const { return search_.path().empty() ? 0 : search_.path().size()-1; }
    std::size_t revealed() const { return revealed_; }
    bool visible(Cell cell) const;
private:
    AStarSearch search_;
    Rover rover_;
    SimulationStatus status_ = SimulationStatus::READY;
    bool playing_ = false;
    double elapsed_ = 0, speed_ = 8;
    double arrivalTime_ = 0;
    std::size_t revealed_ = 0;
    void advance();
    void syncSearch();
};
