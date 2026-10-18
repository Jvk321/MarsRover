#pragma once
#include "terrain.h"
#include <array>

// Fixed inputs, not stored maps. The normal generator remains unchanged.
struct PresentationPreset {
    const char* name;
    TerrainConfig terrain;
    double searchSpeed;
    double drivingSpeed;
};
inline constexpr std::array<PresentationPreset,3> presentationPresets{{
    {"LAB DEMO", {20,20,20,42}, 30,2.5},
    {"MARS EXPEDITION", {35,35,20,2027}, 60,3.0},
    {"LARGE TERRAIN", {60,50,18,47}, 60,3.5}
}};
