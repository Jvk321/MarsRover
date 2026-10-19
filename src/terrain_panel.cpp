#include "terrain_panel.h"
#include "ui_controls.h"
#include <limits>
#include <algorithm>
#include <cmath>
namespace {
bool action(Rectangle r,const char* label) {return Ui::button(r,label,true,false,static_cast<int>(r.height*0.4f));}
}
TerrainPanel::TerrainPanel(TerrainConfig config) {setConfig(config);}
void TerrainPanel::setConfig(TerrainConfig config) {
    fields_={std::to_string(config.width),std::to_string(config.height),std::to_string(config.density),std::to_string(config.seed)};
    focus_=-1;error_.clear();
}
std::optional<TerrainConfig> TerrainPanel::draw() {
    if(!open) return std::nullopt;
    const float s=Ui::layout(GetScreenWidth(),GetScreenHeight()).scale;
    const float width=760*s,height=660*s;
    Rectangle panel{(GetScreenWidth()-width)/2,(GetScreenHeight()-height)/2,width,height};
    DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(Color{7,14,25,255},0.18f));
    Ui::panel(panel,26*s,Ui::Theme::cyan,0.08f);
    const float x=panel.x+28*s,w=width-56*s;
    Ui::text("MISSION SETUP",{x,panel.y+26*s},13*s,Ui::Theme::muted);
    Ui::text("NEW MARS TERRAIN",{x,panel.y+54*s},32*s);
    Ui::text("Deterministic formations / guaranteed initial route",{x,panel.y+100*s},17*s,Ui::Theme::muted);
    const char* names[]{"LAB 20x20","MEDIUM 30x30","EXPEDITION 50x50","LARGE 80x60"};
    const int widths[]{20,30,50,80},heights[]{20,30,50,60};
    for(int i=0;i<4;++i) if(action({x+(i%2)*(w+14*s)/2,panel.y+150*s+(i/2)*58*s,(w-14*s)/2,46*s},names[i])) {
        fields_[0]=std::to_string(widths[i]);fields_[1]=std::to_string(heights[i]);error_.clear();
    }
    const char* labels[]{"WIDTH  /  2 - 512","HEIGHT  /  2 - 512","OBSTACLE DENSITY  /  0 - 95%","SEED  /  uint32"};
    for(int i=0;i<4;++i) {
        const float y=panel.y+287*s+i*58*s;
        Ui::text(labels[i],{x,y+13*s},17*s,Ui::Theme::muted);
        Rectangle field{x+w*0.58f,y,w*0.42f,44*s};
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),field)) focus_=i;
        Ui::panel(field,13*s,Ui::Theme::cyan,focus_==i?0.42f:0.02f);
        Ui::text(fields_[i].c_str(),{field.x+14*s,field.y+10*s},22*s);
        if(focus_==i && std::fmod(GetTime(),1.0)<0.55) {
            const float cursor=field.x+14*s+Ui::measure(fields_[i].c_str(),22*s)+3*s;
            DrawLineEx({cursor,field.y+11*s},{cursor,field.y+33*s},1.3f*s,Ui::Theme::cyan);
        }
    }
    if (IsKeyPressed(KEY_TAB)) focus_=(focus_+1)%4;
    for (int ch=GetCharPressed();ch;ch=GetCharPressed())
        if (focus_>=0 && ch>='0' && ch<='9' && fields_[focus_].size()<10) fields_[focus_]+=static_cast<char>(ch);
    if (focus_>=0 && (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !fields_[focus_].empty())
        fields_[focus_].pop_back();
    if(action({x,panel.y+height-110*s,w*0.63f,50*s},"GENERATE TERRAIN") || IsKeyPressed(KEY_ENTER)) {
        try {
            const auto w=std::stoull(fields_[0]),h=std::stoull(fields_[1]),d=std::stoull(fields_[2]),seed=std::stoull(fields_[3]);
            if (w<2 || w>512 || h<2 || h>512 || d>95 || seed>std::numeric_limits<std::uint32_t>::max())
                error_="Use dimensions 2-512, density 0-95, uint32 seed.";
            else { open=false; focus_=-1; error_.clear(); return TerrainConfig{static_cast<int>(w),static_cast<int>(h),static_cast<int>(d),static_cast<std::uint32_t>(seed)}; }
        } catch (...) { error_="Enter a valid number in every field."; }
    }
    if(action({x+w*0.67f,panel.y+height-110*s,w*0.33f,50*s},"CANCEL")) open=false;
    Ui::fitText(error_.c_str(),{x,panel.y+height-51*s,w,24*s},16*s,Ui::Theme::orange);
    if(error_.empty()) Ui::fitText("Click a field to edit / TAB next / ENTER generate",{x,panel.y+height-47*s,w,24*s},15*s,Ui::Theme::muted);
    return std::nullopt;
}
