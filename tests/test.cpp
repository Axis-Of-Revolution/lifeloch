#include "model.hpp"
#include "storage.hpp"
#include "ui.hpp"
#include "pixel.hpp"
#include "layout.hpp"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
using namespace deck;
namespace fs=std::filesystem;
struct Probe:Renderer {
    int objects=0;int screenIndex=0;
    std::string lastText;
    std::vector<std::string> topWords,bottomWords;
    void screen(int n)override{screenIndex=n;}
    void box(Rect r,Color,float=0)override{if(r.x+r.w>(screenIndex?320:400)+1||r.y+r.h>241)std::cerr<<"Overflow: "<<lastText<<" at "<<r.x<<","<<r.y<<","<<r.w<<","<<r.h<<" screen="<<screenIndex<<"\n";assert(r.w>=0&&r.h>=0);assert(r.x>=-1&&r.y>=-1);assert(r.x+r.w<=(screenIndex?320:400)+1);assert(r.y+r.h<=241);++objects;}
    void text(const std::string& s,float x,float y,float size,Color c,Align a,float w)override{lastText=s;(screenIndex?bottomWords:topWords).push_back(s);pixelText(*this,s,x,y,size,c,a,w);}
};
void checkRoundtrip(const State& state){auto b=encode(state,1234);State loaded;uint32_t seq=0;assert(!b.empty());assert(decode(b,loaded,seq));assert(seq==1234);assert(encode(loaded,seq)==b);}
int main(){
    Model m;assert(validState(m.state));assert(m.state.setup.players==2&&m.state.setup.life==20);for(int seat:m.state.seats)assert(seat==-1);m.state.setup={4,40,true,false,true};assert(m.start());
    assert(m.change(0,Counter::Life,-3));assert(m.state.game.players[0].life==37);assert(m.state.game.history.size()==1);
    assert(m.undo());assert(m.state.game.players[0].life==40);assert(m.state.game.history.empty());assert(m.redo());assert(m.state.game.players[0].life==37);
    m.beginGesture();for(int i=0;i<7;++i)m.change(0,Counter::Life,1);m.endGesture();assert(m.state.game.history.size()==2);assert(m.state.game.history.back().after-m.state.game.history.back().before==7);m.undo();assert(m.state.game.players[0].life==37);
    assert(m.change(0,Counter::Commander,5,1));assert(m.state.game.players[0].life==32);assert(m.state.game.players[0].damage[1][0]==5);assert(m.state.game.players[1].damage[0][0]==0);assert(!m.canRedo());
    m.state.game.config.linkLife=false;m.change(0,Counter::Commander,20,2);assert(m.state.game.players[0].life==32);assert(!m.atThreshold(0));m.change(0,Counter::Commander,1,2);assert(m.atThreshold(0));
    m.state.game.config.partners=true;m.change(2,Counter::Commander,12,1,0);m.change(2,Counter::Commander,12,1,1);assert(!m.atThreshold(2));
    for(int heads:{2,3}){
        Model team;team.state.setup={6,heads==2?30:45,true,true,true,heads};assert(team.start());team.change(0,Counter::Life,-4);
        for(int i=0;i<heads;++i)assert(team.state.game.players[i].life==team.state.setup.life-4);
        assert(team.state.game.players[heads].life==team.state.setup.life);
        team.change(1,Counter::Commander,8,heads,1);assert(team.state.game.players[0].damage[heads][1]==0);assert(team.state.game.players[1].damage[heads][1]==8);
        for(int i=0;i<heads;++i)assert(team.state.game.players[i].life==team.state.setup.life-12);
        team.change(1,Counter::Poison,team.poisonLimit());for(int i=0;i<heads;++i)assert(team.state.game.players[i].poison==team.poisonLimit());assert(team.atThreshold(0));
        assert(team.undo());assert(team.state.game.players[0].poison==0);team.undo();assert(team.state.game.players[1].damage[heads][1]==0);assert(team.state.game.players[0].life==team.state.setup.life-4);
        assert(team.state.game.history.size()==1);checkRoundtrip(team.state);
    }
    m.change(3,Counter::Life,INT_MIN);assert(m.state.game.players[3].life==MinLife);m.change(3,Counter::Life,INT_MAX);assert(m.state.game.players[3].life==MaxLife);
    m.change(3,Counter::Poison,INT_MAX);assert(m.state.game.players[3].poison==MaxCounter);m.change(3,Counter::Poison,INT_MIN);assert(m.state.game.players[3].poison==0);
    assert(!m.change(-1,Counter::Life,1));assert(!m.change(6,Counter::Life,1));assert(!m.change(1,Counter::Commander,1,7));
    assert(cleanName("   ").empty());assert(cleanName(" abc ")=="abc");assert(cleanName("12345678901234567")=="1234567890123456");assert(cleanName("\xc0\xaf").empty());assert(cleanName("\xe2\x82\xac")!="");
    m.addProfile("Mira");m.assign(0,6);m.assign(1,0);assert(validState(m.state));m.deleteProfile(6);assert(validState(m.state));assert(!m.deleteProfile(0));
    checkRoundtrip(m.state);auto encoded=encode(m.state);State loaded;uint32_t seq=0;
    for(size_t i=0;i<encoded.size();i+=7){auto b=encoded;b[i]^=0x80;assert(!decode(b,loaded,seq));}
    for(size_t n=0;n<encoded.size();n+=13){auto b=encoded;b.resize(n);assert(!decode(b,loaded,seq));}
    fs::path dir="build-host/save-test";fs::remove_all(dir);fs::create_directories(dir);Storage store(dir.string());assert(store.load(loaded)==LoadResult::Missing);assert(store.save(m.state));m.change(0,Counter::Life,1);assert(store.save(m.state));
    Storage reopen(dir.string());assert(reopen.load(loaded)==LoadResult::Loaded);assert(loaded.game.players[0].life==m.state.game.players[0].life);
    std::ofstream(dir/"save-b.bin",std::ios::trunc)<<"damaged";Storage recover(dir.string());assert(recover.load(loaded)==LoadResult::Recovered);assert(loaded.game.players[0].life==m.state.game.players[0].life-1);
    std::ofstream(dir/"save-a.bin",std::ios::trunc)<<"damaged";Storage corrupt(dir.string());assert(corrupt.load(loaded)==LoadResult::Unreadable);assert(!corrupt.save(m.state));fs::remove_all(dir);
    // Random counters, undo, redo and serialization across all supported team layouts.
    std::mt19937 rng(532487);for(int heads:{1,2,3}){
        Model fuzz;fuzz.state.setup={6,40,true,true,true,heads};assert(fuzz.start());
        for(int k=0;k<3000;++k){int p=rng()%6,c=rng()%5,d=static_cast<int>(rng()%21)-10;fuzz.state.game.elapsed++;
            if(k%17==0)fuzz.undo();else if(k%29==0)fuzz.redo();else fuzz.change(p,static_cast<Counter>(c),d,rng()%6,rng()%2);
            assert(validState(fuzz.state));if(k%100==0)checkRoundtrip(fuzz.state);
        }assert(fuzz.state.game.history.size()<=256);
    }
    App app;app.model.state.setup={4,40,true,false,true};app.model.start();app.page=Page::Game;Probe probe;app.render(probe);assert(probe.objects<32768);
    app.tick(1000);app.pointerDown(235,115,1000);app.tick(1500);app.tick(1700);app.pointerUp();assert(app.model.state.game.players[0].life==43);assert(app.model.state.game.history.size()==1);
    app.dispatch({Action::Undo});assert(app.model.state.game.players[0].life==40);app.notice.clear();
    app.dispatch({Action::NewGame});app.dispatch({Action::Heads,3});assert(app.model.state.setup.players==6);assert(validState(app.model.state));app.dispatch({Action::Life});app.submitText("-1",true);assert(app.model.state.setup.life==40);app.dispatch({Action::Life});app.submitText("75",true);assert(app.model.state.setup.life==75);
    int largest=0;
    for(int count=2;count<=6;++count)for(int light=0;light<2;++light){app.model.state.setup={count,40,true,true,true,1};app.model.start();app.model.state.light=light;app.model.change(0,Counter::Life,-33);app.model.change(1,Counter::Life,-40);app.notice.clear();for(auto page:{Page::Home,Page::Rules,Page::Seats,Page::Profiles,Page::Presets,Page::Game,Page::History,Page::Menu,Page::Help,Page::Dice}){app.page=page;for(int tab=0;tab<3;++tab){app.tab=tab;Probe render;app.render(render);largest=std::max(largest,render.objects);assert(render.objects<32768);for(auto& b:app.buttons){assert(b.rect.x>=0&&b.rect.y>=0&&b.rect.x+b.rect.w<=320&&b.rect.y+b.rect.h<=240);}}}}
    for(int heads:{2,3}){app.model.state.setup={6,45,true,true,true,heads};app.model.start();app.page=Page::Game;Probe render;app.render(render);largest=std::max(largest,render.objects);}
    for(int n=2;n<=6;++n)for(int layout=0;layout<layoutCount(n);++layout)for(int angle=0;angle<4;++angle){
        app.model.state.setup={n,40,true,true,true,1};chooseLayout(app.model.state.setup,layout);app.model.state.setup.rotations.fill(angle);assert(app.model.start());app.notice.clear();
        app.page=Page::Game;Probe render;app.render(render);largest=std::max(largest,render.objects);assert(render.objects<32768);
        app.page=Page::Layout;Probe picker;app.render(picker);
        for(const auto& b:app.buttons)assert(b.label.find(":90")==std::string::npos&&b.label.find(":180")==std::string::npos&&b.label.find(":270")==std::string::npos);
    }
    app.model.state.setup={4,40,true,false,true,1};app.model.state.setup.turnEnabled=true;app.model.state.setup.startTurn=8;app.model.state.setup.timerEnabled=true;app.model.state.setup.timerSeconds=3000;app.model.state.setup.clockEnabled=false;app.model.savePreset("Timed table");
    checkRoundtrip(app.model.state);assert(app.model.state.presets.back().config==app.model.state.setup);assert(app.model.start());assert(app.model.state.game.turn==8);
    app.wallSeconds=1800000000;app.tick(10000);assert(app.model.state.game.timerDeadline==1800003000);app.wallSeconds+=30;app.tick(40000);assert(app.model.state.game.timerRemaining==2970);
    app.dispatch({Action::Clock});app.dispatch({Action::TimerPause});app.wallSeconds+=60;app.tick(100000);assert(app.model.state.game.timerRemaining==2970);
    app.dispatch({Action::TimerPause});app.wallSeconds+=10;app.tick(110000);assert(app.model.state.game.timerRemaining==2960);
    app.model.change(0,Counter::Turn,2);assert(app.model.state.game.turn==10);app.model.undo();assert(app.model.state.game.turn==8);assert(app.model.state.game.timerRemaining==2960);
    app.wallSeconds+=3000;app.tick(3110000);assert(app.model.state.game.timerRemaining==0&&!app.model.state.game.timerRunning);checkRoundtrip(app.model.state);
    // Separate taps are one transaction when each gap is strictly below 2 seconds.
    Model batches;batches.state.setup={4,40,true,true,true,1};batches.start();
    for(uint64_t ms:{100ull,600ull,2599ull}){batches.setInputTime(ms);batches.change(0,Counter::Life,-1);}
    assert(batches.state.game.history.size()==1);assert(batches.state.game.history.back().before==40&&batches.state.game.history.back().after==37);
    batches.setInputTime(4599);batches.change(0,Counter::Life,-1);assert(batches.state.game.history.size()==2);
    batches.undo();assert(batches.state.game.players[0].life==37);batches.undo();assert(batches.state.game.players[0].life==40);assert(batches.state.game.history.empty());
    batches.redo();assert(batches.state.game.players[0].life==37);batches.setInputTime(4700);batches.change(0,Counter::Life,-1);assert(batches.state.game.history.size()==2);
    Model net;net.start();net.setInputTime(100);net.change(0,Counter::Life,-1);net.setInputTime(1100);net.change(0,Counter::Life,1);assert(net.state.game.history.empty()&&!net.canUndo());
    Model sources;sources.state.setup={4,40,true,true,true,2};sources.start();sources.setInputTime(100);sources.change(0,Counter::Life,-1);sources.setInputTime(1000);sources.change(1,Counter::Life,-2);assert(sources.state.game.history.size()==1&&sources.state.game.players[0].life==37);
    sources.setInputTime(1200);sources.change(0,Counter::Commander,2,2,0);sources.setInputTime(1400);sources.change(0,Counter::Commander,3,3,0);sources.setInputTime(1600);sources.change(1,Counter::Commander,1,3,0);assert(sources.state.game.history.size()==4);sources.undo();assert(sources.state.game.players[1].damage[3][0]==0&&sources.state.game.players[0].damage[3][0]==3);
    // All orientation defaults describe the physical direction of the text's bottom.
    Config facing;chooseLayout(facing,1);assert(facing.rotations[0]==1&&facing.rotations[1]==3);
    for(int n=3;n<=6;++n)for(int layout=0;layout<layoutCount(n);++layout){Config c;c.players=n;chooseLayout(c,layout);auto rects=playerPanels(n,layout);
        for(int i=0;i<n;++i){auto r=rects[i];int expected=(r.h>r.w*1.5f)?(r.x<200?1:3):(r.y+r.h/2<122?2:0);assert(c.rotations[i]==expected);}

    }
    // New games and presets begin at 20, profiles remain optional, and explicit life survives layout edits.
    App setup;setup.model.assign(0,0);setup.dispatch({Action::NewGame});assert(setup.model.state.setup.life==20&&setup.model.state.setup.players==2);for(int seat:setup.model.state.seats)assert(seat==-1);
    setup.dispatch({Action::PlayerCount,3});assert(setup.model.state.setup.life==40);setup.dispatch({Action::Life});setup.submitText("72",true);setup.dispatch({Action::PlayerCount,3});assert(setup.model.state.setup.life==72);setup.dispatch({Action::PlayerCount,2});assert(setup.model.state.setup.life==20);
    setup.model.assign(0,0);setup.model.savePreset("Anonymous config");int preset=static_cast<int>(setup.model.state.presets.size())-1;setup.model.assign(0,1);setup.dispatch({Action::UsePreset,preset});assert(setup.model.state.seats[0]==1);setup.model.assign(0,-1);assert(setup.model.start());assert(setup.model.state.game.players[0].name=="Player A");checkRoundtrip(setup.model.state);
    // Five changes fit beside a life total, and the All view supports dragging, buttons, and D-pad scrolling.
    App history;history.model.state.setup={4,40,true,false,true,1};history.model.start();history.page=Page::Game;
    for(int i=1;i<=9;++i){history.model.setInputTime(i*3000);history.model.change(0,Counter::Life,-i);}
    Probe recent;history.render(recent);assert(std::find(recent.topWords.begin(),recent.topWords.end(),"-9→-5")!=recent.topWords.end());
    auto visible=std::count_if(recent.topWords.begin(),recent.topWords.end(),[](const auto& text){return text.find("→")!=std::string::npos;});assert(visible>=1&&visible<=9);
    history.dispatch({Action::History});assert(history.showingAllHistory());Probe all;history.render(all);assert(std::find(all.bottomWords.begin(),all.bottomWords.end(),"All")!=all.bottomWords.end());
    history.pointerDown(160,170,40000);history.pointerMove(160,116);history.pointerUp();assert(history.historyOffset()==2);history.keyDown(Key::Down,40100);assert(history.historyOffset()==3);history.dispatch({Action::HistoryScroll,-1});assert(history.historyOffset()==2);history.scrollHistory(-10000);assert(history.historyOffset()==0);
    history.dispatch({Action::SelectPlayer,0});assert(!history.showingAllHistory());Probe individual;history.render(individual);for(const auto& word:individual.bottomWords)if(word.find("→")!=std::string::npos)assert(word.find(':')==std::string::npos);
    // Reset preserves both running and paused timers, and can be undone without rewinding either timer.
    Model reset;reset.state.setup={4,40,true,true,true,1};reset.state.setup.timerEnabled=true;reset.state.setup.turnEnabled=true;reset.start();auto& resetGame=reset.state.game;resetGame.timerRemaining=1234;resetGame.timerDeadline=1800001234;resetGame.turn=8;reset.change(0,Counter::Commander,7,1,1);reset.change(0,Counter::Poison,2);reset.change(0,Counter::Energy,5);reset.change(0,Counter::Experience,9);reset.marker(0,false);reset.marker(1,true);auto prior=resetGame;
    reset.resetTable();assert(resetGame.timerRunning&&resetGame.timerRemaining==1234&&resetGame.timerDeadline==1800001234);assert(resetGame.turn==0&&resetGame.history.empty()&&resetGame.monarch==-1&&resetGame.initiative==-1);for(const auto& p:resetGame.players){assert(p.life==40&&p.poison==0&&p.energy==0&&p.experience==0);for(auto damage:p.damage)assert(damage[0]==0&&damage[1]==0);}
    reset.undo();assert(resetGame.players==prior.players&&resetGame.history==prior.history);resetGame.timerRunning=false;resetGame.timerDeadline=0;reset.resetTable();assert(!resetGame.timerRunning&&resetGame.timerDeadline==0&&resetGame.timerRemaining==1234);checkRoundtrip(reset.state);
    app.dispatch({Action::Dice});uint64_t time=100000;
    for(int sides:{2,4,6,8,10,12,20}){app.dispatch({Action::Die,sides});for(int n=0;n<200;++n){app.tick(time);app.dispatch({Action::Roll});assert(app.rolling);assert(app.dieResult==0);app.tick(time+500);Probe render;app.render(render);app.tick(time+4000);assert(!app.rolling);assert(app.dieResult>=0&&app.dieResult<=sides);time+=4500;}}
    std::cout<<"PASS: counter arithmetic, Giant sharing, individual commanders, partner thresholds, grouped undo/history, UTF-8, profiles, save integrity/recovery, 9,000 random transitions, every screen, and 1,400 dice rolls. Maximum drawing objects: "<<largest<<"\n";
}
