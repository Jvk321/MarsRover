#pragma once
#include "terrain.h"
#include "raylib.h"

namespace TerrainArt {
// Shared vertex heights make neighboring obstacle tiles one continuous formation.
bool cliff(const Grid& grid,Cell c);
float cliffHeight(int x,int y,std::uint32_t seed);
void ground(Cell c,std::uint32_t seed,bool detail,float haze);
void gridLines(const Grid& grid,Rectangle visible,bool active,float zoom);
void obstacle(const Grid& grid,Cell c,std::uint32_t seed,bool detail,float haze);
void shadow(const Grid& grid,Cell c,std::uint32_t seed,float haze);
void plateau(const Grid& grid,std::uint32_t seed,Rectangle visible,bool detail);
}
