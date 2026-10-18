#include "simulation.h"
#include <algorithm>
#include <cmath>

bool Simulation::active() const {
    return status_ == SimulationStatus::READY || status_ == SimulationStatus::SEARCHING ||
           status_ == SimulationStatus::PATH_REVEAL || status_ == SimulationStatus::PATH_READY ||
           status_ == SimulationStatus::DRIVING || status_ == SimulationStatus::ARRIVAL;
}
void Simulation::syncSearch() {
    switch (search_.status()) {
    case SearchStatus::READY: status_ = SimulationStatus::READY; break;
    case SearchStatus::SEARCHING: status_ = SimulationStatus::SEARCHING; break;
    case SearchStatus::PATH_FOUND:
        status_ = pathLength() ? SimulationStatus::PATH_REVEAL : SimulationStatus::PATH_READY;
        break;
    case SearchStatus::NO_PATH: status_ = SimulationStatus::NO_PATH; break;
    case SearchStatus::INVALID_ENDPOINTS: status_ = SimulationStatus::INVALID_ENDPOINTS; break;
    }
}
void Simulation::reset() {
    search_.reset(); rover_.reset(); revealed_ = 0; playing_ = false; elapsed_ = 0; arrivalTime_ = 0; syncSearch();
}
void Simulation::play() { if (active()) { playing_ = true; elapsed_ = 0; } }
void Simulation::pause() { playing_ = false; elapsed_ = 0; }
void Simulation::setSpeed(double speed) {
    if (std::isfinite(speed)) speed_ = std::clamp(speed, 1.0, 60.0);
}
void Simulation::advance() {
    if (!active()) return;
    if (status_ == SimulationStatus::PATH_READY) {
        rover_.reset(search_.path());
        status_ = rover_.complete() ? SimulationStatus::ARRIVAL : SimulationStatus::DRIVING;
        if (!active()) pause();
        return;
    }
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
bool Simulation::visible(Cell cell) const {
    if (status_ != SimulationStatus::PATH_REVEAL && status_ != SimulationStatus::PATH_READY &&
        status_ != SimulationStatus::DRIVING && status_ != SimulationStatus::ARRIVAL && status_ != SimulationStatus::MISSION_COMPLETE) return false;
    const auto& path = search_.path();
    return std::find(path.begin(), path.begin() + revealed_ + 1, cell) != path.begin() + revealed_ + 1;
}
