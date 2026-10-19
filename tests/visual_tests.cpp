#include "visual_effects.h"
#include "visual_config.h"
#include "scene_renderer.h"
#include "terrain_art.h"
#include "world_art.h"
#include <cstdlib>
#include <iostream>
void checkImpl(bool value,int line) {
    if(!value) { std::cerr << "Visual test failed at " << line << '\n';std::exit(1); }
}
#define check(...) checkImpl((__VA_ARGS__),__LINE__)
int main() {
    check(SceneRenderer::tileHash(3,8,42)==SceneRenderer::tileHash(3,8,42));
    check(SceneRenderer::tileHash(3,8,42)!=SceneRenderer::tileHash(3,8,43));
    Grid formation(20,20);formation.setBlocked({4,5});formation.setBlocked({5,5});formation.setBlocked({6,5});
    check(TerrainArt::cliff(formation,{5,5}) && !TerrainArt::cliff(formation,{4,5}));
    auto height=TerrainArt::cliffHeight(5,5,42);
    check(height==TerrainArt::cliffHeight(5,5,42) && height!=TerrainArt::cliffHeight(5,5,43));
    WorldArt::Props props;
    for(auto size:{Cell{2,2},Cell{20,20},Cell{50,50},Cell{80,60}}) {
        Grid grid(size.x,size.y);auto count=WorldArt::props(grid,42,props);check(count<=props.size());
        for(std::size_t i=0;i<count;++i) {
            const auto& prop=props[i];
            check(prop.x<0 || prop.y<0 || prop.x>=size.x || prop.y>=size.y);
        }
        if(size.x==2) check(count==0);
    }
    Rectangle viewport{300,140,980,790};auto inset=SceneRenderer::minimapBounds(viewport);
    check(inset.x>=viewport.x && inset.y>=viewport.y && inset.x+inset.width<=viewport.x+viewport.width);
    Simulation sim(Grid(80,60),{1,1},{78,58});sim.setSpeed(60);sim.setDrivingSpeed(3);
    VisualEffects effects;effects.reset(42);sim.play();
    sim.update(1.0/60);effects.update(1.0f/60,sim,{1,1});
    check(effects.sonar().strength()==1 && effects.sonar().angle()>0);
    sim.pause();float time=effects.time(),angle=effects.sonar().angle();
    effects.update(1,sim,{1,1});
    check(effects.time()==time && effects.sonar().angle()==angle);
    sim.play();
    for(int i=0;sim.status()!=SimulationStatus::DRIVING;++i) {
        check(i<20000);sim.update(1.0/60);effects.update(1.0f/60,sim,{1,1});
    }
    for(int i=0;i<120;++i) { sim.update(1.0/60);effects.update(1.0f/60,sim,{1,1}); }
    check(!effects.tracks().empty() && !effects.dust().empty());
    sim.pause();auto tracks=effects.tracks().size(),dust=effects.dust().size();
    time=effects.time();float age=effects.dust()[0].age;
    effects.update(100,sim,{1,1});
    check(effects.time()==time && effects.tracks().size()==tracks && effects.dust().size()==dust);
    check(effects.dust()[0].age==age);
    effects.setStorm(true);check(effects.storm());sim.play();
    for(int i=0;sim.status()!=SimulationStatus::MISSION_COMPLETE;++i) {
        check(i<30000);sim.update(1.0/60);effects.update(1.0f/60,sim,{1,1});
        check(effects.dust().size()<=VisualConfig::maxDust);
        check(effects.tracks().size()<=VisualConfig::maxTracks);
    }
    // Contact impressions belong to the driven history, never to A* geometry.
    const auto history=effects.tracks().size();
    const auto first=effects.tracks().front().to;
    sim.pause();effects.update(1,sim,{1,1});
    check(effects.tracks().size()==history && effects.tracks().front().to.x==first.x);
    for(const auto& mark:effects.tracks()) {
        check(std::isfinite(mark.to.x) && std::isfinite(mark.to.y));
    }
    effects.setStorm(false);check(!effects.storm());
    effects.reset(42);check(effects.dust().empty() && effects.tracks().empty() && effects.sonar().angle()==0);
    // All four isometric headings: actual rear contacts stay halfTrack from
    // the driven centerline. Replay reproduces the same sample positions.
    for(Cell direction:{Cell{1,0},Cell{0,1},Cell{-1,0},Cell{0,-1}}) {
        Cell origin{5,5},goal{5+direction.x*3,5+direction.y*3};
        Simulation drive(Grid(20,20),origin,goal);drive.setSpeed(60);
        VisualEffects tire;tire.reset(17);
        std::vector<Vector2> recorded;
        for(int replay=0;replay<2;++replay) {
            drive.reset();drive.setDrivingSpeed(replay?5:0.25);tire.reset(17);drive.play();
            for(int step=0;drive.status()!=SimulationStatus::MISSION_COMPLETE;++step) {
                check(step<20000);drive.update(1.0/60);tire.update(1.0f/60,drive,origin);
            }
            check(!tire.tracks().empty());
            for(const auto& mark:tire.tracks()) {
                float cross=(mark.to.x-origin.x-0.5f)*direction.y-(mark.to.y-origin.y-0.5f)*direction.x;
                check(std::abs(std::abs(cross)-RoverGeometry::halfTrack)<0.0001);
            }
            if(!replay) for(const auto& mark:tire.tracks()) recorded.push_back(mark.to);
            else {
                check(recorded.size()==tire.tracks().size());
                for(std::size_t k=0;k<recorded.size();++k)
                    check(std::hypot(recorded[k].x-tire.tracks()[k].to.x,recorded[k].y-tire.tracks()[k].to.y)<0.0001f);
            }
        }
    }
    std::cout << "Visual state, deterministic art, pause and limits passed\n";
}


