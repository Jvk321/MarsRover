#include "rover.h"
#include <algorithm>
#include <cmath>
namespace { constexpr double pi = 3.14159265358979323846; constexpr double turnDuration = 0.45; }
void Rover::setSpeed(double speed) {
    if (std::isfinite(speed)) speed_ = std::clamp(speed, 0.25, 5.0);
}
void Rover::reset(const std::vector<Cell>& path) {
    path_ = path; segment_ = 0; progress_ = 0; wheelAngle_ = 0; wheelYaw_=0; tilt_ = 0; steering_=0;
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
    if (!braking_) { turning_=false; tilt_=0; steering_=0; }
}
void Rover::updateBrake(double seconds) {
    if (!braking_ || !std::isfinite(seconds) || seconds<=0) return;
    const double previous=progress_;
    brakeTime_=std::min(brakeDuration_,brakeTime_+seconds);
    const double u=brakeTime_/brakeDuration_;
    progress_=brakeProgress_+(1-brakeProgress_)*(2*u-u*u);
    const auto a=path_[segment_],b=path_[segment_+1];
    x_=a.x+(b.x-a.x)*progress_; y_=a.y+(b.y-a.y)*progress_;
    wheelAngle_+=(progress_-previous)/RoverGeometry::wheelRadius;
    if (u>=1) { ++segment_; progress_=0; braking_=false; turning_=false; tilt_=0; steering_=0; }
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
            const double body=std::clamp((t-0.18)/0.64,0.0,1.0);
            const double nextHeading=turnStart_+turnDelta_*body*body*(3-2*body);
            wheelYaw_+=nextHeading-heading_;heading_=nextHeading;
            steering_=RoverGeometry::steeringAngle*(turnDelta_>0?1:-1)*std::min({1.0,t/0.18,(1-t)/0.18});
            tilt_=0.045*std::sin(pi*t)*(turnDelta_>0 ? 1 : -1);
            if (t>=1) { turning_=false; tilt_=0; steering_=0; }
        } else {
            double dt=0,distance=0;
            bool corner=false;
            if(segment_+2<path_.size()) {
                const auto a=path_[segment_],b=path_[segment_+1],c=path_[segment_+2];
                corner=(b.x-a.x)!=(c.x-b.x) || (b.y-a.y)!=(c.y-b.y);
            }
            if(corner && progress_>=0.8-1e-10) {
                const double root=std::sqrt(std::max(0.0,1-progress_));
                dt=std::min(seconds,2*std::sqrt(0.2)*root/speed_);
                const double next=std::max(0.0,root-speed_*dt/(2*std::sqrt(0.2)));
                const double progress=1-next*next;
                distance=progress-progress_;progress_=progress;
            } else if(segment_+2==path_.size()) {
                // Final tile only: continuous deceleration to zero at GOAL.
                // Closed-form integration preserves motion across different frame rates.
                const double remaining=std::sqrt(std::max(0.0,1-progress_));
                dt=std::min(seconds,2*remaining/speed_);
                const double next=std::max(0.0,remaining-speed_*dt/2);
                const double progress=1-next*next;
                distance=progress-progress_;progress_=progress;
            } else {
                dt=std::min(seconds,((corner?0.8:1.0)-progress_)/speed_);
                distance=dt*speed_;progress_+=distance;
            }
            seconds-=dt;
            wheelAngle_+=distance/RoverGeometry::wheelRadius;
            auto a=path_[segment_], b=path_[segment_+1];
            x_=a.x+(b.x-a.x)*progress_; y_=a.y+(b.y-a.y)*progress_;
            if (progress_>=1-1e-10) {
                x_=b.x; y_=b.y; ++segment_; progress_=0;
                prepareSegment();
            }
        }
    }
}



