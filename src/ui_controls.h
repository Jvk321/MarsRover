#pragma once
#include "visual_config.h"

namespace Ui {
// Drawing/input only. Callers own the action and optional click sound.
inline bool button(Rectangle rect,const char* label,bool enabled=true,bool blocked=false,int fontSize=17) {
    bool hover=enabled && !blocked && CheckCollisionPointRec(GetMousePosition(),rect);
    DrawRectangleRounded(rect,0.12f,4,!enabled?Color{23,31,40,220}:hover?Color{35,71,83,245}:Color{24,43,56,235});
    DrawRectangleRoundedLines(rect,0.12f,4,Fade(VisualConfig::cyan,enabled?(hover?0.8f:0.3f):0.12f));
    while(fontSize>10 && MeasureText(label,fontSize)>rect.width-20) --fontSize;
    DrawText(label,static_cast<int>(rect.x+10),static_cast<int>(rect.y+(rect.height-fontSize)/2),fontSize,
             enabled?Color{213,230,233,255}:Color{77,99,110,255});
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
}
