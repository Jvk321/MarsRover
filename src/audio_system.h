#pragma once
#include "simulation.h"
#include "sandstorm.h"
#include "raylib.h"
#include "sonar.h"
#include "drive_audio_synth.h"
#include <array>
#include <string>
#include <vector>
namespace AudioConfig {
constexpr float master=0.65f,ambient=0.32f,sfx=0.55f;
constexpr float engineGain=DriveAudioConfig::engineVolume,windGain=DriveAudioConfig::windVolume;
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
    float enginePitch() const { return enginePitch_; }
    float sonarAngle() const { return sonar_.angle(); }
private:
    std::array<bool,static_cast<int>(AudioCue::COUNT)> cues_{};
    SimulationStatus previous_=SimulationStatus::READY;
    double lastX_=0,lastY_=0;
    bool hadPose_=false;
    float engine_=0,wind_=0,windPitch_=1,enginePitch_=1;
    SonarClock sonar_;
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
    unsigned engineStarts() const { return engineStarts_; }
    unsigned windStarts() const { return windStarts_; }
    const DriveAudioSynth& synth() const { return synth_; }
private:
    AudioState state_;
    bool ready_=false,muted_=false,suspendFrame_=false;
    std::array<Sound,static_cast<int>(AudioCue::COUNT)> sounds_{};
    AudioStream wind_{},engine_{};
    DriveAudioSynth synth_;
    std::array<float,DriveAudioConfig::blockFrames> windBuffer_{},engineBuffer_{};
    unsigned engineStarts_=0,windStarts_=0;
};
