#pragma once
#include <cstdint>
#include <cstddef>
namespace DriveAudioConfig {
constexpr float windVolume=0.075f,windGustVolume=0.035f;
constexpr float stormWindLight=0.022f,stormWindMedium=0.050f,stormWindHeavy=0.095f;
constexpr float engineVolume=0.28f,servoVolume=0.024f;
constexpr float enginePitchMin=0.94f,enginePitchMax=1.10f;
constexpr float windRumbleHz=42,windAirHz=165;
constexpr unsigned sampleRate=22050,blockFrames=2048;
}
// Continuous state across all audio blocks. No short PCM loops or per-block envelopes.
class DriveAudioSynth {
public:
    void reset();
    void wind(float* buffer,std::size_t frames);
    void engine(float* buffer,std::size_t frames);
    std::uint64_t windFrames() const { return windFrames_; }
    std::uint64_t engineFrames() const { return engineFrames_; }
private:
    std::uint32_t random_=0x5e731ab;
    double rumble_=0,air_=0,air2_=0,slow_=0,slow2_=0,phase_=0;
    std::uint64_t windFrames_=0,engineFrames_=0;
    float noise();
};
