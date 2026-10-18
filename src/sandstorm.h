#pragma once
#include "raylib.h"
#include <array>
#include <cstdint>

enum class StormIntensity { LIGHT, MEDIUM, HEAVY };
namespace StormConfig {
struct Profile { int nearCount,midCount,groundCount,nearGust,midGust,groundGust; float haze,speed; };
inline constexpr std::array<Profile,3> profiles{{
    {18,90,12,28,145,16,0.16f,0.65f},
    {48,220,26,66,330,34,0.34f,1.0f},
    {80,390,40,112,512,56,0.72f,1.35f}
}};
constexpr Vector2 windDirection{1.0f,0.32f}; // screen/projected-world direction
constexpr float transitionSeconds=2.5f;
constexpr float gustInterval=5.5f,gustDuration=1.35f,gustSpeedBoost=2.0f;
constexpr float nearSpeed=1.65f,midSpeed=0.15f,groundSpeed=0.014f;
constexpr float foregroundLength=24,foregroundSpeedLength=26,foregroundGustLength=42;
constexpr float foregroundOpacity=0.32f,foregroundWidth=1.05f;
constexpr float groundOpacity=0.20f,groundLength=34,groundGustLength=48;
constexpr float localGroundRadius=300; // screen pixels around rover; half of ground pool
constexpr float clearRadius=155,fadeDistance=620;
constexpr float hazePatchScale=0.006f,hazePatchDrift=0.17f,hazePatchMinimum=0.38f;
constexpr float farHazeBoost=1.25f,nearHazeFactor=0.16f;
constexpr float roverLightOpacity=0.28f,beaconLightOpacity=0.27f;
constexpr Color hazeColor{123,124,123,255}; // neutral grey mineral dust, no red screen tint
constexpr std::size_t nearCapacity=112,midCapacity=512,groundCapacity=56;
}
struct StormParticle { Vector2 position; float size,alpha,speed,phase; };
class Sandstorm {
public:
    void reset(std::uint32_t seed);
    void setEnabled(bool value) { enabled_=value; }
    bool enabled() const { return enabled_; }
    void setIntensity(StormIntensity value) { intensity_=value; }
    StormIntensity intensity() const { return intensity_; }
    void update(float dt);
    float strength() const { return strength_; }
    float gust() const { return gust_; }
    float time() const { return time_; }
    int nearCount() const;
    int midCount() const;
    int groundCount() const;
    float haze(Vector2 world,Vector2 focus,float zoom) const;
    float backgroundHaze(int layer) const;
    void drawGround(int width,int height,Rectangle worldBounds,Vector2 focus,float zoom) const;
    void drawAir(Rectangle viewport) const;
private:
    std::array<StormParticle,StormConfig::nearCapacity> near_{};
    std::array<StormParticle,StormConfig::midCapacity> mid_{};
    std::array<StormParticle,StormConfig::groundCapacity> ground_{};
    bool enabled_=false;
    StormIntensity intensity_=StormIntensity::MEDIUM;
    float strength_=0,ramp_=0,gust_=0,time_=0,level_=1;
    StormConfig::Profile profile() const;
};
