#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace deck {
constexpr int MaxPlayers = 6, MaxProfiles = 16, MaxPresets = 8;
constexpr int NamedProfileStart = 6, MaxNamedProfiles = 10, MaxStoredProfiles = 30;
constexpr int MinLife = -9999, MaxLife = 99999, MaxCounter = 999;
struct Config {
    int players = 2, life = 20;
    bool commander = false, partners = false, linkLife = true;
    int heads = 1;
    bool turnEnabled = false, timerEnabled = false;
    bool clockEnabled = true;
    int timerSeconds = 3000, layout = 0;
    int startTurn = 0;
    int themeColors = 31;
    std::array<int,MaxPlayers> rotations{};
    bool operator==(const Config& b) const;
};
struct Profile { std::string name; int color = 0; };
struct Preset { std::string name; Config config; };
struct Player {
    std::string name;
    bool profileUsed = false;
    int profileIndex = -1; // Saved-profile link; -1 retains a detached identity.
    int color = 0, life = 40, poison = 0, energy = 0, experience = 0;
    std::array<std::array<int, 2>, MaxPlayers> damage{};
    bool operator==(const Player& b) const;
};
enum class HistoryKind { Life, Poison };
struct LifeEvent {
    int team=0, player=0, before=0, after=0, source=-1, slot=0;
    uint32_t seconds=0;
    HistoryKind kind=HistoryKind::Life;
    int turn=-1; // -1 for historic saves predating turn attribution
    bool operator==(const LifeEvent& b) const;
};
struct Game {
    bool active = false;
    Config config;
    std::array<Player, MaxPlayers> players{};
    int monarch = -1, initiative = -1;
    uint32_t elapsed = 0;
    int turn = 0, timerRemaining = 3000;
    std::array<int,7> floatPool{}; // W, U, B, R, G, colorless, storm; shared by the table.
    bool timerRunning = false;
    uint64_t timerDeadline = 0;
    std::vector<LifeEvent> history;
    bool operator==(const Game& b) const;
};
struct State {
    std::vector<Profile> profiles;
    std::vector<Preset> presets;
    Config setup;
    std::array<int, MaxPlayers> seats{{-1,-1,-1,-1,-1,-1}};
    Game game;
    bool light = false; // Legacy save field; appearance now uses themeColors.
    int themeColors = 31; // W U B R G bits, one to five selected.
};
enum class Counter { Life, Poison, Energy, Experience, Commander, Turn };
bool validConfig(const Config& c);
bool validState(const State& s);
std::string cleanName(const std::string& name);
State defaults();
std::string defaultPlayerName(int seat);
std::string playerAbbreviation(const Player& player,int seat);
class Model {
public:
    State state = defaults();
    uint32_t revision = 0;
    void touch() { ++revision; }
    void beginGesture();
    void endGesture();
    void setInputTime(uint64_t ms) { inputMs=ms; hasInputClock=true; }
    bool start();
    bool resetTable();
    bool setLife(int player,int value);
    bool changeFloat(int index,int delta);
    bool resetFloat();
    bool change(int player, Counter kind, int delta, int source = 0, int slot = 0);
    bool marker(int player, bool initiative);
    bool undo();
    bool redo();
    bool canUndo() const { return !past.empty(); }
    bool canRedo() const { return !future.empty(); }
    void clearHistory();
    bool assign(int seat, int profile);
    bool assignActive(int seat, int profile);
    bool addProfile(const std::string& name);
    bool renameProfile(int index, const std::string& name);
    bool deleteProfile(int index);
    bool colorProfile(int index,int color);
    bool savePreset(const std::string& name);
    bool deletePreset(int index);
    bool atThreshold(int player) const;
    int teamOf(int player) const { return player/state.game.config.heads; }
    int poisonLimit() const { return state.game.config.heads>1?15:10; }
private:
    void refreshProfile(int index);
    bool gesture = false;
    bool hasInputClock=false,batchAvailable=false,gestureChanged=false;
    uint64_t inputMs=0,gestureFirstMs=0,gestureLastMs=0,lastBatchMs=0;
    uint32_t batchSeconds=0;
    Game before;
    int actor=0, damageSource=-1, damageSlot=0;
    int batchActor=-1,batchSource=-1,batchSlot=0;
    Counter gestureKind=Counter::Life,batchKind=Counter::Life;
    std::vector<Game> past, future;
};
}
