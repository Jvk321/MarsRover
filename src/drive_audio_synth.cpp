#include "drive_audio_synth.h"
#include <cmath>
#include <algorithm>
namespace { constexpr double pi=3.14159265358979323846,rate=DriveAudioConfig::sampleRate;
double coefficient(double hz) { return 1-std::exp(-2*pi*hz/rate); } }
float DriveAudioSynth::noise() { random_=random_*1664525u+1013904223u;return (random_>>8)/8388608.0f-1; }
void DriveAudioSynth::reset() { *this=DriveAudioSynth{}; }
void DriveAudioSynth::wind(float* buffer,std::size_t frames) {
    const double r=coefficient(DriveAudioConfig::windRumbleHz),a=coefficient(DriveAudioConfig::windAirHz);
    const double s=coefficient(0.035),s2=coefficient(0.11);
    for(std::size_t i=0;i<frames;++i) {
        double n=noise();rumble_+=(n-rumble_)*r;
        air_+=(n-air_)*a;air2_+=(air_-air2_)*a;
        slow_+=(n-slow_)*s;slow2_+=(n-slow2_)*s2;
        double t=static_cast<double>(windFrames_++)/rate;
        double gust=0.80+0.12*std::sin(2*pi*t/31.7)+0.06*std::sin(2*pi*t/47.3)+std::clamp(slow_*8+slow2_*3,-0.15,0.15);
        // Warm low-passed noise plus a barely audible sub-bass bed; no pitched whistle.
        double value=gust*(rumble_*1.9+air2_*0.55)+0.012*std::sin(2*pi*32*t)*(0.7+slow_);
        buffer[i]=static_cast<float>(std::clamp(value,-0.65,0.65));
    }
}
void DriveAudioSynth::engine(float* buffer,std::size_t frames) {
    for(std::size_t i=0;i<frames;++i) {
        double t=static_cast<double>(engineFrames_++)/rate;
        phase_+=2*pi*86/rate;if(phase_>2*pi) phase_-=2*pi;
        double hum=0.19*std::sin(phase_)+0.055*std::sin(phase_*2);
        double whirr=0.025*std::sin(2*pi*344*t+0.12*std::sin(2*pi*1.13*t));
        double servo=0.007*std::sin(2*pi*610*t)*(0.5+0.5*std::sin(2*pi*0.71*t));
        buffer[i]=static_cast<float>(hum+whirr+servo);
    }
}
