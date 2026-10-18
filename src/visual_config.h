#pragma once
#include "raylib.h"
#include <cstddef>
// Shared art/effect tuning. Geometry lives in scene_renderer/rover_renderer.
// No simulation parameters.
namespace VisualConfig {
constexpr Color skyTop{12,17,28,255}, skyBottom{77,43,42,255};
constexpr Color cyan{100,218,229,255}, gold{255,210,92,255};
constexpr int terrainDetailCount=4;
constexpr float gridAlpha=0.025f, openAlpha=0.18f, closedAlpha=0.18f;
constexpr float rockHeight=22, shadowAlpha=0.22f;
constexpr std::size_t maxDust=384, maxTracks=16000;
constexpr int ambientCount=72;
constexpr float trackSpacing=0.10f, trackHalfWidth=0.29f;
constexpr float dustLife=1.8f, dustRate=32, turnDustRate=48;
constexpr float windX=18, windY=3;
constexpr float scanInterval=3.0f, scanLife=2.4f, scanRadius=145;
constexpr float farParallax=0.018f, nearParallax=0.055f;
constexpr float antennaSpeed=0.7f, beaconSignalSpeed=0.35f;
constexpr float detailZoomThreshold=0.45f;
}
