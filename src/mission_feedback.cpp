#include "mission_feedback.h"
#include "presentation.h"
#include "ui_controls.h"
#include <algorithm>
#include <cmath>
namespace {
bool action(Rectangle r,const char* label,AudioSystem& audio,Ui::Icon icon=Ui::Icon::NONE) {
    bool pressed=Ui::button(r,label,true,false,static_cast<int>(std::clamp(r.height*0.40f,17.0f,24.0f)),icon);
    if(pressed) audio.click();
    return pressed;
}
}
void MissionFeedback::reset() {
    status_=previous_=SimulationStatus::READY;statusAge_=100;overlayAge_=0;
    missionTime_=searchTime_=travelled_=0;iterations_=lastIterations_=0;hadPose_=false;
}
bool MissionFeedback::terminal() const {
    return status_==SimulationStatus::MISSION_COMPLETE || status_==SimulationStatus::NO_ROUTE ||
           status_==SimulationStatus::NO_PATH || status_==SimulationStatus::INVALID_ENDPOINTS;
}
float MissionFeedback::flash() const {
    float entry=std::max(0.0f,1-statusAge_/0.7f);
    if(status_==SimulationStatus::ROUTE_BLOCKED) return 0.25f+0.45f*(0.5f+0.5f*std::sin(statusAge_*9));
    return entry;
}
void MissionFeedback::update(float dt,const Simulation& sim) {
    float elapsed=std::isfinite(dt)?std::max(0.0f,dt):0;
    dt=std::min(elapsed,0.1f);status_=sim.status();
    if(status_!=previous_) {statusAge_=0;overlayAge_=0;}
    else { statusAge_+=dt;overlayAge_+=dt; }
    if(sim.playing() && sim.active()) missionTime_+=elapsed;
    const auto& r=sim.rover();
    if(r.available() && hadPose_) travelled_+=std::hypot(r.x()-lastX_,r.y()-lastY_);
    hadPose_=r.available();lastX_=r.x();lastY_=r.y();
    auto count=sim.search().iterations();
    const auto searching=[](SimulationStatus s) {
        return s==SimulationStatus::SEARCHING || s==SimulationStatus::RECALCULATING;
    };
    // Active playback time, including the frame that finishes a search. Excludes pauses,
    // reveal, driving and the blocked-route hold; manual STEP counts its feedback frame.
    if ((searching(status_) || searching(previous_) || count!=lastIterations_) &&
        (sim.playing() || count!=lastIterations_)) searchTime_+=elapsed;
    iterations_+=count>=lastIterations_?count-lastIterations_:count;lastIterations_=count;
    previous_=status_;
}
FeedbackAction MissionFeedback::draw(Rectangle v,const Terrain& terrain,const Simulation& sim,AudioSystem& audio) {
    const float s=Ui::layout(GetScreenWidth(),GetScreenHeight()).scale;
    if(!blocksInput()) {
        wasModal_=false;
        const char* message=nullptr;Color color=Ui::Theme::cyan;
        if(status_==SimulationStatus::ROUTE_BLOCKED) {message="ROUTE BLOCKED";color=Ui::Theme::orange;}
        else if(statusAge_<1.8f) {
            if(status_==SimulationStatus::PATH_REVEAL || status_==SimulationStatus::PATH_READY) {message="PATH FOUND / REVEALING ROUTE";color=Ui::Theme::gold;}
            if(status_==SimulationStatus::SEARCHING) message="SEARCHING / A* ONLINE";
            if(status_==SimulationStatus::RECALCULATING) {message="RECALCULATING / NEW ROUTE";color=Ui::Theme::orange;}
            if(status_==SimulationStatus::DRIVING) message="DRIVING / ROUTE ACTIVE";
        }
        if(message) {
            float fade=std::clamp(statusAge_/0.18f,0.0f,1.0f);
            if(status_!=SimulationStatus::ROUTE_BLOCKED) fade*=std::clamp((1.8f-statusAge_)/0.3f,0.0f,1.0f);
            const float width=std::min(440*s,v.width-48*s);
            Rectangle toast{v.x+(v.width-width)/2,v.y+126*s-(1-fade)*6*s,width,52*s};
            Ui::opacity(fade);Ui::panel(toast,26*s,color,0.25f);
            DrawCircleV({toast.x+23*s,toast.y+26*s},3*s,Fade(color,fade));
            Ui::fitText(message,{toast.x+40*s,toast.y,toast.width-56*s,toast.height},18*s,color);Ui::opacity(1);
        }
        return FeedbackAction::NONE;
    }
    if(!wasModal_) {modalAge_=0;wasModal_=true;}else modalAge_+=Ui::delta();
    float entry=std::clamp(modalAge_/0.24f,0.0f,1.0f);entry=entry*entry*(3-2*entry);
    const bool success=status_==SimulationStatus::MISSION_COMPLETE;
    const float width=std::min((helpOpen?830.0f:780.0f)*s,v.width-48*s);
    const float requested=helpOpen?680:audioOpen?550:presentationOpen?570:success?625:410;
    float height=std::min(requested*s,v.height-48*s);
    Rectangle panel{v.x+(v.width-width)/2,v.y+(v.height-height)/2+(1-entry)*7*s,width,height};
    panel.x+=width*(1-entry)*0.007f;panel.width*=1-(1-entry)*0.014f;
    const float x=panel.x+28*s,w=panel.width-56*s,cx=panel.x+panel.width/2;
    DrawRectangleRec(v,Fade(Color{6,13,23,255},0.18f*entry));
    Color color=terminal()&&!helpOpen&&!audioOpen&&!presentationOpen?(success?Ui::Theme::gold:Ui::Theme::orange):Ui::Theme::cyan;
    Ui::opacity(entry);Ui::panel(panel,26*s,color,0.10f);
    auto title=[&](const char* eyebrow,const char* label,const char* subtitle) {
        Ui::text(eyebrow,{x,panel.y+25*s},13*s,Ui::Theme::muted);
        Ui::fitText(label,{x,panel.y+49*s,w,48*s},38*s,color);
        Ui::fitText(subtitle,{x,panel.y+101*s,w,30*s},17*s,Ui::Theme::muted);
    };
    FeedbackAction result=FeedbackAction::NONE;
    if(helpOpen) {
        title("ARES / OPERATOR BRIEFING","MARS ROVER A* NAVIGATION","2.5D A* pathfinding and dynamic replanning simulator");
        const char* keys[]{"SPACE","N / STEP","R / RESET","MOUSE WHEEL","MMB / WASD","AUTO CAMERA","EDIT MAP","DROP ROCK","C / SHOW COSTS","SANDSTORM","AUDIO / F2"};
        const char* descriptions[]{"Play / pause the mission","One A* iteration, reveal segment or drive step","Reset mission, effects and camera","Zoom the world","Pan the camera","Soft following and cinematic arrival","Set start, goal, rocks and eraser","Block the route and trigger replanning","Show g / h / f on explored cells","Toggle wind and LIGHT / MEDIUM / HEAVY","Sound, master, ambient and effects"};
        float y=panel.y+158*s;
        for(int i=0;i<11;++i) {
            Ui::text(keys[i],{x,y+i*35*s},15*s,Ui::Theme::cyan);
            Ui::fitText(descriptions[i],{x+176*s,y+i*35*s-3*s,w-176*s,27*s},18*s,Ui::Theme::text);
        }
        float bw=(w-14*s)/2;
        if(action({x,panel.y+height-80*s,bw,50*s},"START MISSION",audio,Ui::Icon::PLAY)) {helpOpen=false;result=FeedbackAction::START_MISSION;}
        if(action({x+bw+14*s,panel.y+height-80*s,bw,50*s},"PRESENTATION MODE",audio)) {helpOpen=false;presentationOpen=true;wasModal_=false;}
    } else if(audioOpen) {
        title("MISSION SETTINGS","AUDIO / MISSION MIXER","A quiet ambient bed, clear sonar and distinct alerts.");
        if(Ui::toggle({x,panel.y+155*s,w,46*s},"SOUND",audio.settings.enabled,false,Ui::Icon::AUDIO)) audio.click();
        Ui::slider({x,panel.y+232*s,w,58*s},"MASTER VOLUME",audio.settings.master,0,1,false,"%.0f%%",100);
        Ui::slider({x,panel.y+318*s,w,58*s},"MUSIC / AMBIENT",audio.settings.ambient,0,1,false,"%.0f%%",100);
        Ui::slider({x,panel.y+404*s,w,58*s},"SFX",audio.settings.sfx,0,1,false,"%.0f%%",100);
        if(action({x,panel.y+height-68*s,w,46*s},"CLOSE AUDIO",audio)) audioOpen=false;
    } else if(presentationOpen) {
        title("DEMONSTRATION PRESETS","PRESENTATION MODE","Fixed seeds / full control / manual DROP ROCK");
        for(std::size_t i=0;i<presentationPresets.size();++i) {
            const auto& preset=presentationPresets[i];Rectangle card{x,panel.y+155*s+i*94*s,w,80*s};
            if(action(card,"",audio)) selectedPreset=i;
            if(selectedPreset==i) DrawRectangleRoundedLines(card,0.4f,8,Fade(Ui::Theme::gold,0.45f*entry));
            Ui::text(preset.name,{card.x+18*s,card.y+13*s},22*s);
            Ui::text(TextFormat("%d x %d / %d%% rocks / seed %u",preset.terrain.width,preset.terrain.height,preset.terrain.density,preset.terrain.seed),{card.x+18*s,card.y+46*s},16*s,Ui::Theme::muted);
        }
        Ui::fitText("During driving, use DEMO BLOCK ROUTE to demonstrate replanning.",{x,panel.y+451*s,w,26*s},16*s,Ui::Theme::muted);
        if(action({x,panel.y+height-68*s,(w-14*s)/2,46*s},"START DEMO",audio,Ui::Icon::PLAY)) {presentationOpen=false;result=FeedbackAction::START_DEMO;}
        if(action({cx+7*s,panel.y+height-68*s,(w-14*s)/2,46*s},"CANCEL",audio)) presentationOpen=false;
    } else if(terminal()) {
        const char* failure=status_==SimulationStatus::INVALID_ENDPOINTS?"INVALID START / GOAL":status_==SimulationStatus::NO_PATH?"NO ROUTE TO GOAL":"NO ALTERNATIVE ROUTE";
        title(success?"MISSION REPORT / GOAL ACQUIRED":"NAVIGATION ALERT",success?"MISSION COMPLETE":failure,
              success?"Rover secured. Navigation sequence completed.":"Rover stopped safely. Operator action required.");
        if(success) {
            const char* labels[]{"PATH LENGTH","DISTANCE TRAVELLED","ITERATIONS","REPLANS","SEARCH TIME","MISSION TIME","MAP SIZE","SEED"};
            for(int i=0;i<8;++i) {
                Rectangle card{x+(i%2)*(w+14*s)/2,panel.y+159*s+(i/2)*87*s,(w-14*s)/2,74*s};
                Ui::panel(card,15*s,Ui::Theme::gold,0.025f,0.94f);
                Ui::text(labels[i],{card.x+16*s,card.y+12*s},13*s,Ui::Theme::muted);
                const char* value="";
                switch(i) {
                case 0:value=TextFormat("%d segments",int(sim.pathLength()));break;
                case 1:value=TextFormat("%.2f tiles",travelled_);break;
                case 2:value=TextFormat("%d",int(iterations_));break;
                case 3:value=TextFormat("%d",int(sim.replans()));break;
                case 4:value=TextFormat("%.1f s",searchTime_);break;
                case 5:value=TextFormat("%.1f s",missionTime_);break;
                case 6:value=TextFormat("%d x %d",terrain.grid.width(),terrain.grid.height());break;
                case 7:value=TextFormat("%u",terrain.config.seed);break;
                }
                Ui::text(value,{card.x+16*s,card.y+35*s},25*s);
            }
        } else {
            Ui::text("Edit the terrain or reset the mission to try again.",{x,panel.y+177*s},20*s);
            Ui::text("Your map remains available.",{x,panel.y+216*s},17*s,Ui::Theme::muted);
        }
        float bw=(w-24*s)/3,y=panel.y+height-76*s;
        if(action({x,y,bw,50*s},success?"NEW MISSION":"NEW MAP",audio,Ui::Icon::MAP)) result=FeedbackAction::NEW_MISSION;
        if(action({x+bw+12*s,y,bw,50*s},success?"REPLAY":"RESET",audio,Ui::Icon::RESET)) result=success?FeedbackAction::REPLAY:FeedbackAction::RESET;
        if(action({x+2*(bw+12*s),y,bw,50*s},"EDIT MAP",audio,Ui::Icon::EDIT)) result=FeedbackAction::EDIT;
    }
    Ui::opacity(1);return result;
}
