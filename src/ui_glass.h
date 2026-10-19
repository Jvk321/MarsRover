#pragma once
#include "raylib.h"
#include <cstdint>

namespace Ui {
enum class Quality { LOW, HIGH };
enum class Pass { BOTH, INPUT, DRAW };
enum class Icon { NONE, PLAY, PAUSE, STEP, RESET, CAMERA, MAP, STORM, AUDIO, EDIT, ROCK, HELP, CHEVRON };
namespace Theme {
constexpr Color text{233,240,247,255},muted{152,174,191,255},cyan{117,211,228,255};
constexpr Color navy{14,30,47,255},gold{239,199,113,255},orange{232,148,103,255};
constexpr float radius=22,controlRadius=13,blurRadius=2.6f,refraction=1.25f;
constexpr float transition=0.22f;
}
struct Layout {
    float scale=1;
    Rectangle canvas{},world{},dock{},status{},navigation{},search{},mission{},settings{},tabs{},bar{},minimap{},brand{},topActions{},editor{};
};
Layout layout(float width,float height);
void init();
bool glassReady();
void shutdown();
void beginFrame(float dt,int width,int height);
void beginWorld();
void endWorld();
void drawWorld();
void pass(Pass value);
bool drawing();
bool input();
float delta();
void opacity(float value);
float opacity();
void quality(Quality value);
Quality quality();
float measure(const char* value,float size);
void text(const char* value,Vector2 position,float size,Color color=Theme::text);
void centered(const char* value,Rectangle rect,float size,Color color=Theme::text);
void fitText(const char* value,Rectangle rect,float size,Color color=Theme::text);
void panel(Rectangle rect,float radius=Theme::radius,Color accent=Theme::cyan,float active=0,float opacity=1);
void icon(Icon kind,Vector2 center,float size,Color color);
float animate(std::uint32_t id,float target,float initial=0);
std::uint32_t id(const char* label,Rectangle rect);
}
