#include "world_camera.h"
#include <algorithm>
#include <cmath>
Vector2 WorldCamera::project(double x,double y) {
    return {static_cast<float>((x-y)*32),static_cast<float>((x+y)*16)};
}
std::optional<Cell> WorldCamera::pick(Vector2 screen,int width,int height) const {
    if (!CheckCollisionPointRec(screen,viewport_)) return std::nullopt;
    Vector2 world=GetScreenToWorld2D(screen,camera_);
    Cell cell{static_cast<int>(std::floor(world.x/64+world.y/32)),
              static_cast<int>(std::floor(world.y/32-world.x/64))};
    if (cell.x<0 || cell.y<0 || cell.x>=width || cell.y>=height) return std::nullopt;
    return cell;
}
WorldCamera::WorldCamera(int width,int height,Rectangle viewport) {
    bounds_={-height*32.0f,0,(width+height)*32.0f,(width+height)*16.0f};
    pathCenter_={bounds_.x+bounds_.width/2,bounds_.height/2};
    resize(viewport); reset();
}
void WorldCamera::resize(Rectangle viewport) {
    viewport_=viewport;
    camera_.offset={viewport.x+viewport.width/2,viewport.y+viewport.height/2};
    fit_=std::max(0.001f,std::min((viewport.width-60)/bounds_.width,(viewport.height-60)/bounds_.height));
    maxZoom_=std::max(fit_*4,4.0f);
    desiredZoom_=std::clamp(desiredZoom_,fit_,maxZoom_);
    camera_.zoom=std::clamp(camera_.zoom,fit_,maxZoom_);
}
void WorldCamera::reset() {
    desired_={bounds_.x+bounds_.width/2,bounds_.height/2};
    camera_.target=desired_; camera_.zoom=desiredZoom_=fit_; camera_.rotation=0;
}
void WorldCamera::setPath(const std::vector<Vector2>& path) {
    if (path.empty()) return;
    Vector2 lo=path.front(),hi=lo;
    for (auto p:path) {
        lo.x=std::min(lo.x,p.x); lo.y=std::min(lo.y,p.y);
        hi.x=std::max(hi.x,p.x); hi.y=std::max(hi.y,p.y);
    }
    pathCenter_={(lo.x+hi.x)/2,(lo.y+hi.y)/2};
}
void WorldCamera::clampTarget() {
    // Clamp in grid space, inside the diamond rather than its empty bounding-box corners.
    const float height=-bounds_.x/32, width=bounds_.width/32-height;
    float x=desired_.x/64+desired_.y/32;
    float y=desired_.y/32-desired_.x/64;
    desired_=project(std::clamp(x,0.0f,width),std::clamp(y,0.0f,height));
}
void WorldCamera::update(float dt,CameraPhase phase,Vector2 rover,double heading,bool autoRunning,const CameraInput& input) {
    if (!std::isfinite(dt) || dt<=0) return;
    dt=std::min(dt,0.1f);
    const bool manual=input.worldHovered && (input.wheel!=0 || input.drag.x!=0 || input.drag.y!=0 || input.keys.x!=0 || input.keys.y!=0);
    if (manual) {
        automatic_=false;
        desiredZoom_=std::clamp(desiredZoom_*std::exp(input.wheel*0.15f),fit_,maxZoom_);
        desired_.x+=(-input.drag.x+input.keys.x*500*dt)/camera_.zoom;
        desired_.y+=(-input.drag.y+input.keys.y*500*dt)/camera_.zoom;
    }
    if (automatic_ && autoRunning) {
        desiredZoom_=fit_;
        desired_={bounds_.x+bounds_.width/2,bounds_.height/2};
        if (phase==CameraPhase::REVEAL) { desired_=pathCenter_; desiredZoom_=fit_*1.12f; }
        if (phase==CameraPhase::FOLLOW || phase==CameraPhase::ARRIVAL || phase==CameraPhase::COMPLETE) {
            desired_=rover;
            desiredZoom_=std::max(fit_*2.5f,2.2f);
            if (phase==CameraPhase::FOLLOW) {
                Vector2 look=project(std::cos(heading)*0.65,std::sin(heading)*0.65);
                desired_.x+=look.x; desired_.y+=look.y;
            } else desiredZoom_*=1.3f;
        }
    } else if (automatic_ && !autoRunning) return; // Pause freezes automatic tracking.
    desiredZoom_=std::clamp(desiredZoom_,fit_,maxZoom_);
    clampTarget();
    const float alpha=1-std::exp(-4*dt);
    camera_.target.x+=(desired_.x-camera_.target.x)*alpha;
    camera_.target.y+=(desired_.y-camera_.target.y)*alpha;
    camera_.zoom+=(desiredZoom_-camera_.zoom)*alpha;
}
