#pragma once
#include "terrain.h"
#include "visual_effects.h"
#include "raylib.h"
#include <array>

namespace WorldArt {
enum class PropKind { HABITAT, DOME, SOLAR, ANTENNA, CRATES, LAMP };
struct Prop { PropKind kind;float x,y;std::uint32_t variant; };
using Props=std::array<Prop,32>;
// Decorations stay outside the logical grid and never become obstacles or affect generation.
std::size_t props(const Grid& grid,std::uint32_t seed,Props& out);
void baseGround(const Grid& grid,Rectangle visible);
void propShadow(const Prop& prop);
void prop(const Prop& prop,bool detail,float haze,float time);
void background(Rectangle viewport,const Camera2D& camera,const VisualEffects& effects);
void beacon(Cell goal,float time,bool complete);
void startPad(Cell start,float time);
}
