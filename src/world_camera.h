#pragma once
#include "raylib.h"
#include <vector>
#include "astar.h"

enum class CameraPhase { WIDE, REVEAL, FOLLOW, BLOCKED, REPLAN, ARRIVAL, COMPLETE };
struct CameraInput {
    float wheel = 0;
    Vector2 drag{};
    Vector2 keys{};
    bool worldHovered = false;
};
class WorldCamera {
public:
    WorldCamera(int width, int height, Rectangle viewport);
    static Vector2 project(double x, double y);
    std::optional<Cell> pick(Vector2 screen, int width, int height) const;
    void resize(Rectangle viewport);
    void reset();
    void setAuto(bool enabled) { automatic_ = enabled; }
    bool automatic() const { return automatic_; }
    void setPath(const std::vector<Vector2>& path);
    void noticeRock(Vector2 rock) { rock_=rock; rockTime_=1.1f; }
    void update(float dt, CameraPhase phase, Vector2 rover, double heading,
                bool autoRunning, const CameraInput& input = {});
    const Camera2D& view() const { return camera_; }
    float fitZoom() const { return fit_; }
    float maxZoom() const { return maxZoom_; }
private:
    Camera2D camera_{};
    Rectangle viewport_{}, bounds_{};
    Vector2 desired_{}, pathCenter_{};
    float fit_ = 1, maxZoom_ = 4, desiredZoom_ = 1;
    bool automatic_ = true;
    Vector2 rock_{};
    float rockTime_=0;
    void clampTarget();
};
