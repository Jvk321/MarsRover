#include "ui_controls.h"
#include <algorithm>
#include <cmath>
namespace {std::uint32_t dragging=0;}
bool Ui::button(Rectangle r,const char* label,bool enabled,bool blocked,int size,Icon glyph) {
    bool hover=enabled && !blocked && CheckCollisionPointRec(GetMousePosition(),r);
    if(drawing()) {
        float highlight=animate(id(label,r),hover?1.0f:0.0f);
        Rectangle visual=r;visual.y-=highlight*1.7f;
        const bool pressed=hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        panel(visual,std::min(16.0f,r.height*0.28f),Theme::cyan,pressed?0.08f:highlight*0.55f,enabled?1.0f:0.58f);
        Color color=enabled?Theme::text:Color{100,123,141,210};
        float inset=glyph==Icon::NONE?12:40;
        if(glyph!=Icon::NONE) icon(glyph,{visual.x+22,visual.y+visual.height/2},18,color);
        fitText(label,{visual.x+inset,visual.y,visual.width-inset-10,visual.height},size,color);
    }
    return input() && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
bool Ui::toggle(Rectangle r,const char* label,bool& value,bool blocked,Icon glyph) {
    bool hover=!blocked && CheckCollisionPointRec(GetMousePosition(),r);
    bool changed=input() && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if(changed) value=!value;
    if(drawing()) {
        Color color=blocked?Color{106,126,142,255}:Theme::text;
        float font=std::clamp(r.height*0.43f,16.0f,23.0f);
        if(glyph!=Icon::NONE) icon(glyph,{r.x+12,r.y+r.height/2},20,color);
        fitText(label,{r.x+(glyph==Icon::NONE?0:32),r.y,r.width-98,r.height},font,color);
        Rectangle capsule{r.x+r.width-58,r.y+(r.height-28)/2,58,28};
        float t=animate(id(label,r),value?1.0f:0.0f,value?1.0f:0.0f);
        DrawRectangleRounded(capsule,1,12,Color{48,70,87,200});
        DrawRectangleRounded(capsule,1,12,Fade(Theme::cyan,0.22f*t));
        DrawRectangleRoundedLines(capsule,1,12,Fade(Theme::text,hover?0.30f:0.12f));
        Vector2 thumb{capsule.x+14+t*(capsule.width-28),capsule.y+14};
        DrawCircleV(thumb,10.5f,Color{203,218,225,255});
        DrawCircleV({thumb.x-1,thumb.y-2},7.5f,Fade(WHITE,0.18f));
    }
    return changed;
}
bool Ui::slider(Rectangle r,const char* label,float& value,float low,float high,bool blocked,const char* format,float displayScale,bool integer) {
    const auto key=id(label,r);float original=value;
    const Rectangle hit{r.x-5,r.y+23,r.width+10,r.height-18};
    if(input()) {
        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) dragging=0;
        if(!blocked && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),hit)) dragging=key;
        if(!blocked && dragging==key && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            value=low+(high-low)*std::clamp((GetMouseX()-r.x)/r.width,0.0f,1.0f);
            if(integer) value=std::round(value);
        }
    }
    if(drawing()) {
        const float font=std::clamp(r.height*0.31f,15.0f,21.0f);
        text(label,{r.x,r.y},font,Theme::muted);
        const char* number=TextFormat(format,value*displayScale);
        text(number,{r.x+r.width-measure(number,font),r.y},font,Theme::text);
        float y=r.y+r.height-12;
        DrawLineEx({r.x,y},{r.x+r.width,y},4,Color{72,95,114,115});
        float fraction=std::clamp((value-low)/(high-low),0.0f,1.0f);
        DrawLineEx({r.x,y},{r.x+r.width*fraction,y},3,Fade(Theme::cyan,0.65f));
        Vector2 thumb{r.x+r.width*fraction,y};
        float glow=animate(key,dragging==key?1.0f:0.0f);
        DrawCircleV(thumb,11,Fade(Theme::cyan,glow*0.12f));
        DrawCircleV(thumb,6.5f,Color{211,225,231,255});
        DrawCircleV({thumb.x-0.5f,thumb.y-1},3,Fade(WHITE,0.6f));
    }
    return value!=original;
}
