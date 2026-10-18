#include "simulation.h"
#include <algorithm>
#include <cmath>

bool Simulation::active() const {
    return status_ == SimulationStatus::EDIT_MAP || status_ == SimulationStatus::READY || status_ == SimulationStatus::SEARCHING ||
           status_ == SimulationStatus::PATH_REVEAL || status_ == SimulationStatus::PATH_READY ||
           status_ == SimulationStatus::DRIVING || status_ == SimulationStatus::ARRIVAL ||
           status_ == SimulationStatus::ROUTE_BLOCKED || status_ == SimulationStatus::RECALCULATING;
}
void Simulation::syncSearch() {
    switch (search_.status()) {
    case SearchStatus::READY: status_ = replanning_ ? SimulationStatus::RECALCULATING : SimulationStatus::READY; break;
    case SearchStatus::SEARCHING: status_ = replanning_ ? SimulationStatus::RECALCULATING : SimulationStatus::SEARCHING; break;
    case SearchStatus::PATH_FOUND:
        status_ = pathLength() ? SimulationStatus::PATH_REVEAL : SimulationStatus::PATH_READY;
        break;
    case SearchStatus::NO_PATH: status_ = replanning_ ? SimulationStatus::NO_ROUTE : SimulationStatus::NO_PATH; break;
    case SearchStatus::INVALID_ENDPOINTS: status_ = SimulationStatus::INVALID_ENDPOINTS; break;
    }
}
void Simulation::reset() {
    search_=AStarSearch(grid_,start_,goal_); rover_.reset(); revealed_ = 0; playing_ = false; elapsed_ = 0; arrivalTime_ = 0;
    oldPath_.clear(); rocks_.clear(); replans_=0; oldLength_=0; replanning_=false; blockedTime_=0; syncSearch();
}
void Simulation::replaceMap(const Grid& grid,Cell start,Cell goal,bool editing) {
    grid_=grid; start_=start; goal_=goal;
    reset();
    if (editing) status_=SimulationStatus::EDIT_MAP;
}
void Simulation::enterEdit() { reset(); status_=SimulationStatus::EDIT_MAP; }
void Simulation::play() {
    if (status_==SimulationStatus::EDIT_MAP) syncSearch();
    if (active()) { playing_ = true; elapsed_ = 0; }
}
void Simulation::pause() { playing_ = false; elapsed_ = 0; }
void Simulation::setSpeed(double speed) {
    if (std::isfinite(speed)) speed_ = std::clamp(speed, 1.0, 60.0);
}
void Simulation::advance() {
    if (!active()) return;
    if (status_==SimulationStatus::EDIT_MAP) syncSearch();
    if (status_ == SimulationStatus::PATH_READY) {
        if (replanning_) rover_.followPath(search_.path());
        else rover_.reset(search_.path());
        status_ = rover_.complete() ? SimulationStatus::ARRIVAL : SimulationStatus::DRIVING;
        if (!active()) pause();
        return;
    }
    if (status_==SimulationStatus::ROUTE_BLOCKED) { updateBlocked(1.0/60); return; }
    if (status_ == SimulationStatus::DRIVING) {
        rover_.update(1.0/60);
        if (rover_.complete()) { status_ = SimulationStatus::ARRIVAL; arrivalTime_ = 0; }
        return;
    }
    if (status_ == SimulationStatus::ARRIVAL) {
        arrivalTime_ += 1.0/60;
        if (arrivalTime_ >= 2) { status_ = SimulationStatus::MISSION_COMPLETE; pause(); }
        return;
    }
    if (status_ == SimulationStatus::PATH_REVEAL) {
        ++revealed_;
        if (revealed_ == pathLength()) status_ = SimulationStatus::PATH_READY;
    } else {
        search_.step();
        syncSearch();
    }
    if (!active()) pause();
}
void Simulation::step() { pause(); advance(); }
void Simulation::update(double delta) {
    if (!playing_ || !active() || !std::isfinite(delta) || delta <= 0) return;
    if (status_==SimulationStatus::ROUTE_BLOCKED) { updateBlocked(delta); return; }
    if (status_ == SimulationStatus::PATH_READY) { advance(); return; }
    if (status_ == SimulationStatus::ARRIVAL) {
        arrivalTime_ += delta;
        if (arrivalTime_ >= 2) { status_ = SimulationStatus::MISSION_COMPLETE; pause(); }
        return;
    }
    if (status_ == SimulationStatus::DRIVING) {
        rover_.update(delta);
        if (rover_.complete()) { status_ = SimulationStatus::ARRIVAL; arrivalTime_ = 0; }
        return;
    }
    elapsed_ += delta;
    const double interval = 1.0 / speed_;
    while (playing_ && active() && elapsed_ >= interval) {
        elapsed_ -= interval;
        auto previous = status_;
        advance();
        if (status_ == SimulationStatus::PATH_READY) { elapsed_ = 0; break; }
        // Give the first reveal segment a full interval after reaching the goal.
        if (previous != SimulationStatus::PATH_REVEAL && status_ == SimulationStatus::PATH_REVEAL) {
            elapsed_ = 0;
            break;
        }
    }
}
std::vector<Cell> Simulation::dropCandidates() const {
    std::vector<Cell> result;
    if (status_!=SimulationStatus::DRIVING) return result;
    const auto& path=search_.path();
    // At least four segments ahead; the current and next tile remain safe for braking.
    for (std::size_t i=rover_.segment()+4;i+1<path.size();++i) {
        Cell c=path[i];
        if (!(c==start_) && !(c==goal_) && !grid_.blocked(c)) result.push_back(c);
    }
    return result;
}
bool Simulation::canDropRock() const { return !dropCandidates().empty(); }
std::optional<Cell> Simulation::dropRock(std::uint32_t random) {
    auto candidates=dropCandidates();
    if (candidates.empty()) return std::nullopt;
    Cell rock=candidates[random%candidates.size()];
    grid_.setBlocked(rock); rocks_.push_back(rock);
    oldPath_=search_.path(); oldLength_=pathLength(); revealed_=0;
    rover_.brake(); blockedTime_=0; elapsed_=0;
    status_=SimulationStatus::ROUTE_BLOCKED;
    return rock;
}
void Simulation::updateBlocked(double seconds) {
    if (rover_.braking()) { rover_.updateBrake(seconds); return; }
    blockedTime_+=seconds;
    if (blockedTime_<0.8) return;
    replanning_=true; ++replans_;
    search_=AStarSearch(grid_,rover_.stoppedCell(),goal_);
    revealed_=0; elapsed_=0; status_=SimulationStatus::RECALCULATING;
}
bool Simulation::visible(Cell cell) const {
    if (status_ != SimulationStatus::PATH_REVEAL && status_ != SimulationStatus::PATH_READY &&
        status_ != SimulationStatus::DRIVING && status_ != SimulationStatus::ARRIVAL && status_ != SimulationStatus::MISSION_COMPLETE) return false;
    const auto& path = search_.path();
    return std::find(path.begin(), path.begin() + revealed_ + 1, cell) != path.begin() + revealed_ + 1;
}
