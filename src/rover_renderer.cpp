#include "rover_renderer.h"
#include <algorithm>
#include <cmath>

void drawRover(const Rover& rover, Vector2 origin, float halfW, float halfH) {
    if (!rover.available()) return;
    const double c=std::cos(rover.heading()), s=std::sin(rover.heading());
    auto point = [&](double forward, double side, double height) {
        const double x=rover.x()+0.5+c*forward-s*side;
        const double y=rover.y()+0.5+s*forward+c*side;
        return Vector2{origin.x+static_cast<float>(x-y)*halfW,
                       origin.y+static_cast<float>(x+y)*halfH-static_cast<float>(height+side*rover.tilt())*halfW};
    };
    const auto ground=point(0,0,0);
    DrawEllipse(static_cast<int>(ground.x),static_cast<int>(ground.y),halfW*0.55f,halfH*0.45f,Color{20,20,25,110});
    const float radius=std::max(2.5f,halfW*0.12f);
    for (double side : {-0.32,0.32}) for (double front : {-0.29,0.0,0.29}) {
        Vector2 wheel=point(front,side,0.08);
        DrawCircleV(wheel,radius,Color{25,29,36,255});
        DrawCircleLinesV(wheel,radius,GRAY);
        float dx=static_cast<float>(std::cos(rover.wheelAngle()))*radius*0.8f;
        float dy=static_cast<float>(std::sin(rover.wheelAngle()))*radius*0.8f;
        DrawLineEx({wheel.x-dx,wheel.y-dy},{wheel.x+dx,wheel.y+dy},1.5f,LIGHTGRAY);
    }
    auto quad = [&](double f0,double f1,double width,double height,Color color) {
        Vector2 a=point(f0,-width,height), b=point(f1,-width,height);
        Vector2 d=point(f0,width,height), e=point(f1,width,height);
        DrawTriangle(a,d,e,color); DrawTriangle(a,e,b,color);
        DrawLineV(a,b,WHITE); DrawLineV(b,e,WHITE); DrawLineV(e,d,WHITE); DrawLineV(d,a,WHITE);
    };
    // Raised chassis, deck and blue equipment panel.
    quad(-0.38,0.38,0.25,0.12,Color{85,97,112,255});
    quad(-0.38,0.38,0.25,0.25,Color{222,229,230,255});
    quad(-0.29,0.06,0.19,0.28,Color{40,89,150,255});
    // Bright nose and forward arrow make the heading explicit in every direction.
    DrawLineEx(point(0.34,-0.22,0.28),point(0.34,0.22,0.28),3,ORANGE);
    DrawLineEx(point(0.10,0,0.32),point(0.48,0,0.32),3,ORANGE);
    DrawLineEx(point(0.48,0,0.32),point(0.32,-0.10,0.32),2,ORANGE);
    DrawLineEx(point(0.48,0,0.32),point(0.32,0.10,0.32),2,ORANGE);
    DrawLineEx(point(-0.12,0,0.3),point(-0.12,0,0.65),2,LIGHTGRAY);
    DrawCircleV(point(-0.12,0,0.65),3,ORANGE);
}
