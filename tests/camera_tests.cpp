#include "world_camera.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
void checkImpl(bool condition,int line) {
    if (!condition) { std::cerr << "Camera test failed: " << line << '\n'; std::exit(1); }
}
#define check(...) checkImpl((__VA_ARGS__),__LINE__)
bool near(float a,float b) { return std::abs(a-b)<0.001f; }
int main() {
    for (Vector2 size : {Vector2{20,20},Vector2{48,35}}) {
        Rectangle viewport{300,80,980,850};
        WorldCamera camera(static_cast<int>(size.x),static_cast<int>(size.y),viewport);
        auto reset=camera.view();
        check(near(reset.zoom,camera.fitZoom()));
        // Every map corner fits the world viewport after RESET.
        for (Vector2 c : {Vector2{0,0},Vector2{size.x,0},Vector2{0,size.y},size}) {
            auto world=WorldCamera::project(c.x,c.y);
            float x=(world.x-reset.target.x)*reset.zoom+reset.offset.x;
            float y=(world.y-reset.target.y)*reset.zoom+reset.offset.y;
            check(x>=viewport.x && x<=viewport.x+viewport.width);
            check(y>=viewport.y && y<=viewport.y+viewport.height);
        }
        camera.update(0.016f,CameraPhase::WIDE,{},0,true);
        check(near(camera.view().zoom,reset.zoom));
        CameraInput wheel; wheel.worldHovered=true; wheel.wheel=4;
        camera.update(0.016f,CameraPhase::WIDE,{},0,true,wheel);
        check(!camera.automatic() && camera.view().zoom>reset.zoom);
        check(camera.view().zoom<reset.zoom*std::exp(0.6f)); // Smooth, not instantaneous.
        wheel.wheel=100;
        for (int i=0;i<200;++i) camera.update(0.05f,CameraPhase::WIDE,{},0,true,wheel);
        check(camera.view().zoom<=camera.maxZoom()+0.001f);
        CameraInput pan; pan.worldHovered=true; pan.drag={100000,100000}; pan.keys={-1,-1};
        for (int i=0;i<200;++i) camera.update(0.05f,CameraPhase::WIDE,{},0,true,pan);
        auto target=camera.view().target;
        float gx=target.x/64+target.y/32,gy=target.y/32-target.x/64;
        check(gx>=-0.001 && gx<=size.x+0.001 && gy>=-0.001 && gy<=size.y+0.001);
        wheel.wheel=-100;
        for (int i=0;i<200;++i) camera.update(0.05f,CameraPhase::WIDE,{},0,true,wheel);
        check(camera.view().zoom>=camera.fitZoom()-0.001f);
        camera.reset(); camera.setAuto(true);
        Vector2 rover=WorldCamera::project(4.5,5.5);
        camera.update(0.016f,CameraPhase::FOLLOW,rover,0,true);
        check(camera.view().zoom>camera.fitZoom());
        check(!near(camera.view().target.x,rover.x)); // Soft follow, not locked to center.
        auto frozen=camera.view();
        camera.update(0.1f,CameraPhase::FOLLOW,WorldCamera::project(10,10),1,false);
        check(camera.view().target.x==frozen.target.x && camera.view().zoom==frozen.zoom);
        for (int i=0;i<200;++i) camera.update(0.016f,CameraPhase::FOLLOW,rover,0,true);
        auto follow=camera.view();
        camera.update(0.016f,CameraPhase::FOLLOW,WorldCamera::project(4.51,5.5),0,true);
        check(std::abs(camera.view().target.x-follow.target.x)<1); // No tile snapping.
        for (int i=0;i<125;++i) camera.update(0.016f,CameraPhase::ARRIVAL,rover,0,true);
        check(camera.view().zoom>follow.zoom);
        const auto beforeRock=camera.view().target;
        const auto rock=WorldCamera::project(9.5,5.5);
        camera.noticeRock(rock);
        camera.update(0.016f,CameraPhase::BLOCKED,rover,0,true);
        check(camera.view().target.x>beforeRock.x);
        check(camera.view().target.x<rock.x); // Smooth interest shift, no camera teleport.
        for (int i=0;i<250;++i) camera.update(0.016f,CameraPhase::REPLAN,rover,0,true);
        check(std::abs(camera.view().target.x-rover.x)<0.1f);
        camera.setAuto(false);
        auto manual=camera.view();
        camera.update(0.1f,CameraPhase::FOLLOW,WorldCamera::project(30,20),0,true);
        check(std::abs(camera.view().target.x-manual.target.x)<1);
        camera.reset();
        check(near(camera.view().zoom,camera.fitZoom()) && near(camera.view().target.x,reset.target.x));
        camera.resize({300,80,700,600}); camera.reset();
        check(near(camera.view().offset.x,650) && near(camera.view().zoom,camera.fitZoom()));
        CameraInput outside; outside.wheel=10; outside.worldHovered=false;
        camera.update(0.016f,CameraPhase::WIDE,{},0,true,outside);
        check(near(camera.view().zoom,camera.fitZoom()));
    }
    std::cout << "Camera tests passed\n";
}
