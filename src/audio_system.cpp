#include "audio_system.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace {
constexpr double pi=3.14159265358979323846;
bool searching(SimulationStatus s) { return s==SimulationStatus::SEARCHING || s==SimulationStatus::RECALCULATING; }
std::vector<short> samples(int kind,float seconds) {
    std::vector<short> pcm(static_cast<std::size_t>(AudioConfig::sampleRate*seconds));
    std::uint32_t noise=57;double filtered=0;
    for(std::size_t i=0;i<pcm.size();++i) {
        double t=static_cast<double>(i)/AudioConfig::sampleRate,value=0;
        double envelope=std::min(1.0,t/0.012)*std::min(1.0,(seconds-t)/0.05);
        if(kind==20) {
            noise=noise*1664525u+1013904223u;
            filtered=filtered*0.985+((noise>>8)/8388608.0-1)*0.015;
            value=filtered*2+0.025*std::sin(2*pi*37*t);
        } else if(kind==21) value=0.17*std::sin(2*pi*80*t)+0.07*std::sin(2*pi*160*t)+0.025*std::sin(2*pi*320*t);
        else {
            const double frequencies[]{1100,950,740,240,1450,523.25,170};
            double frequency=frequencies[kind];
            if(kind==2) frequency=t<seconds/2?740:988;
            if(kind==3) frequency=std::fmod(t,0.18)<0.09?240:330;
            if(kind==4) frequency=t<seconds/2?1450:1050;
            if(kind==5) frequency=t<seconds/3?523.25:t<seconds*2/3?659.25:783.99;
            value=0.24*std::sin(2*pi*frequency*t)*std::exp(-t*(kind==0?55:2));
        }
        pcm[i]=static_cast<short>(std::clamp(value*envelope,-0.9,0.9)*32767);
    }
    return pcm;
}
std::vector<unsigned char> wav(const std::vector<short>& pcm) {
    std::vector<unsigned char> bytes;bytes.reserve(44+pcm.size()*2);
    auto text=[&](const char* s){for(int i=0;i<4;++i) bytes.push_back(static_cast<unsigned char>(s[i]));};
    auto number=[&](std::uint32_t n,int count){for(int i=0;i<count;++i) bytes.push_back(static_cast<unsigned char>(n>>(i*8)));};
    text("RIFF");number(static_cast<unsigned>(36+pcm.size()*2),4);text("WAVE");text("fmt ");number(16,4);
    number(1,2);number(1,2);number(AudioConfig::sampleRate,4);number(AudioConfig::sampleRate*2,4);number(2,2);number(16,2);
    text("data");number(static_cast<unsigned>(pcm.size()*2),4);
    for(short value:pcm) number(static_cast<std::uint16_t>(value),2);
    return bytes;
}
}
void AudioState::reset() { *this=AudioState{}; }
void AudioState::update(float dt,const Simulation& sim,const Sandstorm& storm) {
    cues_.fill(false);dt=std::clamp(dt,0.0f,0.1f);
    const auto status=sim.status();bool changed=status!=previous_;
    if(changed) {
        if(searching(status)) cues_[static_cast<int>(AudioCue::SCAN)]=true;
        if(status==SimulationStatus::PATH_REVEAL || (status==SimulationStatus::PATH_READY && previous_!=SimulationStatus::PATH_REVEAL)) cues_[static_cast<int>(AudioCue::PATH_FOUND)]=true;
        if(status==SimulationStatus::ROUTE_BLOCKED) cues_[static_cast<int>(AudioCue::BLOCKED)]=true;
        if(status==SimulationStatus::RECALCULATING) cues_[static_cast<int>(AudioCue::RECALCULATING)]=true;
        if(status==SimulationStatus::MISSION_COMPLETE) cues_[static_cast<int>(AudioCue::SUCCESS)]=true;
        if(status==SimulationStatus::NO_ROUTE || status==SimulationStatus::NO_PATH || status==SimulationStatus::INVALID_ENDPOINTS) cues_[static_cast<int>(AudioCue::ERROR)]=true;
    }
    if(searching(status) && sim.playing()) {
        scanTime_+=dt;
        if(scanTime_>=AudioConfig::scanInterval) { cues_[static_cast<int>(AudioCue::SCAN)]=true;scanTime_=0; }
    } else scanTime_=0;
    const auto& r=sim.rover();double velocity=0;
    if(r.available() && hadPose_ && dt>0) velocity=std::hypot(r.x()-lastX_,r.y()-lastY_)/dt;
    float target=sim.playing() && (status==SimulationStatus::DRIVING || status==SimulationStatus::ROUTE_BLOCKED)?
                 AudioConfig::engineGain*static_cast<float>(std::clamp(velocity/3.0,0.0,1.0)):0;
    engine_+=(target-engine_)*(1-std::exp(-dt*9));
    float intensity=static_cast<float>(storm.intensity());
    float windTarget=AudioConfig::windGain+storm.strength()*(0.14f+intensity*0.09f+storm.gust()*0.10f);
    wind_+=(windTarget-wind_)*(1-std::exp(-dt*2));
    float pitch=1+storm.strength()*(0.08f+intensity*0.08f+storm.gust()*0.1f);
    windPitch_+=(pitch-windPitch_)*(1-std::exp(-dt*2));
    hadPose_=r.available();lastX_=r.x();lastY_=r.y();previous_=status;
}
bool AudioSystem::init(const std::string& directory) {
    InitAudioDevice();ready_=IsAudioDeviceReady();if(!ready_) return false;
    const char* files[]{"ui_click.wav","lidar.wav","path_found.wav","route_blocked.wav","recalculating.wav","success.wav","error.wav"};
    const float durations[]{0.045f,0.15f,0.25f,0.42f,0.22f,0.65f,0.4f};
    for(std::size_t i=0;i<sounds_.size();++i) {
        std::string path=directory+"/"+files[i];
        if(FileExists(path.c_str())) sounds_[i]=LoadSound(path.c_str());
        if(!IsSoundValid(sounds_[i])) {
            auto pcm=samples(static_cast<int>(i),durations[i]);
            Wave wave{static_cast<unsigned>(pcm.size()),AudioConfig::sampleRate,16,1,pcm.data()};
            sounds_[i]=LoadSoundFromWave(wave);
        }
    }
    auto loop=[&](const char* name,int kind,std::vector<unsigned char>& data) {
        std::string path=directory+"/"+name;
        Music music{};if(FileExists(path.c_str())) music=LoadMusicStream(path.c_str());
        if(!IsMusicValid(music)) { data=wav(samples(kind,2));music=LoadMusicStreamFromMemory(".wav",data.data(),static_cast<int>(data.size())); }
        music.looping=true;return music;
    };
    wind_=loop("ambient_wind.wav",20,windData_);engine_=loop("rover_engine.wav",21,engineData_);
    reset();return true;
}
void AudioSystem::reset() {
    if(ready_) {
        for(auto sound:sounds_) if(IsSoundValid(sound)) StopSound(sound);
        if(IsMusicValid(wind_)) StopMusicStream(wind_);
        if(IsMusicValid(engine_)) StopMusicStream(engine_);
    }
    state_.reset();
    suspendFrame_=true;
}
void AudioSystem::click() {
    if(ready_ && settings.enabled && IsSoundValid(sounds_[0])) {
        SetSoundVolume(sounds_[0],settings.sfx*0.22f);PlaySound(sounds_[0]);
    }
}
bool AudioSystem::anyPlaying() const {
    if(!ready_) return false;
    if(IsMusicValid(wind_) && IsMusicStreamPlaying(wind_)) return true;
    if(IsMusicValid(engine_) && IsMusicStreamPlaying(engine_)) return true;
    for(auto sound:sounds_) if(IsSoundValid(sound) && IsSoundPlaying(sound)) return true;
    return false;
}
void AudioSystem::update(float dt,const Simulation& sim,const Sandstorm& storm) {
    if(suspendFrame_) {suspendFrame_=false;return;}
    state_.update(dt,sim,storm);if(!ready_) return;
    SetMasterVolume(settings.enabled?std::clamp(settings.master,0.0f,1.0f):0);
    if(!settings.enabled && !muted_) for(auto sound:sounds_) if(IsSoundValid(sound)) StopSound(sound);
    muted_=!settings.enabled;
    auto loop=[&](Music music,float volume,float pitch) {
        if(!IsMusicValid(music)) return;
        SetMusicVolume(music,settings.enabled?volume:0);SetMusicPitch(music,pitch);
        if(settings.enabled && volume>0.0001f && !IsMusicStreamPlaying(music)) PlayMusicStream(music);
        UpdateMusicStream(music);
    };
    loop(wind_,state_.wind()*settings.ambient,state_.windPitch());
    loop(engine_,state_.engine()*settings.sfx,1+static_cast<float>(sim.rover().speed())*0.025f);
    for(std::size_t i=1;i<sounds_.size();++i) if(settings.enabled && state_.cue(static_cast<AudioCue>(i)) && IsSoundValid(sounds_[i])) {
        SetSoundVolume(sounds_[i],settings.sfx);PlaySound(sounds_[i]);
    }
}
void AudioSystem::shutdown() {
    if(!ready_) return;
    reset();for(auto sound:sounds_) if(IsSoundValid(sound)) UnloadSound(sound);
    if(IsMusicValid(wind_)) UnloadMusicStream(wind_);
    if(IsMusicValid(engine_)) UnloadMusicStream(engine_);
    windData_.clear();engineData_.clear();CloseAudioDevice();ready_=false;
}
