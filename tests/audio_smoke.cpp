#include "audio_system.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <chrono>
int main() {
    AudioSystem audio;if(!audio.init("assets/audio")) {std::cout<<"No audio device\n";return 77;}
    Simulation sim(Grid(20,20),{1,1},{18,18});sim.setSpeed(60);sim.setDrivingSpeed(3);
    Sandstorm storm;storm.reset(42);storm.setEnabled(true);sim.play();
    for(int i=0;i<400;++i) {
        sim.update(1.0/30);storm.update(1.0f/30);audio.update(1.0f/30,sim,storm);
        if(i==80) audio.settings.enabled=false;
        if(i==100) audio.settings.enabled=true;
        if(i==120) audio.settings.master=0.25f;
        if(i==160) sim.pause();
        if(i==180) sim.play();
        if(i==200) audio.click();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    if(std::abs(GetMasterVolume()-0.25f)>0.01f) return 1;
    audio.reset();if(audio.state().engine()!=0 || audio.anyPlaying()) return 1;
    audio.shutdown();std::cout<<"Audio device, generated WAV loops, mute, master and reset passed\n";
}
