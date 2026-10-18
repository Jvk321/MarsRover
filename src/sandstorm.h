#pragma once
#include "raylib.h"
#include <array>
#include <cstdint>

enum class StormIntensity { LIGHT, MEDIUM, HEAVY };
namespace StormConfig {
struct Profile { int nearCount,midCount,groundCount,nearGust,midGust,groundGust; float haze,speed; };
inline constexpr std::array<Profile,3> profiles{{
    {16,90,10,28,145,16,0.20f,0.65f},
    {40,220,22,66,330,34,0.43f,1.0f},
    {72,390,36,112,512,56,0.70f,1.35f}
}};
constexpr Vector2 windDirection{1.0f,0.32f}; // screen/projected-world direction
constexpr float transitionSeconds=2.5f;
constexpr float gustInterval=5.5f,gustDuration=1.15f,gustSpeedBoost=1.3f;
constexpr float nearSpeed=1.15f,midSpeed=0.15f,groundSpeed=0.008f;
constexpr float clearRadius=105,fadeDistance=560;
constexpr Color hazeColor{128,117,111,255}; // neutral suspended mineral dust
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
    void drawGround(int width,int height,Rectangle worldBounds) const;
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
