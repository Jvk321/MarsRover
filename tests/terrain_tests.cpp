#include "terrain.h"
#include "simulation.h"
#include "world_camera.h"
#include <cstdlib>
#include <iostream>
#include <queue>
void checkImpl(bool condition,int line) {
    if (!condition) { std::cerr << "Terrain test failed: " << line << '\n'; std::exit(1); }
}
#define check(...) checkImpl((__VA_ARGS__),__LINE__)
int shortest(const Terrain& t) {
    std::vector<int> distance(t.grid.width()*t.grid.height(),-1);
    auto index=[&](Cell c){return c.y*t.grid.width()+c.x;};
    std::queue<Cell> q; q.push(t.start); distance[index(t.start)]=0;
    while (!q.empty()) {
        Cell c=q.front();q.pop();
        if(c==t.goal) return distance[index(c)];
        for(Cell d:{Cell{1,0},Cell{-1,0},Cell{0,1},Cell{0,-1}}) {
            Cell n{c.x+d.x,c.y+d.y};
            if(!t.grid.blocked(n) && distance[index(n)]<0) {
                distance[index(n)]=distance[index(c)]+1;q.push(n);
            }
        }
    }
    return -1;
}
int main() {
    for (Cell size:{Cell{20,20},Cell{30,30},Cell{50,50},Cell{80,60},Cell{23,17}}) {
        Terrain a({size.x,size.y,35,123456789}),b(a.config);
        check(connected(a.grid,a.start,a.goal));
        for(int y=0;y<size.y;++y) for(int x=0;x<size.x;++x)
            check(a.grid.blocked({x,y})==b.grid.blocked({x,y}));
        const auto path=findPath(a.grid,a.start,a.goal);
        check(!path.empty() && path.front()==a.start && path.back()==a.goal);
        check(static_cast<int>(path.size())==shortest(a)+1);
        for(std::size_t i=0;i<path.size();++i) {
            check(!a.grid.blocked(path[i]));
            if(i) check(std::abs(path[i].x-path[i-1].x)+std::abs(path[i].y-path[i-1].y)==1);
        }
        std::cout << size.x << 'x' << size.y << " generated, repeatable, A* route verified\n";
    }
    Terrain dense({80,60,95,7}); check(connected(dense.grid,dense.start,dense.goal));
    Terrain empty({20,20,0,42}); check(empty.obstacleCount()==0);
    Terrain different({20,20,35,99}),other({20,20,35,100});
    bool differs=false;
    for(int y=0;y<20;++y) for(int x=0;x<20;++x) differs=differs || different.grid.blocked({x,y})!=other.grid.blocked({x,y});
    check(differs);
    check(empty.edit({0,0},EditTool::SET_START) && empty.start==Cell{0,0});
    check(empty.edit({19,19},EditTool::SET_GOAL) && empty.goal==Cell{19,19});
    check(!empty.edit(empty.start,EditTool::ADD_ROCK));
    check(!empty.edit(empty.goal,EditTool::ADD_ROCK));
    check(empty.edit({2,2},EditTool::ADD_ROCK));
    check(!empty.edit({2,2},EditTool::SET_START));
    check(empty.edit({2,2},EditTool::ERASER) && !empty.grid.blocked({2,2}));
    check(empty.stroke({3,5},{10,5},EditTool::ADD_ROCK));
    for(int x=3;x<=10;++x) check(empty.grid.blocked({x,5}));
    check(empty.stroke({3,5},{10,5},EditTool::ERASER) && empty.obstacleCount()==0);
    check(!empty.edit({-1,0},EditTool::ADD_ROCK));
    Simulation sim(empty.grid,empty.start,empty.goal);
    sim.play(); sim.update(0.5); check(sim.search().iterations()>0);
    sim.enterEdit(); check(sim.status()==SimulationStatus::EDIT_MAP && !sim.playing());
    empty.edit({3,3},EditTool::ADD_ROCK);
    sim.replaceMap(empty.grid,empty.start,empty.goal,true);
    check(sim.search().iterations()==0 && sim.search().closed().empty() && sim.search().path().empty());
    check(!sim.rover().available() && sim.revealed()==0);
    sim.play();
    for(int i=0;sim.status()!=SimulationStatus::PATH_REVEAL;++i) { check(i<400);sim.update(0.125); }
    check(!sim.search().path().empty());
    check(empty.edit({19,18},EditTool::SET_GOAL));
    sim.replaceMap(empty.grid,empty.start,empty.goal,true);
    check(sim.status()==SimulationStatus::EDIT_MAP && sim.search().path().empty());
    check(sim.search().closed().empty() && !sim.search().current());
    check(empty.edit({0,1},EditTool::SET_START));
    sim.replaceMap(empty.grid,empty.start,empty.goal,true);
    sim.update(100);
    check(sim.search().iterations()==0 && !sim.playing());
    // The same screen-to-world conversion used by the editor, at high zoom and after pan.
    WorldCamera camera(80,60,{300,140,980,790});
    CameraInput zoom;zoom.worldHovered=true;zoom.wheel=10;
    for(int i=0;i<100;++i) camera.update(0.05f,CameraPhase::WIDE,{},0,false,zoom);
    auto verifyPick=[&]() {
        Vector2 center=camera.view().target;
        Cell c{static_cast<int>(center.x/64+center.y/32),static_cast<int>(center.y/32-center.x/64)};
        Vector2 screen=GetWorldToScreen2D(WorldCamera::project(c.x+0.5,c.y+0.5),camera.view());
        auto picked=camera.pick(screen,80,60);
        check(picked && *picked==c);
    };
    verifyPick();
    CameraInput pan;pan.worldHovered=true;pan.drag={8,-3};
    for(int i=0;i<30;++i) camera.update(0.05f,CameraPhase::WIDE,{},0,false,pan);
    verifyPick();
    check(!camera.pick({50,200},80,60)); // UI cannot edit the map.
    camera.reset();
    Vector2 outside=GetWorldToScreen2D(WorldCamera::project(-2,10),camera.view());
    check(!camera.pick(outside,80,60));
    bool rejected=false;
    try { Terrain invalid({1,20,25,1}); } catch(const std::invalid_argument&) {rejected=true;}
    check(rejected);
    std::cout << "Terrain/editor tests passed\n";
}
