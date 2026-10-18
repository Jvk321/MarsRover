#pragma once
#include "simulation.h"
#include "sandstorm.h"
#include "raylib.h"
#include <array>
#include <string>
#include <vector>
namespace AudioConfig {
constexpr float master=0.65f,ambient=0.32f,sfx=0.55f;
constexpr float engineGain=0.24f,windGain=0.18f,scanInterval=3;
constexpr unsigned sampleRate=22050;
}
enum class AudioCue { CLICK, SCAN, PATH_FOUND, BLOCKED, RECALCULATING, SUCCESS, ERROR, COUNT };
struct AudioSettings { bool enabled=true;float master=AudioConfig::master,ambient=AudioConfig::ambient,sfx=AudioConfig::sfx; };
// Pure observer: testable without an audio device. No writes to simulation.
class AudioState {
public:
    void reset();
    void update(float dt,const Simulation& sim,const Sandstorm& storm);
    bool cue(AudioCue cue) const { return cues_[static_cast<int>(cue)]; }
    float engine() const { return engine_; }
    float wind() const { return wind_; }
    float windPitch() const { return windPitch_; }
private:
    std::array<bool,static_cast<int>(AudioCue::COUNT)> cues_{};
    SimulationStatus previous_=SimulationStatus::READY;
    double lastX_=0,lastY_=0;
    bool hadPose_=false;
    float scanTime_=0,engine_=0,wind_=0,windPitch_=1;
};
class AudioSystem {
public:
    AudioSystem() = default;
    ~AudioSystem() { shutdown(); }
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    AudioSettings settings;
    bool init(const std::string& directory);
    void shutdown();
    void reset();
    void click();
    void update(float dt,const Simulation& sim,const Sandstorm& storm);
    bool ready() const { return ready_; }
    bool anyPlaying() const;
    const AudioState& state() const { return state_; }
private:
    AudioState state_;
    bool ready_=false,muted_=false,suspendFrame_=false;
    std::array<Sound,static_cast<int>(AudioCue::COUNT)> sounds_{};
    Music wind_{},engine_{};
    // Music decoders reference their memory; keep fallback WAV data until shutdown.
    std::vector<unsigned char> windData_,engineData_;
};
