#include "scene_renderer.h"
#include "rover_renderer.h"
#include "visual_config.h"
#include <algorithm>
#include <cmath>
namespace {
constexpr float pi=3.14159265358979323846f;
Color fogged(Color c,float amount) {
    const Color fog=StormConfig::hazeColor;
    return {static_cast<unsigned char>(c.r+(fog.r-c.r)*amount),
            static_cast<unsigned char>(c.g+(fog.g-c.g)*amount),
            static_cast<unsigned char>(c.b+(fog.b-c.b)*amount),c.a};
}
Vector2 project(float x,float y) { return WorldCamera::project(x,y); }
void triangle(Vector2 a,Vector2 b,Vector2 c,Color color) {
    if((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x)>0) std::swap(b,c);
    DrawTriangle(a,b,c,color);
}
void quad(Vector2 a,Vector2 b,Vector2 c,Vector2 d,Color color) {
    triangle(a,b,c,color);triangle(a,c,d,color);
}
void diamond(Cell c,float inset,Color color,bool outline=false,float thickness=1) {
    Vector2 p[]{project(c.x+inset,c.y+inset),project(c.x+1-inset,c.y+inset),
                project(c.x+1-inset,c.y+1-inset),project(c.x+inset,c.y+1-inset)};
    if(outline) for(int i=0;i<4;++i) DrawLineEx(p[i],p[(i+1)%4],thickness,color);
    else quad(p[0],p[1],p[2],p[3],color);
}
void ellipseRing(Vector2 center,float radius,Color color,float thickness=1) {
    Vector2 last{center.x+radius,center.y};
    for(int i=1;i<=48;++i) {
        float angle=i*2*pi/48;
        Vector2 next{center.x+std::cos(angle)*radius,center.y+std::sin(angle)*radius*0.5f};
        DrawLineEx(last,next,thickness,color);last=next;
    }
}
Color groundColor(Cell cell,std::uint32_t hash,std::uint32_t seed) {
    int cx=cell.x/6,cy=cell.y/6;
    float fx=(cell.x%6)/6.0f,fy=(cell.y%6)/6.0f;
    fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy);
    auto noise=[&](int x,int y){return (SceneRenderer::tileHash(x,y,seed)%1000)/1000.0f;};
    float a=noise(cx,cy)*(1-fx)+noise(cx+1,cy)*fx;
    float b=noise(cx,cy+1)*(1-fx)+noise(cx+1,cy+1)*fx;
    int value=static_cast<int>(((a*(1-fy)+b*fy)-0.5f)*28)+static_cast<int>(hash%5)-2;
    return {static_cast<unsigned char>(158+value),static_cast<unsigned char>(77+value/2),
            static_cast<unsigned char>(49+value/3),255};
}
void ground(Cell c,std::uint32_t h,bool detail,std::uint32_t seed,float haze) {
    diamond(c,0,fogged(groundColor(c,h,seed),haze));
    // Fine sand seams rather than a high-contrast chessboard.
    diamond(c,0,Fade(Color{241,165,101,255},VisualConfig::gridAlpha),true,0.45f);
    if(!detail || haze>0.4f) return;
    for(int i=0;i<VisualConfig::terrainDetailCount;++i) {
        h=h*1664525u+1013904223u;
        float x=c.x+0.1f+(h%800)/1000.0f;
        h=h*1664525u+1013904223u;
        float y=c.y+0.1f+(h%800)/1000.0f;
        Vector2 p=project(x,y);
        if(i==0) DrawEllipse(static_cast<int>(p.x),static_cast<int>(p.y),7,3,Color{97,48,38,28});
        else if(i==1 && h%3==0) {
            DrawLineEx(p,{p.x+5,p.y+2},0.65f,{65,35,31,90});
            DrawLineEx({p.x+5,p.y+2},{p.x+8,p.y},0.65f,{65,35,31,90});
        } else {
            DrawEllipse(static_cast<int>(p.x),static_cast<int>(p.y),1.4f,0.7f,Color{93,53,43,150});
            DrawPixelV({p.x,p.y-1},Color{221,141,91,150});
        }
    }
}
void rockShape(Vector2 p,float width,float height,std::uint32_t h,float haze=0) {
    DrawEllipse(static_cast<int>(p.x+8),static_cast<int>(p.y+6),width*1.15f,width*0.48f,Fade(BLACK,VisualConfig::shadowAlpha*(1-haze)));
    Vector2 base[]{ {p.x-width,p.y}, {p.x-width*0.3f,p.y+width*0.45f},
                    {p.x+width,p.y+width*0.15f}, {p.x+width*0.6f,p.y-width*0.32f},
                    {p.x-width*0.35f,p.y-width*0.4f} };
    Vector2 top[5];
    for(int i=0;i<5;++i) top[i]={p.x+(base[i].x-p.x)*0.76f,p.y+(base[i].y-p.y)*0.76f-height};
    Color sides[]{Color{69,40,34,255},Color{85,47,37,255},Color{100,56,42,255},Color{93,52,41,255},Color{102,58,43,255}};
    for(int i=0;i<5;++i) quad(base[i],base[(i+1)%5],top[(i+1)%5],top[i],fogged(sides[i],haze));
    Color cap{static_cast<unsigned char>(140+h%25),static_cast<unsigned char>(82+h%14),57,255};
    for(int i=1;i<4;++i) triangle(top[0],top[i],top[i+1],fogged(cap,haze));
    DrawLineEx(top[4],top[0],0.8f,{211,139,91,180});
    DrawLineEx(top[0],top[1],0.8f,{189,113,73,180});
    DrawLineEx(top[1],{top[1].x+4,top[1].y+5},0.6f,{61,36,32,180});
}
void obstacle(Cell c,std::uint32_t hash,bool detailed,float haze) {
    Vector2 center=project(c.x+0.5f,c.y+0.5f);
    switch(hash%4) {
    case 0: rockShape(center,20,VisualConfig::rockHeight+(hash%11),hash,haze);break;
    case 1:
        rockShape({center.x-10,center.y-4},12,14,hash,haze);
        rockShape({center.x+9,center.y},13,18,hash+3,haze);
        if(detailed) rockShape({center.x-5,center.y+7},9,9,hash+7,haze);
        break;
    case 2: rockShape(center,25,VisualConfig::rockHeight*1.4f,hash,haze);break;
    default:
        DrawEllipse(static_cast<int>(center.x+5),static_cast<int>(center.y+5),26,12,Fade(BLACK,0.2f));
        DrawEllipse(static_cast<int>(center.x),static_cast<int>(center.y),24,12,fogged({100,51,38,255},haze));
        DrawEllipse(static_cast<int>(center.x),static_cast<int>(center.y-3),22,10,fogged({193,109,65,255},haze));
        DrawEllipse(static_cast<int>(center.x),static_cast<int>(center.y-2),17,7,fogged({58,34,31,255},haze));
        DrawEllipse(static_cast<int>(center.x+2),static_cast<int>(center.y),13,5,fogged({85,43,34,255},haze));
        if(detailed) rockShape({center.x+19,center.y+4},7,7,hash,haze);
        break;
    }
}
void beacon(Cell goal,float time,bool complete) {
    Vector2 p=project(goal.x+0.5f,goal.y+0.5f);
    float pulse=(std::sin(time*3)+1)*0.5f;
    DrawEllipse(static_cast<int>(p.x+8),static_cast<int>(p.y+5),18,6,{15,18,22,65});
    for(int i=0;i<3;++i) {
        float t=std::fmod(time*VisualConfig::beaconSignalSpeed+i/3.0f,1.0f);
        ellipseRing(p,9+t*30,Fade(VisualConfig::gold,(1-t)*(complete?0.75f:0.35f)),1.2f);
    }
    rockShape(p,8,6,5);
    DrawLineEx({p.x,p.y-6},{p.x,p.y-35},2,{193,202,201,255});
    DrawCircleGradient({p.x,p.y-34},complete?16.0f:10.0f,Fade(ORANGE,0.6f),Fade(ORANGE,0));
    DrawCircleV({p.x,p.y-34},2.5f+pulse,VisualConfig::gold);
    float h=50+std::sin(time*2)*2;
    Vector2 top{p.x,p.y-h-7},right{p.x+6,p.y-h},bottom{p.x,p.y-h+7},left{p.x-6,p.y-h};
    quad(top,right,bottom,left,Fade(VisualConfig::gold,0.16f+pulse*0.2f));
    DrawLineEx(top,right,1.1f,VisualConfig::gold);DrawLineEx(right,bottom,1.1f,VisualConfig::gold);
    DrawLineEx(bottom,left,1.1f,VisualConfig::gold);DrawLineEx(left,top,1.1f,VisualConfig::gold);
}
void background(Rectangle v,const Camera2D& camera,const VisualEffects& effects) {
    DrawRectangleGradientV(static_cast<int>(v.x),static_cast<int>(v.y),static_cast<int>(v.width),static_cast<int>(v.height),
                           fogged(VisualConfig::skyTop,effects.sandstorm().backgroundHaze(0)*0.75f),
                           fogged(VisualConfig::skyBottom,effects.sandstorm().backgroundHaze(0)*0.9f));
    DrawCircleGradient({v.x+v.width*0.8f,v.y+100},100,{219,139,96,25},{219,139,96,0});
    for(int layer=0;layer<2;++layer) {
        float rate=layer?VisualConfig::nearParallax:VisualConfig::farParallax;
        float shift=std::fmod(camera.target.x*rate,220.0f);
        float base=v.y+v.height*(layer?0.67f:0.48f)-camera.target.y*rate*0.3f;
        Color color=fogged(layer?Color{62,37,40,255}:Color{43,33,42,255},effects.sandstorm().backgroundHaze(layer));
        for(int i=-2;i<static_cast<int>(v.width/130)+3;++i) {
            float x=v.x+i*150.0f-shift;
            float height=60+((i+20)*73+layer*17)%90;
            triangle({x-80,base},{x+70,base-height},{x+230,base},color);
        }
        DrawRectangle(static_cast<int>(v.x),static_cast<int>(base),static_cast<int>(v.width),static_cast<int>(v.y+v.height-base),color);
    }
    // Small distant research antenna, rendered behind all map geometry.
    float bx=v.x+v.width*0.82f-camera.target.x*0.025f,by=v.y+v.height*0.5f;
    DrawLineEx({bx,by},{bx,by-34},1,{128,92,80,150});
    DrawLineEx({bx-9,by-26},{bx+9,by-34},1,{151,113,94,150});
    DrawCircleV({bx,by-35},1.5f,{235,149,92,180});
}
}
std::uint32_t SceneRenderer::tileHash(int x,int y,std::uint32_t seed) {
    std::uint32_t h=seed ^ (static_cast<std::uint32_t>(x)*0x9e3779b9u) ^ (static_cast<std::uint32_t>(y)*0x85ebca6bu);
    h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;return h^(h>>16);
}
void SceneRenderer::draw(const Terrain& terrain,const Simulation& sim,const WorldCamera& camera,
                         const VisualEffects& effects,Rectangle v,std::optional<Cell> hovered,bool showCosts) const {
    const auto& grid=terrain.grid;const auto& search=sim.search();
    const auto& view=camera.view();const bool editing=sim.status()==SimulationStatus::EDIT_MAP;
    const auto& storm=effects.sandstorm();
    Vector2 focus=sim.rover().available()?project(static_cast<float>(sim.rover().x()+0.5),static_cast<float>(sim.rover().y()+0.5)):
                                       project(terrain.start.x+0.5f,terrain.start.y+0.5f);
    BeginScissorMode(static_cast<int>(v.x),static_cast<int>(v.y),static_cast<int>(v.width),static_cast<int>(v.height));
    background(v,view,effects);
    Vector2 lo=GetScreenToWorld2D({v.x,v.y},view),hi=GetScreenToWorld2D({v.x+v.width,v.y+v.height},view);
    Rectangle visible{lo.x-90,lo.y-90,hi.x-lo.x+180,hi.y-lo.y+180};
    auto onScreen=[&](Vector2 p){return CheckCollisionPointRec(p,visible);};
    const auto count=static_cast<std::size_t>(grid.width())*grid.height();
    states_.assign(count,CellState::UNVISITED);
    if(showCosts) nodes_.assign(count,nullptr);
    auto index=[&](Cell c){return static_cast<std::size_t>(c.y)*grid.width()+c.x;};
    if(!editing && sim.status()!=SimulationStatus::ROUTE_BLOCKED) {
        for(const auto& n:search.open()) {
            states_[index(n.cell)]=CellState::OPEN;
            if(showCosts) nodes_[index(n.cell)]=&n;
        }
        for(const auto& n:search.closed()) {
            states_[index(n.cell)]=CellState::CLOSED;
            if(showCosts) nodes_[index(n.cell)]=&n;
        }
    }
    BeginMode2D(view);
    const bool detail=view.zoom>VisualConfig::detailZoomThreshold;
    for(int y=0;y<grid.height();++y) for(int x=0;x<grid.width();++x) {
        Cell c{x,y};Vector2 p=project(x+0.5f,y+0.5f);if(!onScreen(p)) continue;
        float haze=storm.haze(p,focus,view.zoom);
        ground(c,tileHash(x,y,terrain.config.seed),detail,terrain.config.seed,haze);
        if(states_[index(c)]==CellState::OPEN) {
            diamond(c,0,Fade(SKYBLUE,VisualConfig::openAlpha));
            diamond(c,0.12f,Fade(VisualConfig::cyan,0.38f),true,0.9f);
        }
        if(states_[index(c)]==CellState::CLOSED) diamond(c,0,Fade(Color{130,80,170,255},VisualConfig::closedAlpha));
        if(editing && hovered && *hovered==c) diamond(c,0.03f,Fade(VisualConfig::cyan,0.8f),true,1.5f);
    }
    for(const auto& mark:effects.tracks()) {
        if(!onScreen(project(mark.to.x,mark.to.y))) continue;
        float sx=-std::sin(mark.heading)*VisualConfig::trackHalfWidth,sy=std::cos(mark.heading)*VisualConfig::trackHalfWidth;
        for(float side:{-1.0f,1.0f}) {
            auto a=project(mark.from.x+sx*side,mark.from.y+sy*side),b=project(mark.to.x+sx*side,mark.to.y+sy*side);
            DrawLineEx(a,b,1.5f,{54,29,26,110});
        }
    }
    auto pathLine=[&](const std::vector<Cell>& path,std::size_t segments,bool active) {
        for(std::size_t i=0;i<path.size() && i<=segments;++i) {
            auto p=project(path[i].x+0.5f,path[i].y+0.5f);
            if(!onScreen(p)) continue;
            Color color=fogged(active?VisualConfig::gold:Color{121,101,77,110},storm.haze(p,focus,view.zoom)*(active?0.45f:1));
            diamond(path[i],0.20f,Fade(color,active?0.10f:0.06f));
            if(i) {
                auto a=project(path[i-1].x+0.5f,path[i-1].y+0.5f);
                if(active) DrawLineEx(a,p,8,Fade(color,0.10f));
                DrawLineEx(a,p,active?2.3f:1.4f,Fade(color,active?0.85f:0.45f));
                if(active) {
                    float t=std::fmod(effects.time()*0.8f+i*0.19f,1.0f);
                    Vector2 light{a.x+(p.x-a.x)*t,a.y+(p.y-a.y)*t};
                    DrawCircleV(light,1.8f,Fade(RAYWHITE,0.7f));
                }
            }
        }
    };
    if(!editing) {
        pathLine(sim.oldPath(),sim.oldPath().size(),false);
        if(sim.status()!=SimulationStatus::ROUTE_BLOCKED) pathLine(search.path(),sim.revealed(),true);
    }
    const auto& sonar=effects.sonar();
    if(sonar.strength()>0) {
        const float radius=SonarConfig::sweepRadius;
        for(float factor:{0.33f,0.66f,1.0f}) ellipseRing(focus,radius*factor,Fade(VisualConfig::cyan,SonarConfig::gridAlpha),0.65f);
        for(int i=0;i<18;++i) {
            float a=sonar.angle()-SonarConfig::trailRadians*(i+1)/18;
            float b=sonar.angle()-SonarConfig::trailRadians*i/18;
            Vector2 left{focus.x+std::cos(a)*radius,focus.y+std::sin(a)*radius*0.5f};
            Vector2 right{focus.x+std::cos(b)*radius,focus.y+std::sin(b)*radius*0.5f};
            triangle(focus,left,right,Fade(VisualConfig::cyan,SonarConfig::sweepAlpha*(1-i/18.0f)*0.12f));
            DrawLineEx(focus,right,0.65f,Fade(VisualConfig::cyan,SonarConfig::sweepAlpha*std::exp(-i*0.35f)));
        }
        Vector2 end{focus.x+std::cos(sonar.angle())*radius,focus.y+std::sin(sonar.angle())*radius*0.5f};
        DrawLineEx(focus,end,1.25f,Fade(Color{155,255,229,255},SonarConfig::sweepAlpha));
    }
    if(search.current() && (sim.status()==SimulationStatus::SEARCHING || sim.status()==SimulationStatus::RECALCULATING))
        diamond(search.current()->cell,0.05f,Fade(Color{220,255,255,255},0.65f+0.3f*std::sin(effects.time()*5)),true,2);
    if(sim.status()==SimulationStatus::PATH_REVEAL && !search.path().empty())
        diamond(search.path()[sim.revealed()],0.15f,Fade(VisualConfig::gold,0.65f+0.3f*std::sin(effects.time()*6)),true,2);
    diamond(terrain.start,0.2f,Fade(VisualConfig::cyan,0.5f),true,1);
    storm.drawGround(grid.width(),grid.height(),visible,focus,view.zoom);
    if(!parkedStart_ || !(*parkedStart_==terrain.start)) {
        parked_.reset({terrain.start});parkedStart_=terrain.start;
    }
    const Rover& rover=sim.rover().available()?sim.rover():parked_;
    const int roverDepth=static_cast<int>(std::floor(rover.x()+rover.y()));
    bool roverDrawn=false;
    // Painter order by x+y: rocks, rover and goal beacon share the same depth order.
    for(int depth=0;depth<grid.width()+grid.height();++depth) {
        for(int y=std::max(0,depth-grid.width()+1);y<=std::min(grid.height()-1,depth);++y) {
            int x=depth-y;Cell cell{x,y};auto p=project(x+0.5f,y+0.5f);
            if(!onScreen(p)) continue;
            if(grid.blocked(cell)) obstacle(cell,tileHash(x,y,terrain.config.seed),detail,storm.haze(p,focus,view.zoom));
            if(cell==terrain.goal) beacon(terrain.goal,effects.time(),sim.status()==SimulationStatus::MISSION_COMPLETE || sim.status()==SimulationStatus::ARRIVAL);
        }
        if(!roverDrawn && depth>=roverDepth) {
            if(onScreen(project(static_cast<float>(rover.x()+0.5),static_cast<float>(rover.y()+0.5))))
                drawRover(rover,{0,0},32,16,sonar.angle());
            roverDrawn=true;
        }
    }
    for(const auto& p:effects.dust()) {
        Vector2 pos{p.position.x,p.position.y-p.height};if(!onScreen(pos)) continue;
        float t=p.age/p.life;
        DrawCircleV(pos,p.size*(1+t),Fade(Color{212,143,98,255},0.22f*(1-t)));
    }
    if(storm.strength()>0) {
        // Light scattering is visual only; neither rover model nor beacon behavior is changed.
        Vector2 front=project(static_cast<float>(rover.x()+0.5+std::cos(rover.heading())*0.35),
                              static_cast<float>(rover.y()+0.5+std::sin(rover.heading())*0.35));
        DrawCircleGradient({front.x,front.y-8},13,Fade(Color{255,242,204,255},storm.strength()*StormConfig::roverLightOpacity),Fade(WHITE,0));
        Vector2 beaconPosition=project(terrain.goal.x+0.5f,terrain.goal.y+0.5f);
        if(onScreen(beaconPosition)) DrawCircleGradient({beaconPosition.x,beaconPosition.y-34},20,
            Fade(VisualConfig::gold,storm.strength()*StormConfig::beaconLightOpacity),Fade(VisualConfig::gold,0));
    }
    EndMode2D();
    // Constant-size screen-space motes, clipped to world, with bounded draw count.
    int motes=VisualConfig::ambientCount;
    for(int i=0;i<motes;++i) {
        auto h=tileHash(i,7,terrain.config.seed);
        float x=std::fmod((h%10000)/10000.0f*v.width+effects.time()*VisualConfig::windX,v.width);
        float y=std::fmod(((h>>12)%10000)/10000.0f*v.height+effects.time()*VisualConfig::windY,v.height);
        DrawLineEx({v.x+x,v.y+y},{v.x+x+2,v.y+y+1},1,Color{232,172,119,24});
    }
    storm.drawAir(v);
    if(showCosts && !editing) {
        // Screen-space labels remain legible; at fit zoom use the hover readout.
        if(view.zoom>=1.8f) for(const auto* node:nodes_) {
            if(!node || !onScreen(project(node->cell.x+0.5f,node->cell.y+0.5f))) continue;
            Vector2 p=GetWorldToScreen2D(project(node->cell.x+0.5f,node->cell.y+0.5f),view);
            int size=std::clamp(static_cast<int>(view.zoom*6),10,16);
            for(int line=0;line<3;++line) {
                const char* text=line==0?TextFormat("g %d",node->g):line==1?TextFormat("h %.1f",node->h):TextFormat("f %.1f",node->f);
                int x=static_cast<int>(p.x)-MeasureText(text,size)/2;
                int y=static_cast<int>(p.y)+(line-1)*size-size/2;
                DrawText(text,x+1,y+1,size,Color{7,13,23,240});
                DrawText(text,x,y,size,line==2?Color{255,236,158,255}:RAYWHITE);
            }
        }
        Rectangle readout{v.x+12,v.y+v.height-66,std::min(510.0f,v.width-24),29};
        DrawRectangleRounded(readout,0.15f,4,Color{10,24,35,235});
        const SearchNode* node=hovered?nodes_[index(*hovered)]:nullptr;
        const char* text=node?TextFormat("(%d,%d)  g %d  h %.2f  f %.2f",node->cell.x,node->cell.y,node->g,node->h,node->f):
                              "COSTS: hover OPEN/CLOSED; zoom in for tile labels";
        DrawText(text,static_cast<int>(readout.x+9),static_cast<int>(readout.y+7),16,RAYWHITE);
    }
    EndScissorMode();
}
