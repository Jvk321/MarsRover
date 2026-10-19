#pragma once
#include "ui_glass.h"

namespace Ui {
bool button(Rectangle rect,const char* label,bool enabled=true,bool blocked=false,int fontSize=17,Icon glyph=Icon::NONE);
bool toggle(Rectangle rect,const char* label,bool& value,bool blocked=false,Icon glyph=Icon::NONE);
bool slider(Rectangle rect,const char* label,float& value,float minimum,float maximum,bool blocked=false,
            const char* format="%.2f",float displayScale=1,bool integer=false);
}
