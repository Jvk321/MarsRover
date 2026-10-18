#include "rover_renderer.h"
#include "visual_config.h"
#include <algorithm>
#include <cmath>

void drawRover(const Rover& rover, Vector2 origin, float halfW, float halfH, float scanAngle) {
    if (!rover.available()) return;
    const double c=std::cos(rover.heading()), s=std::sin(rover.heading());
    auto point = [&](double forward, double side, double height) {
        const double x=rover.x()+0.5+c*forward-s*side;
        const double y=rover.y()+0.5+s*forward+c*side;
        return Vector2{origin.x+static_cast<float>(x-y)*halfW,
                       origin.y+static_cast<float>(x+y)*halfH-static_cast<float>(height+side*rover.tilt())*halfW};
    };
    const auto ground=point(0,0,0);
    DrawEllipse(static_cast<int>(ground.x+6),static_cast<int>(ground.y+4),halfW*0.68f,halfH*0.64f,Color{12,16,23,24});
    DrawEllipse(static_cast<int>(ground.x),static_cast<int>(ground.y),halfW*0.55f,halfH*0.45f,Color{20,20,25,110});
    const float radius=std::max(2.5f,halfW*0.12f);
    for (double side : {-0.32,0.32}) for (double front : {-0.29,0.0,0.29}) {
        Vector2 wheel=point(front,side,0.08);
        DrawCircleV(wheel,radius,Color{25,29,36,255});
        DrawCircleLinesV(wheel,radius,GRAY);
        DrawCircleV(wheel,radius*0.48f,Color{99,109,114,255});
        float dx=static_cast<float>(std::cos(rover.wheelAngle()))*radius*0.8f;
        float dy=static_cast<float>(std::sin(rover.wheelAngle()))*radius*0.8f;
        DrawLineEx({wheel.x-dx,wheel.y-dy},{wheel.x+dx,wheel.y+dy},1.5f,LIGHTGRAY);
    }
    auto quad = [&](double f0,double f1,double width,double height,Color color) {
        Vector2 a=point(f0,-width,height), b=point(f1,-width,height);
        Vector2 d=point(f0,width,height), e=point(f1,width,height);
        DrawTriangle(a,d,e,color); DrawTriangle(a,e,b,color);
        DrawLineV(a,b,Color{162,176,184,255}); DrawLineV(b,e,Color{162,176,184,255});
        DrawLineV(e,d,Color{162,176,184,255}); DrawLineV(d,a,Color{162,176,184,255});
    };
    // Raised chassis, deck and blue equipment panel.
    quad(-0.38,0.38,0.25,0.12,Color{85,97,112,255});
    quad(-0.38,0.38,0.25,0.25,Color{222,229,230,255});
    quad(-0.29,0.06,0.19,0.28,Color{40,89,150,255});
    for(int i=0;i<4;++i) DrawLineEx(point(-0.27+i*0.08,-0.18,0.29),point(-0.27+i*0.08,0.18,0.29),0.65f,Color{102,153,183,220});
    for(double side:{-0.3,0.3}) DrawLineEx(point(-0.29,side,0.13),point(0.29,side,0.13),1.5f,Color{142,151,152,255});
    // Bright nose and forward arrow make the heading explicit in every direction.
    DrawLineEx(point(0.34,-0.22,0.28),point(0.34,0.22,0.28),3,ORANGE);
    for(double side:{-0.16,0.16}) {
        auto light=point(0.39,side,0.23);
        DrawCircleV(light,5,Color{255,223,163,25});
        DrawCircleV(light,1.8f,Color{255,247,214,255});
    }
    DrawLineEx(point(0.12,0,0.3),point(0.12,0,0.85),2.5f,LIGHTGRAY);
    auto camera=point(0.12,0,0.85);
    DrawRectangleRounded({camera.x-5,camera.y-3,10,6},0.2f,3,Color{208,215,211,255});
    DrawCircleV(point(0.19,0,0.85),2.2f,Color{39,73,94,255});
    DrawCircleV(point(0.20,0,0.85),0.8f,SKYBLUE);
    DrawLineEx(point(-0.28,0.12,0.3),point(-0.28,0.12,0.7),1.3f,LIGHTGRAY);
    auto antenna=point(-0.28,0.12,0.7);
    float rotation=scanAngle;
    Vector2 end{antenna.x+std::cos(rotation)*5,antenna.y+std::sin(rotation)*2.5f};
    DrawLineEx({2*antenna.x-end.x,2*antenna.y-end.y},end,1.8f,Color{217,221,210,255});
    DrawCircleV(antenna,1.5f,ORANGE);
}
