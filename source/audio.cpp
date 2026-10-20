#include "audio.hpp"
#include <algorithm>
#include <cmath>
namespace deck {
std::vector<int16_t> soundPcm(Sound sound){
    constexpr float pi=3.14159265359f;
    const float durations[]={.11f,.38f,.16f,.75f,.18f,.065f,.14f};
    int count=static_cast<int>(durations[static_cast<int>(sound)]*SoundRate);
    std::vector<int16_t> out(count);uint32_t noise=0x0c17ca1;
    float lowNoise=0;
    for(int i=0;i<count;++i){float t=float(i)/SoundRate,v=0;
        noise^=noise<<13;noise^=noise>>17;noise^=noise<<5;float hiss=(int(noise&65535)-32768)/32768.0f;
        switch(sound){
        case Sound::Tick:
            // A short, downward-pitched body hit, with a damped low noise transient.
            lowNoise+=(hiss-lowNoise)*.12f;
            v=(.55f*std::sin(2*pi*(72*t+146*(1-std::exp(-40*t))/40))+.28f*lowNoise)*std::exp(-t*38);break;
        case Sound::Impact:{
            // A fast blade swish into a sharp strike and inharmonic steel ring.
            lowNoise+=(hiss-lowNoise)*.15f;
            v=.22f*(hiss-lowNoise)*std::sin(pi*std::min(1.f,t/.025f))*std::exp(-t*25);
            float u=t-.018f;if(u>=0){
                v+=.42f*hiss*std::exp(-u*85)+.28f*std::sin(2*pi*(112*u-90*u*u))*std::exp(-u*27);
                v+=(.23f*std::sin(2*pi*1460*u)+.18f*std::sin(2*pi*2330*u)+.11f*std::sin(2*pi*3470*u))*std::exp(-u*16)*(1-std::exp(-u*1600));
            }break;}
        case Sound::Chime:
            v=(.22f*std::sin(2*pi*880*t)+.12f*std::sin(2*pi*1320*t))*std::exp(-t*24);break;
        case Sound::Heal:{
            // Bright potion sequence: ascending fifths with a final glassy shimmer.
            const float notes[]={523.25f,659.25f,783.99f,1046.50f,1567.98f};
            for(int n=0;n<5;++n){float u=t-n*.105f;
                if(u>=0)v+=.15f*(std::sin(2*pi*notes[n]*u)+.32f*std::sin(2*pi*notes[n]*2*u))*std::exp(-u*(n==4?5.0f:12.0f));}
            float sparkle=std::max(0.f,t-.44f);
            if(t>=.44f)v+=.07f*std::sin(2*pi*2100*sparkle)*std::exp(-sparkle*12);
            v+=.012f*hiss*std::exp(-t*30);break;}
        case Sound::DiceShake:v=(.22f*hiss+.09f*std::sin(2*pi*240*t))*std::exp(-t*18)*(0.55f+.45f*std::sin(t*150));break;
        case Sound::DiceTap:v=(.34f*hiss+.18f*std::sin(2*pi*185*t))*std::exp(-t*75);break;
        case Sound::CoinClink:v=(.2f*std::sin(2*pi*1870*t)+.14f*std::sin(2*pi*2910*t)+.06f*hiss)*std::exp(-t*38);break;
        }
        float fade=std::min(1.0f,t/.0015f)*std::min(1.0f,(count-1-i)/(SoundRate*.008f));
        out[i]=static_cast<int16_t>(std::clamp(v*fade,-.85f,.85f)*32767);
    }return out;
}
}
