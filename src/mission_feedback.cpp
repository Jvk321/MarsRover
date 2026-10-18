#include "mission_feedback.h"
#include "visual_config.h"
#include "presentation.h"
#include "ui_controls.h"
#include <algorithm>
#include <cmath>
namespace {
bool action(Rectangle r,const char* label,AudioSystem& audio) {
    bool pressed=Ui::button(r,label);if(pressed) audio.click();return pressed;
}
void centered(const char* text,float center,float y,int size,Color color) {
    DrawText(text,static_cast<int>(center-MeasureText(text,size)/2),static_cast<int>(y),size,color);
}
void slider(const char* label,float x,float y,float width,float& value,AudioSystem& audio) {
    DrawText(TextFormat("%s  %d%%",label,static_cast<int>(value*100)),static_cast<int>(x),static_cast<int>(y),17,LIGHTGRAY);
    Rectangle r{x,y+26,width,18};
    DrawRectangleRounded(r,0.3f,4,Color{29,48,61,255});
    DrawRectangleRounded({r.x,r.y,r.width*value,r.height},0.3f,4,Fade(VisualConfig::cyan,0.55f));
    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),{r.x-6,r.y-8,r.width+12,34}))
        value=std::clamp((GetMouseX()-r.x)/r.width,0.0f,1.0f);
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),r)) audio.click();
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
    float cx=v.x+v.width/2;
    if(!blocksInput()) {
        const char* text=nullptr;
        if(statusAge_<1.2f) {
            if(status_==SimulationStatus::PATH_REVEAL || status_==SimulationStatus::PATH_READY) text="PATH FOUND / REVEALING ROUTE";
            if(status_==SimulationStatus::SEARCHING) text="SEARCHING / A* ONLINE";
            if(status_==SimulationStatus::RECALCULATING) text="RECALCULATING / NEW ROUTE";
            if(status_==SimulationStatus::DRIVING) text="DRIVING / ROUTE ACTIVE";
        }
        if(text) {
            DrawRectangleRounded({cx-225,v.y+20,450,42},0.1f,4,Color{13,30,42,230});
            centered(text,cx,v.y+32,18,VisualConfig::cyan);
        }
        return FeedbackAction::NONE;
    }
    DrawRectangleRec(v,Color{4,10,18,165});
    float width=std::min(620.0f,v.width-32),height=helpOpen?570:audioOpen?430:490;
    Rectangle panel{cx-width/2,v.y+(v.height-height)/2,width,height};
    DrawRectangleRounded(panel,0.035f,6,Color{13,25,37,247});
    Color border=terminal() && !helpOpen && !audioOpen && !presentationOpen && status_!=SimulationStatus::MISSION_COMPLETE ? ORANGE:VisualConfig::cyan;
    DrawRectangleRoundedLines(panel,0.035f,6,Fade(border,0.7f));
    if(helpOpen) {
        centered("MARS ROVER A* NAVIGATION",cx,panel.y+30,25,VisualConfig::cyan);
        centered("2.5D A* pathfinding and dynamic replanning simulator",cx,panel.y+70,16,LIGHTGRAY);
        const char* lines[]{"SPACE / PLAY / PAUSE   Mission playback","N / STEP   One search/reveal step; drive 1/60 s","R / RESET   Clear mission, effects and camera",
            "MOUSE WHEEL   Zoom   /   WASD + MMB   Camera pan","AUTO CAMERA   Soft rover tracking and arrival",
            "EDIT MAP   Start, goal, rocks and eraser","DROP ROCK   Block route and replan while driving", "C / SHOW COSTS   g, h, f on explored cells (zoom in)",
            "SANDSTORM   OFF/ON and LIGHT / MEDIUM / HEAVY","AUDIO   Sound, master, ambient and SFX levels","F1   Help   /   F2   Audio   /   M   Sound on/off"};
        for(int i=0;i<11;++i) DrawText(lines[i],static_cast<int>(panel.x+24),static_cast<int>(panel.y+115+i*30),16,LIGHTGRAY);
        if(action({panel.x+24,panel.y+height-70,(width-60)/2,42},"START MISSION",audio)) {
            helpOpen=false;return FeedbackAction::START_MISSION;
        }
        if(action({cx+6,panel.y+height-70,(width-60)/2,42},"PRESENTATION MODE",audio)) {
            helpOpen=false;presentationOpen=true;
        }
    } else if(audioOpen) {
        centered("AUDIO / MISSION MIXER",cx,panel.y+28,24,VisualConfig::cyan);
        if(action({panel.x+24,panel.y+74,width-48,42},audio.settings.enabled?"SOUND: ON":"SOUND: OFF",audio)) audio.settings.enabled=!audio.settings.enabled;
        slider("MASTER VOLUME",panel.x+24,panel.y+140,width-48,audio.settings.master,audio);
        slider("MUSIC / AMBIENT",panel.x+24,panel.y+210,width-48,audio.settings.ambient,audio);
        slider("SFX",panel.x+24,panel.y+280,width-48,audio.settings.sfx,audio);
        if(!audio.ready()) centered("No audio device: simulation remains available",cx,panel.y+336,14,ORANGE);
        if(action({panel.x+24,panel.y+height-62,width-48,42},"CLOSE AUDIO",audio)) audioOpen=false;
    } else if(presentationOpen) {
        centered("PRESENTATION MODE",cx,panel.y+30,26,VisualConfig::cyan);
        centered("Fixed seeds / manual control / no automatic DROP ROCK",cx,panel.y+76,16,LIGHTGRAY);
        for(std::size_t i=0;i<presentationPresets.size();++i) {
            const auto& preset=presentationPresets[i];
            Rectangle r{panel.x+24,panel.y+118+i*74,width-48,60};
            if(action(r,"",audio)) selectedPreset=i;
            if(selectedPreset==i) DrawRectangleRoundedLines(r,0.1f,4,GOLD);
            DrawText(preset.name,static_cast<int>(r.x+10),static_cast<int>(r.y+10),18,RAYWHITE);
            DrawText(TextFormat("%d x %d  /  %d%% rocks  /  seed %u",preset.terrain.width,preset.terrain.height,
                     preset.terrain.density,preset.terrain.seed),static_cast<int>(r.x+10),static_cast<int>(r.y+36),15,LIGHTGRAY);
        }
        centered("During DRIVING: use DEMO BLOCK ROUTE to show replanning.",cx,panel.y+363,16,LIGHTGRAY);
        if(action({panel.x+24,panel.y+height-70,(width-60)/2,42},"START DEMO",audio)) {
            presentationOpen=false;return FeedbackAction::START_DEMO;
        }
        if(action({cx+6,panel.y+height-70,(width-60)/2,42},"CANCEL",audio)) presentationOpen=false;
    } else if(terminal()) {
        bool success=status_==SimulationStatus::MISSION_COMPLETE;
        float fade=std::clamp(overlayAge_/0.7f,0.0f,1.0f);
        int size=static_cast<int>(25+5*fade);
        const char* failure=status_==SimulationStatus::INVALID_ENDPOINTS?"INVALID START / GOAL":
                            status_==SimulationStatus::NO_PATH?"NO ROUTE TO GOAL":"NO ALTERNATIVE ROUTE";
        centered(success?"MISSION COMPLETE":failure,cx,panel.y+35,size,Fade(success?Color{127,227,167,255}:ORANGE,success?fade:0.7f+0.3f*fade));
        centered(success?"GOAL ACQUIRED / ROVER SECURED":"NAVIGATION HALTED / OPERATOR ACTION REQUIRED",cx,panel.y+80,15,LIGHTGRAY);
        if(success) {
            const char* labels[]{"PATH LENGTH","DISTANCE TRAVELLED","ITERATIONS","REPLANS","SEARCH TIME","MISSION TIME","MAP SIZE","SEED"};
            for(int i=0;i<8;++i) DrawText(labels[i],static_cast<int>(panel.x+28),static_cast<int>(panel.y+130+i*33),17,Color{140,167,181,255});
            float valueX=panel.x+width*0.62f;
            DrawText(TextFormat("%d segments",static_cast<int>(sim.pathLength())),static_cast<int>(valueX),static_cast<int>(panel.y+130),17,RAYWHITE);
            DrawText(TextFormat("%.2f tiles",travelled_),static_cast<int>(valueX),static_cast<int>(panel.y+163),17,RAYWHITE);
            DrawText(TextFormat("%d",static_cast<int>(iterations_)),static_cast<int>(valueX),static_cast<int>(panel.y+196),17,RAYWHITE);
            DrawText(TextFormat("%d",static_cast<int>(sim.replans())),static_cast<int>(valueX),static_cast<int>(panel.y+229),17,RAYWHITE);
            DrawText(TextFormat("%.1f s",searchTime_),static_cast<int>(valueX),static_cast<int>(panel.y+262),17,RAYWHITE);
            DrawText(TextFormat("%.1f s",missionTime_),static_cast<int>(valueX),static_cast<int>(panel.y+295),17,RAYWHITE);
            DrawText(TextFormat("%d x %d",terrain.grid.width(),terrain.grid.height()),static_cast<int>(valueX),static_cast<int>(panel.y+328),17,RAYWHITE);
            DrawText(TextFormat("%u",terrain.config.seed),static_cast<int>(valueX),static_cast<int>(panel.y+361),17,RAYWHITE);
        } else {
            centered("Rover stopped safely. Edit the terrain or start again.",cx,panel.y+160,17,LIGHTGRAY);
            centered("No automatic retries. Your map remains available.",cx,panel.y+200,17,LIGHTGRAY);
        }
        float buttonWidth=(width-64)/3;
        if(action({panel.x+16,panel.y+height-70,buttonWidth,42},success?"NEW MISSION":"NEW MAP",audio)) return FeedbackAction::NEW_MISSION;
        if(action({panel.x+32+buttonWidth,panel.y+height-70,buttonWidth,42},success?"REPLAY":"RESET",audio)) return success?FeedbackAction::REPLAY:FeedbackAction::RESET;
        if(action({panel.x+48+buttonWidth*2,panel.y+height-70,buttonWidth,42},"EDIT MAP",audio)) return FeedbackAction::EDIT;
    }
    return FeedbackAction::NONE;
}
