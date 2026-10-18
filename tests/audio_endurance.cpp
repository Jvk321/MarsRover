#include "audio_system.h"
#include <chrono>
#include <thread>
#include <iostream>
int main() {
    AudioSystem audio;if(!audio.init("assets/audio")) return 77;
    Simulation sim(Grid(80,60),{1,1},{78,58});sim.setDrivingSpeed(0.5);
    for(int i=0;sim.status()!=SimulationStatus::PATH_READY;++i) {if(i>20000) return 1;sim.step();}
    sim.step(); // Parked in DRIVING; first 35 s are ambient only.
    Sandstorm storm;storm.reset(42);
    using Clock=std::chrono::steady_clock;auto start=Clock::now(),last=start;
    int report=-1;
    while(true) {
        auto now=Clock::now();float elapsed=std::chrono::duration<float>(now-start).count();
        float dt=std::chrono::duration<float>(now-last).count();last=now;
        if(elapsed>=240) break;
        if(elapsed>=35 && !(elapsed>=100 && elapsed<105)) {if(!sim.playing()) sim.play();}
        else if(sim.playing()) sim.pause();
        audio.settings.enabled=!(elapsed>=70 && elapsed<73);
        storm.setEnabled(elapsed>=115 && elapsed<210);
        storm.setIntensity(elapsed<140?StormIntensity::LIGHT:elapsed<170?StormIntensity::MEDIUM:StormIntensity::HEAVY);
        sim.update(dt);storm.update(dt);audio.update(dt,sim,storm);
        if(static_cast<int>(elapsed/20)!=report) {
            report=static_cast<int>(elapsed/20);
            std::cout<<"elapsed="<<elapsed<<" windStarts="<<audio.windStarts()<<" engineStarts="<<audio.engineStarts()<<std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    bool okay=audio.windStarts()==1 && audio.engineStarts()==1 &&
              audio.synth().windFrames()>DriveAudioConfig::sampleRate*230ULL &&
              audio.synth().engineFrames()>DriveAudioConfig::sampleRate*180ULL;
    std::cout<<"endurance wind frames="<<audio.synth().windFrames()<<" engine frames="<<audio.synth().engineFrames()<<std::endl;
    audio.reset();okay=okay && !audio.anyPlaying();audio.shutdown();
    std::cout<<(okay?"240 s audio endurance passed":"Audio endurance failed")<<std::endl;
    return okay?0:1;
}
