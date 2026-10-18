#pragma once
#include "astar.h"
#include "rover.h"
#include <cstdint>

enum class SimulationStatus { EDIT_MAP, READY, SEARCHING, PATH_REVEAL, PATH_READY, DRIVING, ROUTE_BLOCKED, RECALCULATING, NO_ROUTE, ARRIVAL, MISSION_COMPLETE, NO_PATH, INVALID_ENDPOINTS };

// Playback and reveal state, independent of raylib and A* implementation details.
class Simulation {
public:
    Simulation(const Grid& grid, Cell start, Cell goal) : grid_(grid),start_(start),goal_(goal),search_(grid, start, goal) { syncSearch(); }
    std::optional<Cell> dropRock(std::uint32_t random);
    bool canDropRock() const;
    const std::vector<Cell>& oldPath() const { return oldPath_; }
    const std::vector<Cell>& droppedRocks() const { return rocks_; }
    std::size_t replans() const { return replans_; }
    std::size_t oldLength() const { return oldLength_; }
    bool replanning() const { return replanning_; }
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
    Grid grid_;
    Cell start_,goal_;
    AStarSearch search_;
    Rover rover_;
    SimulationStatus status_ = SimulationStatus::READY;
    bool playing_ = false;
    double elapsed_ = 0, speed_ = 8;
    double arrivalTime_ = 0;
    std::size_t revealed_ = 0;
    std::vector<Cell> oldPath_,rocks_;
    std::size_t replans_=0,oldLength_=0;
    bool replanning_=false;
    double blockedTime_=0;
    void updateBlocked(double seconds);
    std::vector<Cell> dropCandidates() const;
    void advance();
    void syncSearch();
};
