#include "audio_system.h"
#include "visual_effects.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
void checkImpl(bool v,int line) { if(!v) {std::cerr<<"Drive audio test failed at "<<line<<'\n';std::exit(1);} }
#define check(...) checkImpl((__VA_ARGS__),__LINE__)
int main() {
    DriveAudioSynth synth;
    std::vector<float> wind(DriveAudioConfig::sampleRate*35);
    for(std::size_t i=0;i<wind.size();) {
        auto n=std::min<std::size_t>(DriveAudioConfig::blockFrames,wind.size()-i);synth.wind(wind.data()+i,n);i+=n;
    }
    double square=0,derivative=0,cross=0,energyA=0,energyB=0;float peak=0;
    const std::size_t lag=DriveAudioConfig::sampleRate*3;
    for(std::size_t i=0;i<wind.size();++i) {
        check(std::isfinite(wind[i]));peak=std::max(peak,std::abs(wind[i]));square+=wind[i]*wind[i];
        if(i) derivative+=(wind[i]-wind[i-1])*(wind[i]-wind[i-1]);
        if(i>=lag) {cross+=wind[i]*wind[i-lag];energyA+=wind[i]*wind[i];energyB+=wind[i-lag]*wind[i-lag];}
    }
    double correlation=cross/std::sqrt(energyA*energyB);
    check(peak<0.65f && peak>0.05f && std::abs(correlation)<0.15);
    check(std::sqrt(derivative/square)<0.4); // Soft bandwidth rather than sharp white noise.
    // Export 35 seconds for listening: same uninterrupted procedural source as runtime.
    std::filesystem::create_directories("build/audio-qa");
    std::ofstream file("build/audio-qa/wind-35s.wav",std::ios::binary);
    auto text=[&](const char* s){file.write(s,4);};
    auto number=[&](unsigned n,int count){for(int i=0;i<count;++i) file.put(static_cast<char>(n>>(i*8)));};
    text("RIFF");number(static_cast<unsigned>(36+wind.size()*2),4);text("WAVE");text("fmt ");number(16,4);
    number(1,2);number(1,2);number(DriveAudioConfig::sampleRate,4);number(DriveAudioConfig::sampleRate*2,4);number(2,2);number(16,2);
    text("data");number(static_cast<unsigned>(wind.size()*2),4);
    for(float sample:wind) number(static_cast<unsigned short>(static_cast<short>(sample*32767)),2);
    file.close();
    std::array<float,DriveAudioConfig::blockFrames> buffer{};float previous=0,enginePeak=0,maxJump=0;
    const std::size_t total=DriveAudioConfig::sampleRate*185;
    for(std::size_t i=0;i<total;) {
        auto n=std::min(buffer.size(),total-i);synth.engine(buffer.data(),n);
        for(std::size_t j=0;j<n;++j) {enginePeak=std::max(enginePeak,std::abs(buffer[j]));maxJump=std::max(maxJump,std::abs(buffer[j]-previous));previous=buffer[j];}
        i+=n;
    }
    check(synth.engineFrames()==total && enginePeak<0.4f && maxJump<0.03f);
    Simulation sim(Grid(80,60),{1,1},{78,58});sim.setSpeed(1);sim.play();sim.update(1);
    VisualEffects visual;visual.reset(42);AudioState audio;int pings=0;
    for(int i=0;i<600;++i) {
        visual.update(1.0f/60,sim,{1,1});audio.update(1.0f/60,sim,Sandstorm{});
        check(std::abs(visual.sonar().angle()-audio.sonarAngle())<0.0001f);
        pings+=audio.cue(AudioCue::SCAN);
    }
    check(pings==2); // Full rotations only, no immediate startup ping.
    sim.pause();float angle=audio.sonarAngle();
    for(int i=0;i<120;++i) {audio.update(1.0f/60,sim,Sandstorm{});check(!audio.cue(AudioCue::SCAN));}
    check(audio.sonarAngle()==angle);
    check(SonarConfig::replanPeriod<SonarConfig::searchPeriod && SonarConfig::drivePeriod>SonarConfig::searchPeriod);
    std::cout<<"35s wind correlation at 3s="<<correlation<<" RMS="<<std::sqrt(square/wind.size())
             <<" engine185s max jump="<<maxJump<<" sonar sync passed\n";
}
