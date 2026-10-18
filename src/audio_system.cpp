#include "audio_system.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace {
constexpr double pi=3.14159265358979323846;

std::vector<short> samples(int kind,float seconds) {
    std::vector<short> pcm(static_cast<std::size_t>(AudioConfig::sampleRate*seconds));
    for(std::size_t i=0;i<pcm.size();++i) {
        double t=static_cast<double>(i)/AudioConfig::sampleRate,value=0;
        double envelope=std::min(1.0,t/0.012)*std::min(1.0,(seconds-t)/0.05);
        if(kind==1) {
            for(int echo=0;echo<3;++echo) {
                double age=t-echo*SonarConfig::echoDelay;
                if(age>=0) value+=0.26*std::pow(SonarConfig::echoGain,echo)*std::min(1.0,age/0.004)*
                    std::exp(-age/SonarConfig::pingDecay)*std::sin(2*pi*SonarConfig::pingFrequency*age);
            }
        } else {
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
}
void AudioState::reset() { *this=AudioState{}; }
void AudioState::update(float dt,const Simulation& sim,const Sandstorm& storm) {
    cues_.fill(false);dt=std::clamp(dt,0.0f,0.1f);
    const auto status=sim.status();bool changed=status!=previous_;
    if(changed) {

        if(status==SimulationStatus::PATH_REVEAL || (status==SimulationStatus::PATH_READY && previous_!=SimulationStatus::PATH_REVEAL)) cues_[static_cast<int>(AudioCue::PATH_FOUND)]=true;
        if(status==SimulationStatus::ROUTE_BLOCKED) cues_[static_cast<int>(AudioCue::BLOCKED)]=true;
        if(status==SimulationStatus::RECALCULATING) cues_[static_cast<int>(AudioCue::RECALCULATING)]=true;
        if(status==SimulationStatus::MISSION_COMPLETE) cues_[static_cast<int>(AudioCue::SUCCESS)]=true;
        if(status==SimulationStatus::NO_ROUTE || status==SimulationStatus::NO_PATH || status==SimulationStatus::INVALID_ENDPOINTS) cues_[static_cast<int>(AudioCue::ERROR)]=true;
    }
    const float period=SonarConfig::period(status);
    if(period>0) cues_[static_cast<int>(AudioCue::SCAN)]=sonar_.update(dt,period,sim.playing());
    else sonar_.inactive();
    const auto& r=sim.rover();double velocity=0;
    if(r.available() && hadPose_ && dt>0) velocity=std::hypot(r.x()-lastX_,r.y()-lastY_)/dt;
    float normalizedSpeed=static_cast<float>(std::clamp(velocity/5.0,0.0,1.0));
    bool active=sim.playing() && (status==SimulationStatus::DRIVING || status==SimulationStatus::ROUTE_BLOCKED);
    float target=active?AudioConfig::engineGain*(normalizedSpeed>0.001f?0.35f+0.65f*normalizedSpeed:0):0;
    if(active && r.turning()) target=DriveAudioConfig::servoVolume;
    engine_+=(target-engine_)*(1-std::exp(-dt*6));
    float pitch=DriveAudioConfig::enginePitchMin+(DriveAudioConfig::enginePitchMax-DriveAudioConfig::enginePitchMin)*normalizedSpeed;
    enginePitch_+=(pitch-enginePitch_)*(1-std::exp(-dt*4));
    const float stormGains[]{DriveAudioConfig::stormWindLight,DriveAudioConfig::stormWindMedium,DriveAudioConfig::stormWindHeavy};
    float windTarget=AudioConfig::windGain+storm.strength()*(stormGains[static_cast<int>(storm.intensity())]+storm.gust()*DriveAudioConfig::windGustVolume);
    wind_+=(windTarget-wind_)*(1-std::exp(-dt*1.3f));
    windPitch_=1; // Never pitch-shift noise into an irritating whistling tone.
    hadPose_=r.available();lastX_=r.x();lastY_=r.y();previous_=status;
}
bool AudioSystem::init(const std::string& directory) {
    InitAudioDevice();ready_=IsAudioDeviceReady();if(!ready_) return false;
    const char* files[]{"ui_click.wav","lidar.wav","path_found.wav","route_blocked.wav","recalculating.wav","success.wav","error.wav"};
    const float durations[]{0.045f,0.60f,0.25f,0.42f,0.22f,0.65f,0.4f};
    for(std::size_t i=0;i<sounds_.size();++i) {
        std::string path=directory+"/"+files[i];
        if(FileExists(path.c_str())) sounds_[i]=LoadSound(path.c_str());
        if(!IsSoundValid(sounds_[i])) {
            auto pcm=samples(static_cast<int>(i),durations[i]);
            Wave wave{static_cast<unsigned>(pcm.size()),AudioConfig::sampleRate,16,1,pcm.data()};
            sounds_[i]=LoadSoundFromWave(wave);
        }
    }
    SetAudioStreamBufferSizeDefault(DriveAudioConfig::blockFrames);
    wind_=LoadAudioStream(DriveAudioConfig::sampleRate,32,1);
    engine_=LoadAudioStream(DriveAudioConfig::sampleRate,32,1);
    reset();return true;
}
void AudioSystem::reset() {
    if(ready_) {
        for(auto sound:sounds_) if(IsSoundValid(sound)) StopSound(sound);
        if(IsAudioStreamValid(wind_)) StopAudioStream(wind_);
        if(IsAudioStreamValid(engine_)) StopAudioStream(engine_);
    }
    state_.reset(); synth_.reset();
    engineStarts_=windStarts_=0;
    suspendFrame_=true;
}
void AudioSystem::click() {
    if(ready_ && settings.enabled && IsSoundValid(sounds_[0])) {
        SetSoundVolume(sounds_[0],settings.sfx*0.22f);PlaySound(sounds_[0]);
    }
}
bool AudioSystem::anyPlaying() const {
    if(!ready_) return false;
    if(IsAudioStreamValid(wind_) && IsAudioStreamPlaying(wind_)) return true;
    if(IsAudioStreamValid(engine_) && IsAudioStreamPlaying(engine_)) return true;
    for(auto sound:sounds_) if(IsSoundValid(sound) && IsSoundPlaying(sound)) return true;
    return false;
}
void AudioSystem::update(float dt,const Simulation& sim,const Sandstorm& storm) {
    state_.update(dt,sim,storm);
    if(suspendFrame_) {suspendFrame_=false;return;}
    if(!ready_) return;
    SetMasterVolume(settings.enabled?std::clamp(settings.master,0.0f,1.0f):0);
    if(!settings.enabled && !muted_) for(auto sound:sounds_) if(IsSoundValid(sound)) StopSound(sound);
    muted_=!settings.enabled;
    auto stream=[&](AudioStream stream,float volume,float pitch,bool isWind) {
        if(!IsAudioStreamValid(stream)) return;
        SetAudioStreamVolume(stream,settings.enabled?volume:0);SetAudioStreamPitch(stream,pitch);
        if(settings.enabled && volume>0.0001f && !IsAudioStreamPlaying(stream)) {
            PlayAudioStream(stream);
            if(isWind) ++windStarts_;else ++engineStarts_;
        }
        if(!IsAudioStreamPlaying(stream)) return;
        for(int i=0;i<2 && IsAudioStreamProcessed(stream);++i) {
            auto& buffer=isWind?windBuffer_:engineBuffer_;
            if(isWind) synth_.wind(buffer.data(),buffer.size());else synth_.engine(buffer.data(),buffer.size());
            UpdateAudioStream(stream,buffer.data(),static_cast<int>(buffer.size()));
        }
    };
    stream(wind_,state_.wind()*settings.ambient,1,true);
    stream(engine_,state_.engine()*settings.sfx,state_.enginePitch(),false);
    for(std::size_t i=1;i<sounds_.size();++i) if(settings.enabled && state_.cue(static_cast<AudioCue>(i)) && IsSoundValid(sounds_[i])) {
        float gain=i==static_cast<int>(AudioCue::SCAN)?SonarConfig::pingVolume:1.0f;
        SetSoundVolume(sounds_[i],settings.sfx*gain);
        if(i==static_cast<int>(AudioCue::SCAN)) SetSoundPitch(sounds_[i],sim.status()==SimulationStatus::RECALCULATING?SonarConfig::replanPingPitch:SonarConfig::pingPitch);
        PlaySound(sounds_[i]);
    }
}
void AudioSystem::shutdown() {
    if(!ready_) return;
    reset();for(auto sound:sounds_) if(IsSoundValid(sound)) UnloadSound(sound);
    if(IsAudioStreamValid(wind_)) UnloadAudioStream(wind_);
    if(IsAudioStreamValid(engine_)) UnloadAudioStream(engine_);
    CloseAudioDevice();ready_=false;
}
