#pragma once
#include <cstdint>
#include <vector>
namespace deck {
enum class Sound {Tick,Impact,Chime,Heal,DiceShake,DiceTap,CoinClink};
constexpr int SoundCount=7;
constexpr int SoundRate=22050;
// Original synthesized retro life and tray collision effects.
std::vector<int16_t> soundPcm(Sound sound);
}
