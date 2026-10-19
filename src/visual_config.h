#pragma once
#include "raylib.h"
#include "rover_geometry.h"
#include <cstddef>
// Shared art/effect tuning. Geometry lives in scene_renderer/rover_renderer.
// No simulation parameters.
namespace VisualConfig {
constexpr Color skyTop{23,20,35,255}, skyBottom{163,69,43,255};
constexpr Color cyan{82,217,255,255}, gold{255,192,51,255}, startGreen{73,255,167,255};
constexpr Color regolith{184,79,39,255}, rockTop{211,108,53,255}, rockShade{85,36,30,255};
constexpr Color sunRim{255,175,91,255}, pathCore{255,246,173,255};
constexpr Vector2 shadowDirection{-0.85f,0.55f};
constexpr int terrainDetailCount=7;
constexpr float gridAlpha=0.018f, activeGridAlpha=0.13f, openAlpha=0.12f, closedAlpha=0.11f;
constexpr float rockHeight=30, cliffHeight=36, cliffVariation=22, plateauDepth=88;
constexpr float shadowAlpha=0.27f, shadowLength=1.35f, rimAlpha=0.72f;
constexpr float openGlow=0.32f,pathGlow=0.20f,beaconGlow=0.32f;
constexpr float pathFlowSpeed=0.65f;
constexpr std::size_t maxDust=384, maxTracks=8000;
constexpr int ambientCount=72;
constexpr float TRACK_SAMPLE_DISTANCE=0.055f;
constexpr float trackWidth=static_cast<float>(RoverGeometry::wheelWidth), trackOpacity=0.30f;
constexpr float dustLife=1.8f, dustRate=32, turnDustRate=48;
constexpr float windX=18, windY=3;
constexpr float farParallax=0.018f, nearParallax=0.055f;
constexpr float beaconSignalSpeed=0.35f;
constexpr float detailZoomThreshold=0.55f;
}
