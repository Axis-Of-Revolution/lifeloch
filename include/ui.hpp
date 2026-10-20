#pragma once
#include "model.hpp"
#include "audio.hpp"
#include "dice.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace deck {
using Color=uint32_t; // 0xRRGGBB
struct Rect {float x,y,w,h;bool operator==(const Rect& other)const{return x==other.x&&y==other.y&&w==other.w&&h==other.h;}bool contains(float px,float py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
enum class Align { Left, Center, Right };
struct Renderer {
    virtual ~Renderer()=default;
    virtual void screen(int which)=0;
    virtual void box(Rect r,Color c,float radius=0)=0;
    virtual Color textColor(Color c,float,float)const{return c;}
    virtual void gradient(Rect r,Color tl,Color tr,Color bl,Color br);
    // Native draws one cached texture; host renderers may override for screenshots.
    virtual void homeArtwork(Rect) {}
    virtual void text(const std::string& s,float x,float y,float size,Color c,Align align=Align::Left,float maxWidth=0)=0;
};
enum class Page { Home, Rules, Seats, Profiles, Profile, Presets, Game, History, Menu, Help, Dice, Layout, Options, Clock, Theme, Float, ActiveSeats, GameTrackers };
enum class Action {
    None, Home, NewGame, Resume, Rules, Seats, Begin, SelectPlayer, Tab, Change,
    Undo, Redo, History, Menu, Help, Theme, LinkLife, Players, Life, Heads,
    Commander, Partners, PickSeat, Profiles, Profile, NewProfile, RenameProfile,
    Color, DeleteProfile, Presets, UsePreset, SavePreset, DeletePreset, Page,
    Source, Slot, Monarch, Initiative, Back, Confirm, Cancel, Exit, Dice, Die, Roll,
    PlayerCount, Layout, Options, TurnToggle, TimerToggle, TimerInput, Clock, TurnInput, TimerPause, ClockToggle,
    HistoryAll, HistoryScroll, ResetTable, ThemeColor, SetLife, QuickTurn, Float, ChangeFloat, ResetFloat,
    GameLayout, LayoutDone, ManageProfiles, GameTrackers, ProfileDone
};
struct Command {Action action=Action::None;int a=0,b=0;bool operator==(const Command& o)const{return action==o.action&&a==o.a&&b==o.b;}};
struct Button {Rect rect;Command command;std::string label;bool enabled=true,repeat=false;};
enum class InputKind { None, NewProfile, RenameProfile, Preset, Life, Turn, TimerSetup, TimerGame, TurnSetup, SetLife };
struct TextRequest {InputKind kind=InputKind::None;std::string title,initial;bool numeric=false;};
enum class Key { Left,Right,Up,Down,A,B,X,Y,L,R,Start,Select };
class App {
public:
    Model model;
    Page page=Page::Home;
    int selected=0,tab=0,source=1,slot=0;
    bool quitRequested=false,saveOk=true,saveBlocked=false,exitWithoutSave=false;
    std::string notice;
    TextRequest input;
    std::vector<Button> buttons;
    void render(Renderer& r);
    void dispatch(Command c);
    void pointerDown(float x,float y,uint64_t ms);
    void pointerMove(float x,float y);
    void pointerUp();
    void tick(uint64_t ms);
    void keyDown(Key k,uint64_t ms);
    void keyUp();
    void scrollHistory(float pixels);
    int historyOffset()const{return historyPage;}
    bool showingAllHistory()const{return historyAll;}
    void submitText(const std::string& value,bool accepted);
    std::string summary() const;
    bool modalOpen()const{return confirming.action!=Action::None;}
    bool buttonNavigation()const{return focusVisible;}
    Command focusedCommand()const{return focused;}
    std::string modalTitle,modalBody;
    uint32_t randomState=0x4d474943;
    int dieSides=20,dieResult=20;
    uint32_t rollSeed=1;
    bool rolling=false;
    void setTilt(float x,float y){tiltX=x;tiltY=y;}
    void setTrayRotation(float radiansPerSecond){trayYawRate=radiansPerSecond;}
    const RollSimulation& diceSimulation()const{return rollSimulation;}
    uint64_t wallSeconds=0;
    std::string currentClock="--:--";
    std::vector<Sound> takeSounds(){auto out=sounds;sounds.clear();return out;}
private:
    std::vector<Sound> sounds;
    int soundTeam=-1,soundKind=-1,soundSource=-1,soundSlot=-1,soundDirection=0,soundAmount=0;
    uint64_t soundTime=0;
    void lifeSound(int delta,int kind);
    Page themeReturn=Page::Home,diceReturn=Page::Game;
    Renderer* draw=nullptr;
    int viewport=400;
    int listPage=0,pickSeat=-1,profile=0,historyPage=0;
    bool historyAll=true,historyDragging=false;
    float historyDragY=0,scrollRemainder=0;
    Page returnPage=Page::Home;
    Command confirming,held,feedback;
    Page feedbackPage=Page::Home;
    uint64_t feedbackUntil=0;
    Rect heldRect{};
    bool holding=false,repeat=false,keyboardHold=false;
    uint64_t nextRepeat=0,lastClock=0,noticeUntil=0,now=0,rollStart=0;
    int rollHitCount=0;
    RollSimulation rollSimulation;
    float trayYawRate=0;
    float tiltX=0,tiltY=0,neutralTiltX=0,neutralTiltY=0;
    uint64_t lastRollTick=0;
    bool hasRollResult=false;
    struct LifeMotion {bool seen=false;int target=0;float from=0;uint64_t start=0;float duration=0;};
    std::array<LifeMotion,MaxPlayers> lifeMotion{};
    void syncLifeMotion();
    float displayedLife(int player)const;
    bool editingGameLayout=false,assigningLiveProfiles=false,focusVisible=false;
    Page focusPage=Page::Home;
    bool focusModal=false;
    Command focused;
    Rect focusedRect{};
    std::vector<std::string> rolls;
    struct Palette {Color bg,panel,raised,ink,muted,line,accent,accentInk,good,bad;};
    Palette p{};
    void header(const std::string& title,bool back=true);
    void button(Rect rect,const std::string& label,Command cmd,bool active=false,bool enabled=true,bool repeating=false,float font=13);
    void label(const std::string& s,float x,float y,float size=12,Color color=0,Align a=Align::Left,float width=0);
    void panel(Rect rect,Color c=0);
    void top();
    void board(const Game& game,bool preview=false);
    void miniHistory(int team,float x,float y,float width,int count=5,float spacing=9.5f,float font=9);
    int maxHistoryOffset()const;
    void historyView();
    void damageRow(const Game& g,int player,Rect area);
    void bottom();
    void identityFill(Rect area,const Player& player,bool subdued=false);
    void identityText(const std::string& text,float x,float y,float size,const Player& player,Align align=Align::Left,float width=0);
    void playerStrip();
    void footer();
    void quickControls();
    void confirm(Command command,const std::string& title,const std::string& body);
    void request(InputKind kind,const std::string& title,const std::string& initial="",bool numeric=false);
    void message(const std::string& s);
    void back();
    void syncFocus();
    void moveFocus(Key direction);
    void drawFocus();
    void activateFocus(uint64_t ms);
    bool changeCommand(Command c)const{return c.action==Action::Change;}
};
std::array<Color,2> playerColors(const Player& player,bool ink=false);
std::string colorPairName(int pair);
Color playerColor(int n,bool light=false);
std::string timeText(uint32_t seconds);
}
