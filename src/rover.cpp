#include "rover.h"
#include <algorithm>
#include <cmath>
namespace { constexpr double pi = 3.14159265358979323846; constexpr double turnDuration = 0.45; }
void Rover::setSpeed(double speed) {
    if (std::isfinite(speed)) speed_ = std::clamp(speed, 0.25, 5.0);
}
void Rover::reset(const std::vector<Cell>& path) {
    path_ = path; segment_ = 0; progress_ = 0; wheelAngle_ = 0; tilt_ = 0;
    heading_ = 0; turnTime_ = 0; turning_ = false;
    braking_=false; brakeTime_=0;
    x_ = path.empty() ? 0 : path.front().x;
    y_ = path.empty() ? 0 : path.front().y;
    if (path.size() > 1) {
        heading_ = std::atan2(path[1].y-path[0].y, path[1].x-path[0].x);
        prepareSegment();
    }
}
Cell Rover::stoppedCell() const { return {static_cast<int>(std::lround(x_)),static_cast<int>(std::lround(y_))}; }
void Rover::followPath(const std::vector<Cell>& path) {
    // New route starts at the center reached by braking. Preserve pose and wheel phase.
    path_=path; segment_=0; progress_=0; turning_=false; braking_=false; tilt_=0;
    if (path_.size()>1) prepareSegment();
}
void Rover::brake() {
    brakeTime_=0; brakeProgress_=progress_;
    braking_=available() && !complete() && !turning_;
    brakeDuration_=braking_ ? 2*(1-progress_)/speed_ : 0;
    // During a turn the vehicle is already stationary.
    if (!braking_) { turning_=false; tilt_=0; }
}
void Rover::updateBrake(double seconds) {
    if (!braking_ || !std::isfinite(seconds) || seconds<=0) return;
    const double previous=progress_;
    brakeTime_=std::min(brakeDuration_,brakeTime_+seconds);
    const double u=brakeTime_/brakeDuration_;
    progress_=brakeProgress_+(1-brakeProgress_)*(2*u-u*u);
    const auto a=path_[segment_],b=path_[segment_+1];
    x_=a.x+(b.x-a.x)*progress_; y_=a.y+(b.y-a.y)*progress_;
    wheelAngle_+=(progress_-previous)/0.12;
    if (u>=1) { ++segment_; progress_=0; braking_=false; turning_=false; tilt_=0; }
}
void Rover::prepareSegment() {
    if (complete()) return;
    const auto a=path_[segment_], b=path_[segment_+1];
    const double target=std::atan2(b.y-a.y,b.x-a.x);
    turnStart_=heading_;
    turnDelta_=std::remainder(target-heading_,2*pi);
    turnTime_=0;
    turning_=std::abs(turnDelta_)>1e-8;
}
void Rover::update(double seconds) {
    if (!std::isfinite(seconds) || seconds<=0 || !available()) return;
    while (seconds>0 && !complete()) {
        if (turning_) {
            double dt=std::min(seconds,turnDuration-turnTime_);
            turnTime_+=dt; seconds-=dt;
            double t=std::min(1.0,turnTime_/turnDuration);
            heading_=turnStart_+turnDelta_*t*t*(3-2*t);
            tilt_=0.045*std::sin(pi*t)*(turnDelta_>0 ? 1 : -1);
            if (t>=1) { turning_=false; tilt_=0; }
        } else {
            const double dt=std::min(seconds,(1-progress_)/speed_);
            const double distance=dt*speed_;
            progress_+=distance; seconds-=dt;
            wheelAngle_+=distance/0.12;
            auto a=path_[segment_], b=path_[segment_+1];
            x_=a.x+(b.x-a.x)*progress_; y_=a.y+(b.y-a.y)*progress_;
            if (progress_>=1-1e-10) {
                x_=b.x; y_=b.y; ++segment_; progress_=0;
                prepareSegment();
            }
        }
    }
}
