#include "sandstorm.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
void check(bool value) { if(!value) {std::cerr<<"Sandstorm test failed\n";std::exit(1);} }
int main() {
    for(int level=0;level<3;++level) {
        Sandstorm storm;storm.setIntensity(static_cast<StormIntensity>(level));storm.reset(47);
        storm.setEnabled(true);storm.update(0.1f);
        check(storm.strength()>0 && storm.strength()<0.01f);
        for(int i=0;i<24;++i) storm.update(0.1f);
        check(storm.strength()>0.999f);
        float largestGust=0;int minCount=9999,maxCount=0;
        for(int i=0;i<600;++i) {
            storm.update(1.0f/60);largestGust=std::max(largestGust,storm.gust());
            int count=storm.nearCount()+storm.midCount()+storm.groundCount();
            minCount=std::min(minCount,count);maxCount=std::max(maxCount,count);
            const auto p=StormConfig::profiles[level];
            check(storm.nearCount()<=p.nearGust && storm.midCount()<=p.midGust && storm.groundCount()<=p.groundGust);
        }
        check(largestGust>0.95f && maxCount>minCount);
        check(storm.haze({0,0},{0,0},2)==0);
        check(storm.haze({0,-1000},{0,0},2)>0.1f);
        check(storm.backgroundHaze(0)>storm.backgroundHaze(1));
        storm.setEnabled(false);storm.update(0.1f);check(storm.strength()>0.99f);
        for(int i=0;i<26;++i) storm.update(0.1f);
        check(storm.strength()==0 && storm.nearCount()==0 && storm.groundCount()==0);
    }
    Sandstorm storm;storm.reset(1);storm.setEnabled(true);
    for(int i=0;i<30;++i) storm.update(0.1f);
    auto time=storm.time();storm.update(0);check(storm.time()==time);
    storm.setIntensity(StormIntensity::HEAVY);storm.update(0.1f);
    check(storm.midCount()<=static_cast<int>(StormConfig::midCapacity));
    std::cout<<"Sandstorm transitions, layers, gusts, levels and limits passed\n";
}
