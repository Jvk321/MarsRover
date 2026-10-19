#pragma once
#include "mission_feedback.h"
#include "world_camera.h"
#include "visual_effects.h"
#include "ui_controls.h"
#include <array>

namespace HudEvent {
enum : unsigned {PLAY=1,PAUSE=2,STEP=4,RESET=8,NEW_MAP=16,GENERATE=32,EDIT=64,CLEAR=128,
                 PRESENTATION=256,HELP=512,DROP_ROCK=1024,AUTO_CAMERA=2048,FIT_CAMERA=4096,STORM=8192};
}
struct HudActions {unsigned flags=0;int stormLevel=-1;float zoomWheel=0;bool has(unsigned flag) const {return (flags&flag)!=0;}};
struct HudSettings {float searchSpeed=8,driveSpeed=1.5f;bool showCosts=false;EditTool tool=EditTool::SET_START;};
enum class HudTab { MAP, CAMERA, ENVIRONMENT, AUDIO };
class MissionHud {
public:
    HudSettings settings;
    void selectTab(HudTab tab) {tab_=static_cast<int>(tab);}
    HudTab selectedTab() const {return static_cast<HudTab>(tab_);}
    HudActions input(const Ui::Layout& layout,const Simulation& sim,const Terrain& terrain,const WorldCamera& camera,
                     const VisualEffects& effects,AudioSystem& audio,bool blocked,bool presentation);
    void draw(const Ui::Layout& layout,const Simulation& sim,const Terrain& terrain,const WorldCamera& camera,
              const VisualEffects& effects,AudioSystem& audio,const MissionFeedback& feedback,bool blocked,bool presentation);
    static bool overControls(const Ui::Layout& layout,Vector2 point,bool editing);
private:
    int tab_=0;
    std::array<float,4> scroll_{};
    bool zoomDragging_=false;
    float lastZoom_=1,statusAge_=1,appearance_=0;
    SimulationStatus previous_=SimulationStatus::READY,oldStatus_=SimulationStatus::READY;
    std::array<double,6> lastValues_{};
    std::array<float,6> pulses_{};
    HudActions controls(const Ui::Layout& layout,const Simulation& sim,const Terrain& terrain,const WorldCamera& camera,
                        const VisualEffects& effects,AudioSystem& audio,bool blocked,bool presentation);
};
