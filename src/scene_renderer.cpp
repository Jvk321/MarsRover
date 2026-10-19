#include "scene_renderer.h"
#include "ui_glass.h"
#include "rover_renderer.h"
#include "visual_config.h"
#include "terrain_art.h"
#include "world_art.h"
#include "art_primitives.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
namespace {
using namespace Art;
Color fogged(Color c,float amount) {return fog(c,amount);}
void ellipseRing(Vector2 center,float radius,Color color,float thickness=1) {ring(center,radius,color,thickness);}
}
std::uint32_t SceneRenderer::tileHash(int x,int y,std::uint32_t seed) {
    return Art::hash(x,y,seed);
}
void SceneRenderer::draw(const Terrain& terrain,const Simulation& sim,const WorldCamera& camera,
                         const VisualEffects& effects,Rectangle v,std::optional<Cell> hovered,bool showCosts,bool withMinimap) const {
    const auto& grid=terrain.grid;const auto& search=sim.search();
    const auto& view=camera.view();const bool editing=sim.status()==SimulationStatus::EDIT_MAP;
    const auto& storm=effects.sandstorm();
    if(!parkedStart_ || !(*parkedStart_==terrain.start)) {
        parked_.reset({terrain.start});parkedStart_=terrain.start;
    }
    const Rover& rover=sim.rover().available()?sim.rover():parked_;
    Vector2 focus=sim.rover().available()?project(static_cast<float>(sim.rover().x()+0.5),static_cast<float>(sim.rover().y()+0.5)):
                                       project(terrain.start.x+0.5f,terrain.start.y+0.5f);
    BeginScissorMode(static_cast<int>(v.x),static_cast<int>(v.y),static_cast<int>(v.width),static_cast<int>(v.height));
    WorldArt::background(v,view,effects);
    Vector2 lo=GetScreenToWorld2D({v.x,v.y},view),hi=GetScreenToWorld2D({v.x+v.width,v.y+v.height},view);
    Rectangle visible{lo.x-150,lo.y-150,hi.x-lo.x+300,hi.y-lo.y+300};
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
    const bool activeGrid=editing || showCosts || sim.status()==SimulationStatus::SEARCHING || sim.status()==SimulationStatus::RECALCULATING;
    WorldArt::Props props;
    const auto propCount=WorldArt::props(grid,terrain.config.seed,props);
    WorldArt::baseGround(grid,visible);
    TerrainArt::plateau(grid,terrain.config.seed,visible,detail);
    for(int y=0;y<grid.height();++y) for(int x=0;x<grid.width();++x) {
        Cell c{x,y};Vector2 p=project(x+0.5f,y+0.5f);if(!onScreen(p)) continue;
        float haze=storm.haze(p,focus,view.zoom);
        TerrainArt::ground(c,terrain.config.seed,detail,haze);
    }
    for(int y=0;y<grid.height();++y) for(int x=0;x<grid.width();++x) {
        auto p=project(x+0.5f,y+0.5f);
        if(grid.blocked({x,y}) && onScreen(p)) TerrainArt::shadow(grid,{x,y},terrain.config.seed,storm.haze(p,focus,view.zoom));
    }
    for(std::size_t i=0;i<propCount;++i) if(onScreen(project(props[i].x,props[i].y))) WorldArt::propShadow(props[i]);
    if(onScreen(focus)) Art::shadow(focus,22,38,0.20f);
    TerrainArt::gridLines(grid,visible,activeGrid,view.zoom);
    if(!editing && sim.status()!=SimulationStatus::ROUTE_BLOCKED) {
        for(const auto& n:search.closed()) if(onScreen(project(n.cell.x+0.5f,n.cell.y+0.5f)))
            diamond(n.cell,0,Fade(Color{95,113,201,255},VisualConfig::closedAlpha));
        for(const auto& n:search.open()) if(onScreen(project(n.cell.x+0.5f,n.cell.y+0.5f)))
            diamond(n.cell,0,Fade(VisualConfig::cyan,VisualConfig::openAlpha));
        // Fill and wire passes avoid a triangle/quad GPU mode change for every visited tile.
        for(const auto& n:search.open()) if(onScreen(project(n.cell.x+0.5f,n.cell.y+0.5f))) {
            float pulse=0.85f+0.15f*std::sin(effects.time()*2.8f+(n.cell.x+n.cell.y)*0.24f);
            diamond(n.cell,0.05f,Fade(VisualConfig::cyan,VisualConfig::openGlow*0.18f*pulse),true,4);
            diamond(n.cell,0.05f,Fade(VisualConfig::cyan,0.62f*pulse),true,0.8f);
        }
        for(const auto& n:search.open()) {
            auto p=project(n.cell.x+0.5f,n.cell.y+0.5f);
            if(onScreen(p)) DrawCircleV(p,1.1f,Fade(VisualConfig::cyan,0.55f));
        }
    }
    if(editing && hovered) diamond(*hovered,0.03f,Fade(VisualConfig::cyan,0.8f),true,1.5f);
    for(const auto& mark:effects.tracks()) {
        if(!onScreen(project(mark.to.x,mark.to.y))) continue;
        const float grain=0.93f+0.07f*std::sin(mark.to.x*193.7f+mark.to.y*71.3f);
        const float sx=-std::sin(mark.heading)*VisualConfig::trackWidth*0.5f*grain;
        const float sy=std::cos(mark.heading)*VisualConfig::trackWidth*0.5f*grain;
        auto a=project(mark.to.x-sx,mark.to.y-sy),b=project(mark.to.x+sx,mark.to.y+sy);
        DrawLineEx(a,b,3.1f,Fade(Color{106,52,34,255},VisualConfig::trackOpacity*0.23f));
        DrawLineEx(a,b,1.7f,Fade(Color{91,45,31,255},VisualConfig::trackOpacity));
        auto m=project(mark.to.x,mark.to.y);
        DrawLineEx(a,{m.x+0.7f,m.y-0.4f},0.7f,Fade(Color{73,39,29,255},VisualConfig::trackOpacity));
        DrawLineEx({a.x,a.y+0.8f},{b.x,b.y+0.8f},0.6f,Fade(Color{219,125,71,255},0.15f));
    }
    auto pathLine=[&](const std::vector<Cell>& path,std::size_t segments,bool active) {
        for(std::size_t i=0;i<path.size() && i<=segments;++i) {
            auto p=project(path[i].x+0.5f,path[i].y+0.5f);
            if(!onScreen(p)) continue;
            Color color=fogged(active?VisualConfig::gold:Color{121,101,77,110},storm.haze(p,focus,view.zoom)*(active?0.45f:1));
            diamond(path[i],0.20f,Fade(color,active?0.10f:0.06f));
            if(i) {
                auto a=project(path[i-1].x+0.5f,path[i-1].y+0.5f);
                if(active) {
                    DrawLineEx(a,p,12,Fade(Color{255,128,25,255},VisualConfig::pathGlow*0.30f));
                    DrawLineEx(a,p,6,Fade(color,VisualConfig::pathGlow));
                }
                DrawLineEx(a,p,active?2.2f:1.4f,Fade(color,active?0.95f:0.45f));
                if(active) {
                    DrawLineEx(a,p,0.85f,Fade(VisualConfig::pathCore,0.80f));
                    if(i%2==0) {
                        Vector2 mid=lerp(a,p,0.5f);float dx=p.x-a.x,dy=p.y-a.y,len=std::hypot(dx,dy);
                        if(len>0) {
                            Vector2 back{mid.x-dx/len*3,mid.y-dy/len*3};
                            DrawLineEx({back.x-dy/len*2,back.y+dx/len*2},mid,1.0f,VisualConfig::pathCore);
                            DrawLineEx({back.x+dy/len*2,back.y-dx/len*2},mid,1.0f,VisualConfig::pathCore);
                        }
                    }
                    float t=std::fmod(effects.time()*VisualConfig::pathFlowSpeed+i*0.19f,1.0f);
                    Vector2 light{a.x+(p.x-a.x)*t,a.y+(p.y-a.y)*t};
                    DrawCircleV(light,2.7f,Fade(color,0.18f));
                    DrawCircleV(light,1.4f,Fade(VisualConfig::pathCore,0.85f));
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
        auto antenna=Rover3D::pose(rover,sonar.angle(),effects.roverMotion()).antenna;antenna.y=0;
        const auto scanFocus=Rover3D::worldToMap(antenna);
        const float radius=SonarConfig::sweepRadius;
        auto beam=[&](float scanAngle) {
            const float yaw=Rover3D::antennaYaw(scanAngle);
            return Vector2{scanFocus.x+(std::cos(yaw)-std::sin(yaw))*radius/std::sqrt(2.0f),
                           scanFocus.y+(std::cos(yaw)+std::sin(yaw))*radius/(2*std::sqrt(2.0f))};
        };
        for(float factor:{0.33f,0.66f,1.0f}) ellipseRing(scanFocus,radius*factor,Fade(VisualConfig::cyan,SonarConfig::gridAlpha),0.65f);
        for(int i=0;i<18;++i) {
            float a=sonar.angle()-SonarConfig::trailRadians*(i+1)/18;
            float b=sonar.angle()-SonarConfig::trailRadians*i/18;
            Vector2 left=beam(a),right=beam(b);
            triangle(scanFocus,left,right,Fade(VisualConfig::cyan,SonarConfig::sweepAlpha*(1-i/18.0f)*0.12f));
            DrawLineEx(scanFocus,right,0.65f,Fade(VisualConfig::cyan,SonarConfig::sweepAlpha*std::exp(-i*0.35f)));
        }
        DrawLineEx(scanFocus,beam(sonar.angle()),1.25f,Fade(Color{155,255,229,255},SonarConfig::sweepAlpha));
    }
    if(search.current() && (sim.status()==SimulationStatus::SEARCHING || sim.status()==SimulationStatus::RECALCULATING)) {
        float pulse=0.75f+0.20f*std::sin(effects.time()*5);
        diamond(search.current()->cell,0.025f,Fade(VisualConfig::cyan,pulse*0.12f),true,7);
        diamond(search.current()->cell,0.025f,Fade(Color{216,255,255,255},pulse),true,1.6f);
    }
    if(sim.status()==SimulationStatus::PATH_REVEAL && !search.path().empty())
        diamond(search.path()[sim.revealed()],0.15f,Fade(VisualConfig::gold,0.65f+0.3f*std::sin(effects.time()*6)),true,2);
    WorldArt::startPad(terrain.start,effects.time());
    storm.drawGround(grid.width(),grid.height(),visible,focus,view.zoom);
    drawItems_.clear();drawItems_.reserve(count+props.size()+2);
    for(int y=0;y<grid.height();++y) for(int x=0;x<grid.width();++x) {
        auto p=project(x+0.5f,y+0.5f);
        if(grid.blocked({x,y}) && onScreen(p)) drawItems_.push_back({p.y,DrawKind::OBSTACLE,{x,y},0});
    }
    for(std::size_t i=0;i<propCount;++i) {
        auto p=project(props[i].x,props[i].y);
        if(onScreen(p)) drawItems_.push_back({p.y,DrawKind::PROP,{0,0},i});
    }
    auto goalPosition=project(terrain.goal.x+0.5f,terrain.goal.y+0.5f);
    if(onScreen(goalPosition)) drawItems_.push_back({goalPosition.y,DrawKind::BEACON,terrain.goal,0});
    if(onScreen(focus)) drawItems_.push_back({focus.y,DrawKind::ROVER,{0,0},0});
    // Continuous ground-footpoint Y; never floor the moving rover to a tile row.
    // Stable tie rules prevent props at equal depth from changing order when culling changes.
    std::sort(drawItems_.begin(),drawItems_.end(),[](const DrawItem& a,const DrawItem& b) {
        if(a.depth!=b.depth) return a.depth<b.depth;
        if(a.kind!=b.kind) return a.kind<b.kind;
        if(a.cell.x!=b.cell.x) return a.cell.x<b.cell.x;
        return a.propIndex<b.propIndex;
    });
    for(const auto& item:drawItems_) {
        switch(item.kind) {
        case DrawKind::OBSTACLE:
            TerrainArt::obstacle(grid,item.cell,terrain.config.seed,detail,
                                 storm.haze(project(item.cell.x+0.5f,item.cell.y+0.5f),focus,view.zoom));break;
        case DrawKind::PROP: {
            const auto& prop=props[item.propIndex];
            WorldArt::prop(prop,detail,storm.haze(project(prop.x,prop.y),focus,view.zoom),effects.time());break;
        }
        case DrawKind::BEACON:
            WorldArt::beacon(terrain.goal,effects.time(),sim.status()==SimulationStatus::MISSION_COMPLETE || sim.status()==SimulationStatus::ARRIVAL);break;
        case DrawKind::ROVER: roverArt_.draw(rover,{0,0},32,16,sonar.angle(),effects.roverMotion());break;
        }
    }
    for(const auto& p:effects.dust()) {
        Vector2 pos{p.position.x,p.position.y-p.height};if(!onScreen(pos)) continue;
        float t=p.age/p.life;
        DrawCircleV(pos,p.size*(1+t),Fade(Color{212,143,98,255},0.22f*(1-t)));
    }
    if(storm.strength()>0) {
        // Light scattering is visual only; neither rover model nor beacon behavior is changed.
        const auto body=Rover3D::pose(rover,sonar.angle(),effects.roverMotion()).chassis;
        const auto front=Rover3D::worldToMap(Vector3Transform({0.522f,0.515f,0},body));
        DrawCircleGradient(front,16,Fade(Color{255,242,204,255},storm.strength()*StormConfig::roverLightOpacity),Fade(WHITE,0));
        Vector2 beaconPosition=project(terrain.goal.x+0.5f,terrain.goal.y+0.5f);
        if(onScreen(beaconPosition)) DrawCircleGradient({beaconPosition.x,beaconPosition.y-52},22,
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
    if(withMinimap) minimap(terrain,sim,camera,v);
    EndScissorMode();
}
Rectangle SceneRenderer::minimapBounds(Rectangle v) {
    return {v.x+v.width-192,v.y+16,176,184};
}
void SceneRenderer::minimap(const Terrain& terrain,const Simulation& sim,const WorldCamera& camera,Rectangle v) const {
    drawMinimap(terrain,sim,camera,v,minimapBounds(v));
}
void SceneRenderer::drawMinimap(const Terrain& terrain,const Simulation& sim,const WorldCamera& camera,Rectangle v,Rectangle panel) const {
    const float s=panel.width/234.0f;
    Ui::panel(panel,22*s);
    Ui::text("TERRAIN / LIVE",{panel.x+18*s,panel.y+14*s},16*s,Ui::Theme::muted);
    const auto& grid=terrain.grid;
    const float contentHeight=panel.height-80*s;
    float scale=std::min((panel.width-36*s)/grid.width(),contentHeight/grid.height());
    Vector2 origin{panel.x+(panel.width-grid.width()*scale)/2,panel.y+43*s+(contentHeight-grid.height()*scale)/2};
    auto mapPoint=[&](float x,float y) {return Vector2{origin.x+x*scale,origin.y+y*scale};};
    DrawRectangleRounded({origin.x-3,origin.y-3,grid.width()*scale+6,grid.height()*scale+6},0.05f,4,{14,28,41,100});
    int stride=std::max(1,static_cast<int>(std::ceil(1.0f/scale)));
    for(int y=0;y<grid.height();y+=stride) for(int x=0;x<grid.width();x+=stride) {
        auto index=static_cast<std::size_t>(y)*grid.width()+x;
        auto state=index<states_.size()?states_[index]:CellState::UNVISITED;
        Color color=grid.blocked({x,y})?Color{163,112,92,210}:state==CellState::OPEN?Color{90,178,204,185}:
                    state==CellState::CLOSED?Color{95,105,151,165}:Color{0,0,0,0};
        if(color.a==0) continue;
        auto p=mapPoint(float(x),float(y));
        DrawRectangleRec({p.x,p.y,std::min(stride,grid.width()-x)*scale,std::min(stride,grid.height()-y)*scale},color);
    }
    auto route=[&](const std::vector<Cell>& path,std::size_t count,Color color) {
        for(std::size_t i=1;i<path.size() && i<=count;++i)
            DrawLineEx(mapPoint(path[i-1].x+0.5f,path[i-1].y+0.5f),mapPoint(path[i].x+0.5f,path[i].y+0.5f),1.3f*s,color);
    };
    if(sim.status()!=SimulationStatus::EDIT_MAP) {
        route(sim.oldPath(),sim.oldPath().size(),Color{123,115,103,120});
        if(sim.status()!=SimulationStatus::ROUTE_BLOCKED) route(sim.search().path(),sim.revealed(),Ui::Theme::gold);
    }
    auto marker=[&](float x,float y,Color color,float radius) {
        auto p=mapPoint(x+0.5f,y+0.5f);DrawCircleV(p,radius*s+1,{12,23,35,255});DrawCircleV(p,radius*s,color);
    };
    marker(float(terrain.start.x),float(terrain.start.y),VisualConfig::startGreen,2.6f);
    marker(float(terrain.goal.x),float(terrain.goal.y),Ui::Theme::orange,3.3f);
    if(sim.rover().available()) marker(float(sim.rover().x()),float(sim.rover().y()),Ui::Theme::text,3);
    if(sim.search().current() && (sim.status()==SimulationStatus::SEARCHING || sim.status()==SimulationStatus::RECALCULATING)) {
        auto c=sim.search().current()->cell;marker(float(c.x),float(c.y),Ui::Theme::cyan,2.6f);
    }
    float left=float(grid.width()),right=0,top=float(grid.height()),bottom=0;
    for(Vector2 screen:{Vector2{v.x,v.y},Vector2{v.x+v.width,v.y},Vector2{v.x+v.width,v.y+v.height},Vector2{v.x,v.y+v.height}}) {
        auto p=GetScreenToWorld2D(screen,camera.view());float x=p.x/64+p.y/32,y=p.y/32-p.x/64;
        left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);
    }
    left=std::clamp(left,0.0f,float(grid.width()));right=std::clamp(right,0.0f,float(grid.width()));
    top=std::clamp(top,0.0f,float(grid.height()));bottom=std::clamp(bottom,0.0f,float(grid.height()));
    DrawRectangleLinesEx({origin.x+left*scale,origin.y+top*scale,(right-left)*scale,(bottom-top)*scale},0.8f*s,Fade(Ui::Theme::text,0.32f));
    const char* labels[]{"ROVER","GOAL","PATH"};const Color colors[]{Ui::Theme::text,Ui::Theme::orange,Ui::Theme::gold};
    for(int i=0;i<3;++i) {
        float x=panel.x+18*s+i*68*s,y=panel.y+panel.height-25*s;
        DrawCircleV({x+3*s,y+6*s},2.5f*s,colors[i]);Ui::text(labels[i],{x+12*s,y},13*s,Ui::Theme::muted);
    }
}
