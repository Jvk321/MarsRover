#include "mission_hud.h"
#include <cstdlib>
#include <iostream>
void require(bool ok,int line) {if(!ok) {std::cerr<<"HUD layout test failed at "<<line<<'\n';std::exit(1);}}
#define check(...) require((__VA_ARGS__),__LINE__)
bool inside(Rectangle r,Rectangle bounds) {
    return r.x>=bounds.x-0.01f && r.y>=bounds.y-0.01f && r.x+r.width<=bounds.x+bounds.width+0.01f && r.y+r.height<=bounds.y+bounds.height+0.01f;
}
int main() {
    for(auto size:{Vector2{1280,800},Vector2{1600,1000},Vector2{1920,1080},Vector2{2560,1440},Vector2{3840,2160}}) {
        auto l=Ui::layout(size.x,size.y);
        for(auto panel:{l.status,l.navigation,l.search,l.mission,l.tabs,l.settings,l.bar,l.minimap,l.brand,l.topActions,l.editor}) check(inside(panel,l.canvas));
        for(auto panel:{l.status,l.navigation,l.search,l.mission,l.tabs,l.settings}) check(inside(panel,l.dock));
        check(l.status.y+l.status.height<l.navigation.y);
        check(l.navigation.y+l.navigation.height<l.search.y);
        check(l.search.y+l.search.height<l.mission.y);
        check(!CheckCollisionRecs(l.bar,l.minimap));
        check(!CheckCollisionRecs(l.brand,l.topActions));
        if(size.x==1920) {check(l.dock.width>=380 && l.dock.width<=430);check(l.world.width/size.x>=0.75f && l.world.width/size.x<=0.80f);}
        if(size.x==2560) {check(l.dock.width>=440 && l.dock.width<=500);check(l.world.width/size.x>=0.75f && l.world.width/size.x<=0.81f);}
        check(MissionHud::overControls(l,{l.bar.x+5,l.bar.y+5},false));
        check(MissionHud::overControls(l,{l.minimap.x+5,l.minimap.y+5},false));
        check(MissionHud::overControls(l,{l.dock.x+5,l.dock.y+5},false));
        check(MissionHud::overControls(l,{l.editor.x+5,l.editor.y+5},true));
        check(!MissionHud::overControls(l,{l.world.width/2,size.y/2},false));
        check(48*l.scale>=40);
    }
    check(Ui::layout(2560,1440).scale>Ui::layout(1920,1080).scale);
    MissionHud hud;hud.selectTab(HudTab::AUDIO);check(hud.selectedTab()==HudTab::AUDIO);
    Ui::quality(Ui::Quality::LOW);check(Ui::quality()==Ui::Quality::LOW);
    Ui::quality(Ui::Quality::HIGH);check(Ui::quality()==Ui::Quality::HIGH);
    std::cout<<"HUD bounds, 1080p/1440p sizing, world area, input exclusion and quality passed\n";
}
