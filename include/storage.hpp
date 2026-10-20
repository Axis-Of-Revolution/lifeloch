#pragma once
#include "model.hpp"
#include <vector>
namespace deck {
std::vector<uint8_t> encode(const State& s,uint32_t sequence=0);
bool decode(const std::vector<uint8_t>& bytes,State& s,uint32_t& sequence);
enum class LoadResult { Missing, Loaded, Recovered, Unreadable };
class Storage {
public:
    explicit Storage(std::string directory):dir(directory){}
    LoadResult load(State& state);
    bool save(const State& state);
    bool blocked=false;
private:
    std::string dir;
    uint32_t sequence=0;
    int nextSlot=0;
};
}
