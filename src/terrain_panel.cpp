#include "terrain_panel.h"
#include "raylib.h"
#include <limits>
namespace {
bool action(Rectangle rect,const char* label) {
    bool hover=CheckCollisionPointRec(GetMousePosition(),rect);
    DrawRectangleRec(rect,hover ? Color{83,84,106,255}:Color{55,57,74,255});
    DrawText(label,static_cast<int>(rect.x+8),static_cast<int>(rect.y+10),18,RAYWHITE);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
}
TerrainPanel::TerrainPanel(TerrainConfig config) : fields_{std::to_string(config.width),std::to_string(config.height),
    std::to_string(config.density),std::to_string(config.seed)} {}
std::optional<TerrainConfig> TerrainPanel::draw() {
    if (!open) return std::nullopt;
    DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),{0,0,0,180});
    const float left=(GetScreenWidth()-580)/2.0f,top=150;
    DrawRectangle(static_cast<int>(left),static_cast<int>(top),580,580,{32,33,43,255});
    DrawText("NEW MARS TERRAIN",static_cast<int>(left+24),174,26,RAYWHITE);
    const char* names[]{"LAB 20x20","MEDIUM 30x30","EXPEDITION 50x50","LARGE 80x60"};
    const int widths[]{20,30,50,80},heights[]{20,30,50,60};
    for (int i=0;i<4;++i) if (action({left+24+(i%2)*270.0f,220+(i/2)*46.0f,258,38},names[i])) {
        fields_[0]=std::to_string(widths[i]); fields_[1]=std::to_string(heights[i]); error_.clear();
    }
    const char* labels[]{"WIDTH (2-512)","HEIGHT (2-512)","OBSTACLE DENSITY % (0-95)","SEED (uint32)"};
    for (int i=0;i<4;++i) {
        float y=330+i*58.0f;
        DrawText(labels[i],static_cast<int>(left+24),static_cast<int>(y+10),17,LIGHTGRAY);
        Rectangle box{left+310,y,240,38};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),box)) focus_=i;
        DrawRectangleRec(box,focus_==i ? Color{75,78,101,255}:Color{45,47,60,255});
        DrawText((fields_[i]+(focus_==i ? "_":"")).c_str(),static_cast<int>(box.x+10),static_cast<int>(y+10),19,RAYWHITE);
    }
    if (IsKeyPressed(KEY_TAB)) focus_=(focus_+1)%4;
    for (int ch=GetCharPressed();ch;ch=GetCharPressed())
        if (focus_>=0 && ch>='0' && ch<='9' && fields_[focus_].size()<10) fields_[focus_]+=static_cast<char>(ch);
    if (focus_>=0 && (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !fields_[focus_].empty())
        fields_[focus_].pop_back();
    if (action({left+24,586,340,42},"GENERATE TERRAIN") || IsKeyPressed(KEY_ENTER)) {
        try {
            const auto w=std::stoull(fields_[0]),h=std::stoull(fields_[1]),d=std::stoull(fields_[2]),seed=std::stoull(fields_[3]);
            if (w<2 || w>512 || h<2 || h>512 || d>95 || seed>std::numeric_limits<std::uint32_t>::max())
                error_="Use dimensions 2-512, density 0-95, uint32 seed.";
            else { open=false; focus_=-1; error_.clear(); return TerrainConfig{static_cast<int>(w),static_cast<int>(h),static_cast<int>(d),static_cast<std::uint32_t>(seed)}; }
        } catch (...) { error_="Enter a valid number in every field."; }
    }
    if (action({left+380,586,170,42},"CANCEL")) open=false;
    DrawText(error_.c_str(),static_cast<int>(left+24),646,17,ORANGE);
    DrawText("Click field, Backspace to edit. Tab: next. Enter: generate.",static_cast<int>(left+24),680,17,LIGHTGRAY);
    return std::nullopt;
}
