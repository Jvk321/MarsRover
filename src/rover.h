#pragma once
#include "astar.h"

// Continuous pose in grid coordinates, independent of rendering.
class Rover {
public:
    void reset(const std::vector<Cell>& path = {});
    void update(double seconds);
    void setSpeed(double speed);
    void brake();
    void updateBrake(double seconds);
    bool braking() const { return braking_; }
    Cell stoppedCell() const;
    void followPath(const std::vector<Cell>& path);
    double speed() const { return speed_; }
    double x() const { return x_; }
    double y() const { return y_; }
    double heading() const { return heading_; } // radians: +x=0, +y=pi/2
    double wheelAngle() const { return wheelAngle_; }
    double tilt() const { return tilt_; }
    double travelled() const { return segment_ + progress_; }
    std::size_t segment() const { return segment_; } // completed segments
    bool complete() const { return !path_.empty() && segment_ + 1 >= path_.size(); }
    bool available() const { return !path_.empty(); }
    bool turning() const { return turning_; }
private:
    std::vector<Cell> path_;
    std::size_t segment_ = 0;
    double x_ = 0, y_ = 0, heading_ = 0, speed_ = 1.5;
    double progress_ = 0, wheelAngle_ = 0, tilt_ = 0;
    double turnStart_ = 0, turnDelta_ = 0, turnTime_ = 0;
    bool turning_ = false;
    bool braking_ = false;
    double brakeTime_ = 0, brakeDuration_ = 0, brakeProgress_ = 0;
    void prepareSegment();
};
