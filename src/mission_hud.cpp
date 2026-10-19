#include "mission_hud.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

namespace {
Color accent(SimulationStatus s) {
    switch(s) {
    case SimulationStatus::SEARCHING:return Ui::Theme::cyan;
    case SimulationStatus::PATH_REVEAL:case SimulationStatus::PATH_READY:return Ui::Theme::gold;
    case SimulationStatus::DRIVING:return {134,219,198,255};
    case SimulationStatus::RECALCULATING:return {162,210,222,255};
    case SimulationStatus::MISSION_COMPLETE:return {187,224,166,255};
    case SimulationStatus::ROUTE_BLOCKED:case SimulationStatus::NO_ROUTE:case SimulationStatus::NO_PATH:case SimulationStatus::INVALID_ENDPOINTS:return Ui::Theme::orange;
    default:return Ui::Theme::muted;
    }
}
const char* statusName(SimulationStatus s) {
    switch(s) {
    case SimulationStatus::READY:return "READY";case SimulationStatus::EDIT_MAP:return "EDIT MAP";
    case SimulationStatus::SEARCHING:return "SEARCHING";case SimulationStatus::PATH_REVEAL:return "PATH REVEAL";
    case SimulationStatus::PATH_READY:return "PATH READY";case SimulationStatus::DRIVING:return "DRIVING";
    case SimulationStatus::ROUTE_BLOCKED:return "ROUTE BLOCKED";case SimulationStatus::RECALCULATING:return "RECALCULATING";
    case SimulationStatus::NO_ROUTE:return "NO ALTERNATIVE ROUTE";case SimulationStatus::NO_PATH:return "NO ROUTE";
    case SimulationStatus::ARRIVAL:return "ARRIVAL";case SimulationStatus::MISSION_COMPLETE:return "MISSION COMPLETE";
    default:return "INVALID START / GOAL";
    }
}
void heading(Rectangle r,const char* label,float s) {Ui::text(label,{r.x+20*s,r.y+14*s},14*s,Ui::Theme::muted);}
}
bool MissionHud::overControls(const Ui::Layout& l,Vector2 p,bool editing) {
    return CheckCollisionPointRec(p,l.dock) || CheckCollisionPointRec(p,l.bar) || CheckCollisionPointRec(p,l.minimap) ||
           CheckCollisionPointRec(p,l.brand) || CheckCollisionPointRec(p,l.topActions) || (editing && CheckCollisionPointRec(p,l.editor)) ||
           (!editing && CheckCollisionPointRec(p,{l.world.width-206*l.scale,l.bar.y-66*l.scale,182*l.scale,46*l.scale}));
}
HudActions MissionHud::input(const Ui::Layout& l,const Simulation& sim,const Terrain& t,const WorldCamera& c,
                             const VisualEffects& e,AudioSystem& a,bool blocked,bool presentation) {
    Ui::pass(Ui::Pass::INPUT);auto result=controls(l,sim,t,c,e,a,blocked,presentation);Ui::pass(Ui::Pass::BOTH);return result;
}
HudActions MissionHud::controls(const Ui::Layout& l,const Simulation& sim,const Terrain& terrain,const WorldCamera& camera,
                                const VisualEffects& effects,AudioSystem& audio,bool blocked,bool presentation) {
    HudActions a;const float s=l.scale,p=20*s;
    const Rectangle content{l.settings.x+8*s,l.settings.y+42*s,l.settings.width-16*s,l.settings.height-50*s};
    bool inSettings=false;
    auto inputBlocked=[&]() {return blocked || (inSettings && !CheckCollisionPointRec(GetMousePosition(),content));};
    auto button=[&](Rectangle r,const char* label,Ui::Icon icon=Ui::Icon::NONE,bool enabled=true) {
        bool clicked=Ui::button(r,label,enabled,inputBlocked(),static_cast<int>(18*s),icon);if(clicked) audio.click();return clicked;
    };
    auto toggle=[&](Rectangle r,const char* label,bool& value,Ui::Icon icon=Ui::Icon::NONE) {
        bool changed=Ui::toggle(r,label,value,inputBlocked(),icon);if(changed) audio.click();return changed;
    };
    auto slider=[&](Rectangle r,const char* label,float& value,float low,float high,const char* format,float multiplier=1,bool integer=false) {
        bool changed=Ui::slider(r,label,value,low,high,inputBlocked(),format,multiplier,integer);
        if(changed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) audio.click();
        return changed;
    };
    float bx=l.bar.x+16*s,by=l.bar.y+24*s,bw=104*s;
    const char* labels[]{"PLAY","PAUSE","STEP","RESET"};
    const Ui::Icon icons[]{Ui::Icon::PLAY,Ui::Icon::PAUSE,Ui::Icon::STEP,Ui::Icon::RESET};
    const unsigned flags[]{HudEvent::PLAY,HudEvent::PAUSE,HudEvent::STEP,HudEvent::RESET};
    for(int i=0;i<4;++i) if(button({bx+i*(bw+8*s),by,bw,48*s},labels[i],icons[i],i==0||i==2?sim.active():i==1?sim.playing():true)) a.flags|=flags[i];
    float sx=bx+4*(bw+8*s)+14*s,sw=(l.bar.x+l.bar.width-20*s-sx-24*s)/2;
    slider({sx,l.bar.y+17*s,sw,62*s},"SEARCH SPEED",settings.searchSpeed,1,60,"%.0f /s",1,true);
    slider({sx+sw+24*s,l.bar.y+17*s,sw,62*s},"DRIVE SPEED",settings.driveSpeed,0.25f,5,"%.2f /s");
    Ui::panel(l.topActions,18*s);
    if(button({l.topActions.x+8*s,l.topActions.y+5*s,166*s,44*s},"PRESENTATION")) a.flags|=HudEvent::PRESENTATION;
    if(button({l.topActions.x+180*s,l.topActions.y+5*s,100*s,44*s},"HELP",Ui::Icon::HELP)) a.flags|=HudEvent::HELP;
    const char* tabs[]{"MAP","CAMERA","FX","AUDIO"};
    Ui::panel(l.tabs,15*s);
    float tw=(l.tabs.width-12*s)/4;
    for(int i=0;i<4;++i) {
        Rectangle r{l.tabs.x+6*s+i*tw,l.tabs.y+4*s,tw,34*s};
        const bool hover=!blocked && CheckCollisionPointRec(GetMousePosition(),r);
        if(Ui::drawing()) {
            if(tab_==i || hover) DrawRectangleRounded(r,0.6f,8,Fade(Ui::Theme::cyan,tab_==i?0.12f:0.05f));
            Ui::centered(tabs[i],r,16*s,tab_==i?Ui::Theme::text:Ui::Theme::muted);
        }
        if(Ui::input() && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {tab_=i;audio.click();}
    }
    const Rectangle r=l.settings;
    if(Ui::input() && !blocked && CheckCollisionPointRec(GetMousePosition(),content))
        scroll_[tab_]=std::clamp(scroll_[tab_]-GetMouseWheelMove()*34*s,0.0f,std::max(0.0f,332*s-content.height));
    const float x=r.x+p,y=r.y+48*s-scroll_[tab_],w=r.width-2*p;
    Ui::panel(r,22*s);
    const char* sectionTitles[]{"MAP / TERRAIN","CAMERA","ENVIRONMENT","AUDIO"};
    heading(r,sectionTitles[tab_],s);
    inSettings=true;
    if(Ui::drawing()) BeginScissorMode(int(content.x),int(content.y),int(content.width),int(content.height));
    if(tab_==0) {
        Ui::text(TextFormat("%d x %d",terrain.grid.width(),terrain.grid.height()),{x,y},30*s);
        const int count=terrain.obstacleCount();
        Ui::text(TextFormat("OBSTACLES   %d / %.1f%%",count,100.0*count/(terrain.grid.width()*terrain.grid.height())),{x,y+43*s},17*s,Ui::Theme::muted);
        Ui::text(TextFormat("SEED   %u",terrain.config.seed),{x,y+71*s},17*s,Ui::Theme::muted);
        float bw2=(w-12*s)/2;
        if(button({x,y+112*s,bw2,46*s},"NEW MAP",Ui::Icon::MAP)) a.flags|=HudEvent::NEW_MAP;
        if(button({x+bw2+12*s,y+112*s,bw2,46*s},"GENERATE")) a.flags|=HudEvent::GENERATE;
        if(button({x,y+170*s,bw2,46*s},"EDIT MAP",Ui::Icon::EDIT)) a.flags|=HudEvent::EDIT;
        if(button({x+bw2+12*s,y+170*s,bw2,46*s},"CLEAR PATH",Ui::Icon::RESET)) a.flags|=HudEvent::CLEAR;
        toggle({x,y+233*s,w,42*s},"SHOW COSTS",settings.showCosts);
        Ui::fitText("COSTS: g / h / f on explored cells",{x,y+286*s,w,28*s},15*s,Ui::Theme::muted);
    } else if(tab_==1) {
        bool automatic=camera.automatic();
        if(toggle({x,y,w,44*s},"AUTO CAMERA",automatic,Ui::Icon::CAMERA)) a.flags|=HudEvent::AUTO_CAMERA;
        float zoom=camera.view().zoom;
        const Rectangle zoomRect{x,y+70*s,w,68*s};
        if(Ui::input() && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) zoomDragging_=false;
        float control=zoomDragging_?lastZoom_:zoom;
        if(slider(zoomRect,"ZOOM",control,camera.fitZoom(),camera.maxZoom(),"%.2fx")) {
            a.zoomWheel=std::log(control/(zoomDragging_?lastZoom_:zoom))/0.15f;lastZoom_=control;zoomDragging_=true;
        }
        if(button({x,y+166*s,w,48*s},"FIT TO MAP",Ui::Icon::CAMERA)) a.flags|=HudEvent::FIT_CAMERA;
        Ui::fitText("WHEEL  Zoom / MMB or WASD  Pan",{x,y+238*s,w,30*s},16*s,Ui::Theme::muted);
        Ui::fitText("Manual input temporarily takes control.",{x,y+272*s,w,30*s},15*s,Ui::Theme::muted);
    } else if(tab_==2) {
        bool storm=effects.storm();
        if(toggle({x,y,w,44*s},"SANDSTORM",storm,Ui::Icon::STORM)) a.flags|=HudEvent::STORM;
        Ui::text("STORM INTENSITY",{x,y+78*s},15*s,Ui::Theme::muted);
        const char* levels[]{"LIGHT","MEDIUM","HEAVY"};
        for(int i=0;i<3;++i) {
            Rectangle option{x+i*(w+8*s)/3,y+109*s,(w-16*s)/3,44*s};
            if(button(option,levels[i])) a.stormLevel=i;
            if(Ui::drawing() && static_cast<int>(effects.sandstorm().intensity())==i) DrawRectangleRoundedLines(option,0.5f,8,Fade(Ui::Theme::gold,0.38f));
        }
        Ui::text("UI GLASS QUALITY",{x,y+193*s},15*s,Ui::Theme::muted);
        if(button({x,y+225*s,(w-12*s)/2,44*s},"LOW")) Ui::quality(Ui::Quality::LOW);
        if(button({x+(w+12*s)/2,y+225*s,(w-12*s)/2,44*s},"HIGH")) Ui::quality(Ui::Quality::HIGH);
        if(Ui::drawing()) {
            Rectangle selected{x+(Ui::quality()==Ui::Quality::HIGH?(w+12*s)/2:0),y+225*s,(w-12*s)/2,44*s};
            DrawRectangleRoundedLines(selected,0.6f,8,Fade(Ui::Theme::cyan,0.30f));
        }
        Ui::text(Ui::quality()==Ui::Quality::HIGH?"HIGH / local backdrop blur":"LOW / lightweight glass",{x,y+290*s},16*s,Ui::Theme::muted);
    } else {
        toggle({x,y,w,44*s},"SOUND",audio.settings.enabled,Ui::Icon::AUDIO);
        slider({x,y+72*s,w,60*s},"MASTER",audio.settings.master,0,1,"%.0f%%",100);
        slider({x,y+158*s,w,60*s},"MUSIC / AMBIENT",audio.settings.ambient,0,1,"%.0f%%",100);
        slider({x,y+244*s,w,60*s},"SFX",audio.settings.sfx,0,1,"%.0f%%",100);
    }
    if(Ui::drawing()) EndScissorMode();
    if(Ui::drawing() && content.height<332*s) {
        const float thumb=content.height*content.height/(332*s);
        const float offset=scroll_[tab_]/(332*s-content.height)*(content.height-thumb);
        Rectangle rail{r.x+r.width-9*s,content.y,3*s,content.height};
        DrawRectangleRounded(rail,1,6,Fade(Ui::Theme::text,0.05f));
        DrawRectangleRounded({rail.x,rail.y+offset,rail.width,thumb},1,6,Fade(Ui::Theme::cyan,0.28f));
    }
    inSettings=false;
    if(sim.status()==SimulationStatus::EDIT_MAP) {
        Ui::panel(l.editor,18*s);
        const char* tools[]{"SET START","SET GOAL","ADD ROCK","ERASER"};
        float ew=(l.editor.width-24*s)/4;
        for(int i=0;i<4;++i) {
            Rectangle tool{l.editor.x+8*s+i*(ew+2*s),l.editor.y+8*s,ew-2*s,46*s};
            if(button(tool,tools[i],i<2?Ui::Icon::EDIT:Ui::Icon::ROCK)) settings.tool=static_cast<EditTool>(i);
            if(Ui::drawing() && static_cast<int>(settings.tool)==i) DrawRectangleRoundedLines(tool,0.4f,8,Fade(Ui::Theme::gold,0.45f));
        }
    } else {
        Rectangle drop{l.world.width-206*s,l.bar.y-66*s,182*s,46*s};
        if(button(drop,presentation?"DEMO BLOCK ROUTE":"DROP ROCK",Ui::Icon::ROCK,sim.canDropRock())) a.flags|=HudEvent::DROP_ROCK;
    }
    return a;
}
void MissionHud::draw(const Ui::Layout& l,const Simulation& sim,const Terrain& terrain,const WorldCamera& camera,
                      const VisualEffects& effects,AudioSystem& audio,const MissionFeedback& feedback,bool blocked,bool presentation) {
    Ui::pass(Ui::Pass::DRAW);const float s=l.scale,p=20*s;
    appearance_=std::min(1.0f,appearance_+Ui::delta()/0.25f);
    Ui::opacity(appearance_);
    rlPushMatrix();rlTranslatef(0,(1-appearance_)*6*s,0);
    if(previous_!=sim.status()) {oldStatus_=previous_;previous_=sim.status();statusAge_=0;}
    else statusAge_+=Ui::delta();
    const float t=std::clamp(statusAge_/Ui::Theme::transition,0.0f,1.0f);
    Color color=accent(sim.status());
    Ui::panel(l.brand,22*s);Ui::text("ARES / MISSION CONTROL",{l.brand.x+p,l.brand.y+14*s},25*s);
    Ui::text("MARS ROVER  /  A* NAVIGATION",{l.brand.x+p,l.brand.y+47*s},14*s,Ui::Theme::muted);
    Ui::panel(l.status,22*s,color,0.35f+feedback.flash()*0.16f);
    heading(l.status,"MISSION STATUS",s);
    const float statusY=l.status.y+40*s;
    if(t<1) Ui::fitText(statusName(oldStatus_),{l.status.x+p,statusY-4*t*s,l.status.width-2*p-18*s,38*s},32*s,Fade(accent(oldStatus_),1-t));
    const char* current=sim.status()==SimulationStatus::PATH_REVEAL && statusAge_<0.5f?"PATH FOUND":statusName(sim.status());
    Ui::fitText(current,{l.status.x+p,statusY+(1-t)*5*s,l.status.width-2*p-18*s,38*s},32*s,Fade(color,t));
    DrawCircleV({l.status.x+l.status.width-26*s,l.status.y+56*s},4*s,color);
    DrawCircleV({l.status.x+l.status.width-26*s,l.status.y+56*s},8*s,Fade(color,0.10f));
    Ui::text(sim.playing()?"LIVE / PLAYING":"STANDBY / PAUSED",{l.status.x+p,l.status.y+85*s},13*s,Ui::Theme::muted);
    Ui::panel(l.navigation,22*s);heading(l.navigation,"NAVIGATION",s);
    const auto& search=sim.search();const auto* n=search.current()?&*search.current():nullptr;
    Ui::text(n?TextFormat("CURRENT NODE   %d / %d",n->cell.x,n->cell.y):"CURRENT NODE   - / -",{l.navigation.x+p,l.navigation.y+39*s},21*s);
    double values[]{n?double(n->g):-1,n?n->h:-1,n?n->f:-1,double(search.open().size()),double(search.closed().size()),double(search.iterations())};
    for(int i=0;i<6;++i) {if(values[i]!=lastValues_[i]) {pulses_[i]=1;lastValues_[i]=values[i];}pulses_[i]=std::max(0.0f,pulses_[i]-Ui::delta()/0.22f);}
    const char* costLabels[]{"g","h","f"};float cw=(l.navigation.width-2*p-16*s)/3;
    for(int i=0;i<3;++i) {
        Rectangle card{l.navigation.x+p+i*(cw+8*s),l.navigation.y+73*s,cw,61*s};
        Ui::panel(card,13*s,Ui::Theme::cyan,pulses_[i]*0.10f,0.92f);
        Ui::text(costLabels[i],{card.x+12*s,card.y+6*s},14*s,Ui::Theme::muted);
        Ui::fitText(values[i]<0?"-":TextFormat(i==0?"%.0f":"%.2f",values[i]),{card.x+12*s,card.y+24*s,cw-20*s,32*s},29*s);
    }
    Ui::panel(l.search,22*s);heading(l.search,"SEARCH",s);
    const char* names[]{"OPEN","CLOSED","ITERATIONS"};
    for(int i=0;i<3;++i) {
        float x=l.search.x+p+i*(cw+8*s);
        Ui::text(names[i],{x,l.search.y+44*s},14*s,i==0?Ui::Theme::cyan:Ui::Theme::muted);
        Ui::fitText(TextFormat("%.0f",values[i+3]),{x,l.search.y+62*s,cw,32*s},28*s,i==0?Ui::Theme::cyan:Ui::Theme::text);
        if(pulses_[i+3]>0) DrawLineEx({x,l.search.y+98*s},{x+cw,l.search.y+98*s},1,Fade(Ui::Theme::cyan,pulses_[i+3]*0.15f));
    }
    Ui::fitText(TextFormat("PATH %d  /  REVEALED %d / %d",int(sim.pathLength()),int(sim.revealed()),int(sim.pathLength())),{l.search.x+p,l.search.y+102*s,l.search.width-2*p,24*s},16*s,Ui::Theme::gold);
    Ui::panel(l.mission,22*s);heading(l.mission,"MISSION",s);
    Ui::text(TextFormat("TRAVELLED  %.2f",feedback.travelled()),{l.mission.x+p,l.mission.y+42*s},21*s);
    Ui::text(TextFormat("REPLANS  %d",int(sim.replans())),{l.mission.x+l.mission.width*0.60f,l.mission.y+42*s},17*s,Ui::Theme::orange);
    Ui::text(TextFormat("MISSION TIME  %.1f s",feedback.missionTime()),{l.mission.x+p,l.mission.y+73*s},20*s);
    Ui::panel(l.bar,28*s);
    controls(l,sim,terrain,camera,effects,audio,blocked,presentation);
    if(sim.rover().available()) Ui::fitText(TextFormat("ROVER  %.2f / %.2f    SEGMENT %d / %d",sim.rover().x(),sim.rover().y(),int(std::min(sim.rover().segment()+1,sim.pathLength())),int(sim.pathLength())),
        {l.world.width*0.30f,l.bar.y-31*s,l.world.width*0.43f,22*s},14*s,Ui::Theme::muted);
    rlPopMatrix();Ui::opacity(1);Ui::pass(Ui::Pass::BOTH);
}
