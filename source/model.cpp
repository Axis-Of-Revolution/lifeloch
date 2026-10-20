#include "model.hpp"
#include <algorithm>
#include <cctype>
#include <limits>

namespace deck {
bool Config::operator==(const Config& b) const {
    return players==b.players && life==b.life && commander==b.commander && partners==b.partners && linkLife==b.linkLife && heads==b.heads && turnEnabled==b.turnEnabled && timerEnabled==b.timerEnabled && clockEnabled==b.clockEnabled && timerSeconds==b.timerSeconds && layout==b.layout && rotations==b.rotations && startTurn==b.startTurn && themeColors==b.themeColors;
}
bool Player::operator==(const Player& b) const {
    return name==b.name && profileIndex==b.profileIndex && profileUsed==b.profileUsed && color==b.color && life==b.life && poison==b.poison && energy==b.energy && experience==b.experience && damage==b.damage;
}
bool Game::operator==(const Game& b) const {
    return active==b.active && config==b.config && players==b.players && monarch==b.monarch && initiative==b.initiative && history==b.history && turn==b.turn && floatPool==b.floatPool;
}
bool LifeEvent::operator==(const LifeEvent& b) const {return team==b.team && player==b.player && before==b.before && after==b.after && source==b.source && slot==b.slot && seconds==b.seconds && kind==b.kind && turn==b.turn;}
bool validConfig(const Config& c) {
    if(!(c.players>=2 && c.players<=6 && c.life>=1 && c.life<=999 && c.heads>=1 && c.heads<=3 && c.players%c.heads==0 && c.players/c.heads>=2))return false;
    if(c.timerSeconds<60||c.timerSeconds>59940||c.startTurn<0||c.startTurn>9999||c.layout<0||c.layout>=(c.players==3?6:c.players==5?4:2)||c.themeColors<1||c.themeColors>31)return false;
    for(int r:c.rotations)if(r<0||r>3)return false;
    return true;
}
std::string cleanName(const std::string& text) {
    // Preserve complete UTF-8 code points, reject controls, and cap at 16 characters.
    std::string out; unsigned count=0;
    for (size_t i=0; i<text.size() && count<16;) {
        unsigned char c=text[i]; size_t n=c<0x80?1:(c>=0xc2 && c<=0xdf?2:(c>=0xe0 && c<=0xef?3:(c>=0xf0 && c<=0xf4?4:0)));
        if (!n || i+n>text.size()) return "";
        for(size_t k=1;k<n;++k) if((static_cast<unsigned char>(text[i+k])&0xc0)!=0x80) return "";
        if(n==3 && ((c==0xe0 && static_cast<unsigned char>(text[i+1])<0xa0)||(c==0xed && static_cast<unsigned char>(text[i+1])>=0xa0))) return "";
        if(n==4 && ((c==0xf0 && static_cast<unsigned char>(text[i+1])<0x90)||(c==0xf4 && static_cast<unsigned char>(text[i+1])>=0x90))) return "";
        if(c<32 || c==127) return "";
        out.append(text,i,n); i+=n; ++count;
    }
    while(!out.empty() && out.front()==' ') out.erase(out.begin());
    while(!out.empty() && out.back()==' ') out.pop_back();
    return out;
}
std::string defaultPlayerName(int seat) {
    static const char* names[]={"Player A","Player B","Player C","Player D","Player E","Player F"};
    return names[std::clamp(seat,0,5)];
}
std::string playerAbbreviation(const Player& player,int seat){
    if(!player.profileUsed)return std::string("P")+char('A'+std::clamp(seat,0,5));
    std::string out;size_t at=0;
    for(int count=0;count<2&&at<player.name.size();++count){unsigned char c=player.name[at];size_t n=c<128?1:(c<224?2:(c<240?3:4));
        if(c<128){out+=static_cast<char>(std::toupper(c));++at;}
        else{out.append(player.name,at,n);at+=n;}
    }return out;
}
State defaults() {
    State s;
    for(int i=0;i<6;++i) s.profiles.push_back({defaultPlayerName(i),i});
    s.setup.themeColors=s.themeColors;
    return s;
}
bool validState(const State& s) {
    if(s.themeColors<1||s.themeColors>31)return false;
    if(s.profiles.size()<6 || s.profiles.size()>MaxStoredProfiles || s.presets.size()>MaxPresets || !validConfig(s.setup) || !validConfig(s.game.config)) return false;
    for(const auto& p:s.profiles) if(p.name.empty() || cleanName(p.name)!=p.name || p.color<0 || p.color>=10) return false;
    for(const auto& p:s.presets) if(p.name.empty() || cleanName(p.name)!=p.name || !validConfig(p.config)) return false;
    for(int i=0;i<6;++i) {
        if(s.seats[i]<-1 || s.seats[i]>=static_cast<int>(s.profiles.size())) return false;
        for(int j=0;j<i;++j) if(s.seats[i]>=0 && s.seats[i]==s.seats[j]) return false;
        const auto& p=s.game.players[i];
        if(s.game.active && i<s.game.config.players && (p.name.empty() || cleanName(p.name)!=p.name)) return false;
        if(p.profileIndex < -1 || p.profileIndex >= static_cast<int>(s.profiles.size()) || (p.profileIndex>=0&&!p.profileUsed))return false;
        if(p.name.size()>64 || p.color<0 || p.color>=(p.profileUsed?10:6) || p.life<MinLife || p.life>MaxLife) return false;
        for(int n:{p.poison,p.energy,p.experience}) if(n<0 || n>MaxCounter) return false;
        for(auto pair:p.damage) for(int n:pair) if(n<0 || n>MaxCounter) return false;
    }
    const auto& g=s.game;
    for(int n:g.floatPool)if(n<0||n>MaxCounter)return false;
    if(g.turn<0||g.turn>9999||g.timerRemaining<0||g.timerRemaining>59940||g.timerDeadline>4102444800ull)return false;
    if(g.history.size()>256)return false;
    for(auto& e:g.history) if(e.team<0||e.team>=g.config.players/g.config.heads||e.player<0||e.player>=g.config.players||e.player/g.config.heads!=e.team||e.before<MinLife||e.before>MaxLife||e.after<MinLife||e.after>MaxLife||e.source<-1||e.source>=g.config.players||e.slot<0||e.slot>1||e.seconds>g.elapsed||(e.kind!=HistoryKind::Life&&e.kind!=HistoryKind::Poison)||(e.turn < -1 || e.turn>9999)||(e.kind==HistoryKind::Poison&&(e.before<0||e.before>MaxCounter||e.after<0||e.after>MaxCounter||e.source!=-1)))return false;
    if(g.active)for(int i=0;i<g.config.players;++i) {
        const auto& lead=g.players[(i/g.config.heads)*g.config.heads];
        if(g.players[i].life!=lead.life || g.players[i].poison!=lead.poison)return false;
    }
    return g.monarch>=-1 && g.monarch<g.config.players && g.initiative>=-1 && g.initiative<g.config.players;
}
void Model::beginGesture() { if(!gesture) { before=state.game; gesture=true;gestureChanged=false; actor=0; damageSource=-1; damageSlot=0;gestureKind=Counter::Life; } }
void Model::endGesture() {
    if(!gesture) return;
    if(!(before==state.game)) {
        auto& g=state.game;
        bool lifeChanged=false;
        for(int i=0;i<g.config.players;i+=g.config.heads)if(before.players[i].life!=g.players[i].life||before.players[i].poison!=g.players[i].poison)lifeChanged=true;
        bool sameRecipient=(gestureKind==Counter::Life||gestureKind==Counter::Poison)?actor/g.config.heads==batchActor/g.config.heads:actor==batchActor;
        bool merge=lifeChanged&&hasInputClock&&batchAvailable&&!past.empty()&&sameRecipient&&gestureKind==batchKind&&damageSource==batchSource&&damageSlot==batchSlot&&gestureFirstMs>=lastBatchMs&&gestureFirstMs-lastBatchMs<2000;
        Game baseline=merge?past.back():before;
        if(merge)g.history=baseline.history;
        uint32_t seconds=merge?batchSeconds:g.elapsed;
        for(int i=0;i<g.config.players;i+=g.config.heads) if(baseline.players[i].life!=g.players[i].life) {
            if(g.history.size()==256)g.history.erase(g.history.begin());
            g.history.push_back({i/g.config.heads,actor,baseline.players[i].life,g.players[i].life,damageSource,damageSlot,seconds,HistoryKind::Life,g.turn});
        }
        for(int i=0;i<g.config.players;i+=g.config.heads) if(baseline.players[i].poison!=g.players[i].poison) {
            if(g.history.size()==256)g.history.erase(g.history.begin());
            g.history.push_back({i/g.config.heads,actor,baseline.players[i].poison,g.players[i].poison,-1,0,seconds,HistoryKind::Poison,g.turn});
        }
        if(!merge){if(past.size()==64)past.erase(past.begin());past.push_back(before);}
        future.clear();
        batchAvailable=lifeChanged&&hasInputClock;
        if(merge && baseline==g){past.pop_back();batchAvailable=false;}
        batchActor=actor;batchSource=damageSource;batchSlot=damageSlot;batchKind=gestureKind;lastBatchMs=gestureLastMs;batchSeconds=seconds;
        touch();
    }
    gesture=false;
}
void Model::clearHistory() { gesture=false;batchAvailable=false; past.clear(); future.clear(); }
bool Model::start() {
    if(!validState(state)) return false;
    Game g; g.active=true; g.config=state.setup;g.timerRemaining=g.config.timerSeconds;g.timerRunning=g.config.timerEnabled;g.turn=g.config.startTurn;
    for(int i=0;i<6;++i) {auto& player=g.players[i];int seat=state.seats[i];player.profileIndex=seat;player.profileUsed=seat>=0;player.name=seat<0?defaultPlayerName(i):state.profiles[seat].name;player.color=seat<0?i:state.profiles[seat].color;player.life=g.config.life;}
    state.game=g; state.themeColors=g.config.themeColors; clearHistory(); touch(); return true;
}
bool Model::resetTable() {
    auto& g=state.game;if(!g.active)return false;
    endGesture();Game previous=g;
    for(auto& p:g.players){p.life=g.config.life;p.poison=0;p.energy=0;p.experience=0;for(auto& pair:p.damage)pair.fill(0);}
    g.floatPool.fill(0);g.monarch=-1;g.initiative=-1;g.turn=g.config.startTurn;g.history.clear();
    if(previous==g)return false;
    if(past.size()==64)past.erase(past.begin());
    past.push_back(previous);future.clear();batchAvailable=false;touch();return true;
}
bool Model::setLife(int player,int value){
    if(value<MinLife||value>MaxLife||player<0||player>=state.game.config.players||!state.game.active)return false;
    endGesture();batchAvailable=false;
    bool changed=change(player,Counter::Life,value-state.game.players[player].life);
    batchAvailable=false;return changed;
}
bool Model::changeFloat(int index,int delta){
    if(!state.game.active||index<0||index>=7)return false;
    int next=static_cast<int>(std::clamp(static_cast<int64_t>(state.game.floatPool[index])+delta,int64_t(0),int64_t(MaxCounter)));
    if(next==state.game.floatPool[index])return false;
    endGesture();beginGesture();state.game.floatPool[index]=next;touch();endGesture();return true;
}
bool Model::resetFloat(){
    if(!state.game.active||std::all_of(state.game.floatPool.begin(),state.game.floatPool.end(),[](int n){return n==0;}))return false;
    endGesture();beginGesture();state.game.floatPool.fill(0);touch();endGesture();return true;
}
bool Model::change(int i,Counter kind,int delta,int source,int slot) {
    auto& g=state.game;
    if(!g.active || i<0 || i>=g.config.players) return false;
    auto& p=g.players[i]; int* value=nullptr; int low=0,high=MaxCounter;
    switch(kind) {
    case Counter::Life: value=&p.life; low=MinLife; high=MaxLife; break;
    case Counter::Poison: value=&p.poison; break;
    case Counter::Energy: value=&p.energy; break;
    case Counter::Experience: value=&p.experience; break;
    case Counter::Turn: value=&g.turn;high=9999;break;
    case Counter::Commander:
        if(!g.config.commander || source<0 || source>=g.config.players || slot<0 || slot>(g.config.partners?1:0)) return false;
        value=&p.damage[source][slot]; break;
    }
    int next=static_cast<int>(std::clamp(static_cast<int64_t>(*value)+delta,static_cast<int64_t>(low),static_cast<int64_t>(high)));
    if(next==*value) return false;
    bool single=!gesture; beginGesture();if(!gestureChanged)gestureFirstMs=inputMs;gestureLastMs=inputMs;gestureChanged=true;gestureKind=kind; int applied=next-*value; *value=next; actor=i;
    if(kind==Counter::Commander){damageSource=source;damageSlot=slot;}
    if(kind==Counter::Commander && g.config.linkLife) p.life=std::clamp(p.life-applied,MinLife,MaxLife);
    int first=(i/g.config.heads)*g.config.heads;
    for(int k=first;k<first+g.config.heads;++k){g.players[k].life=p.life;g.players[k].poison=p.poison;}
    touch(); if(single) endGesture(); return true;
}
bool Model::marker(int i,bool initiative) {
    if(!state.game.active || i<0 || i>=state.game.config.players) return false;
    endGesture(); beginGesture(); int& value=initiative?state.game.initiative:state.game.monarch;
    value=value==i?-1:i; touch(); endGesture(); return true;
}
bool Model::undo() { endGesture();batchAvailable=false; if(past.empty()) return false;auto current=state.game;future.push_back(current);state.game=past.back();state.game.elapsed=current.elapsed;state.game.config=current.config;state.game.timerRemaining=current.timerRemaining;state.game.timerRunning=current.timerRunning;state.game.timerDeadline=current.timerDeadline;past.pop_back();touch();return true; }
bool Model::redo() { endGesture();batchAvailable=false; if(future.empty()) return false;auto current=state.game;past.push_back(current);state.game=future.back();state.game.elapsed=current.elapsed;state.game.config=current.config;state.game.timerRemaining=current.timerRemaining;state.game.timerRunning=current.timerRunning;state.game.timerDeadline=current.timerDeadline;future.pop_back();touch();return true; }
bool Model::assign(int seat,int profile) {
    if(seat<0 || seat>=6 || profile<-1 || profile>=static_cast<int>(state.profiles.size())) return false;
    int old=state.seats[seat];
    if(profile>=0)for(int i=0;i<6;++i) if(state.seats[i]==profile) state.seats[i]=old;
    state.seats[seat]=profile; touch(); return true;
}
bool Model::assignActive(int seat,int profile) {
    if(!state.game.active||seat<0||seat>=state.game.config.players||profile<-1||profile>=static_cast<int>(state.profiles.size()))return false;
    endGesture();batchAvailable=false;
    auto& player=state.game.players[seat];
    player.profileIndex=profile;player.profileUsed=profile>=0;
    player.name=profile<0?defaultPlayerName(seat):state.profiles[profile].name;
    player.color=profile<0?seat:state.profiles[profile].color;
    // Identity is a game setting. Earlier life/counter snapshots keep the new identity.
    for(auto* stack:{&past,&future})for(auto& snapshot:*stack){auto& p=snapshot.players[seat];p.name=player.name;p.color=player.color;p.profileUsed=player.profileUsed;p.profileIndex=player.profileIndex;}
    touch();return true;
}
bool Model::addProfile(const std::string& name) {
    auto n=cleanName(name); if(n.empty() || state.profiles.size()>=MaxProfiles) return false;
    state.profiles.push_back({n,(static_cast<int>(state.profiles.size())-NamedProfileStart)%10}); touch(); return true;
}
bool Model::renameProfile(int i,const std::string& name) {
    auto n=cleanName(name); if(n.empty() || i<0 || i>=static_cast<int>(state.profiles.size())) return false;
    endGesture();state.profiles[i].name=n;refreshProfile(i);touch();return true;
}
void Model::refreshProfile(int i){
    const auto& profile=state.profiles[i];
    auto update=[&](Game& g){for(auto& p:g.players)if(p.profileIndex==i){p.name=profile.name;p.color=profile.color;}};
    update(state.game);for(auto* stack:{&past,&future})for(auto& g:*stack)update(g);
}
bool Model::colorProfile(int i,int color){
    if(i<0||i>=static_cast<int>(state.profiles.size())||color<0||color>=10)return false;
    endGesture();state.profiles[i].color=color;refreshProfile(i);touch();return true;
}
bool Model::deleteProfile(int i) {
    // The first six profiles provide stable reusable fallback seats.
    if(i<6 || i>=static_cast<int>(state.profiles.size())) return false;
    endGesture();state.profiles.erase(state.profiles.begin()+i);
    auto detach=[&](Game& g){for(auto& p:g.players){if(p.profileIndex==i)p.profileIndex=-1;else if(p.profileIndex>i)--p.profileIndex;}};
    detach(state.game);for(auto* stack:{&past,&future})for(auto& g:*stack)detach(g);
    for(auto& x:state.seats) { if(x==i) x=-1; else if(x>i) --x; }
    touch(); return true;
}
bool Model::savePreset(const std::string& name) {
    auto n=cleanName(name); if(n.empty() || state.presets.size()>=MaxPresets || !validConfig(state.setup)) return false;
    state.presets.push_back({n,state.setup}); touch(); return true;
}
bool Model::deletePreset(int i) { if(i<0 || i>=static_cast<int>(state.presets.size())) return false; state.presets.erase(state.presets.begin()+i); touch(); return true; }
bool Model::atThreshold(int i) const {
    if(i<0 || i>=state.game.config.players) return false;
    const auto& p=state.game.players[i];
    if(p.life<=0 || p.poison>=poisonLimit()) return true;
    const auto& g=state.game;
    int first=(i/g.config.heads)*g.config.heads;
    // Commander totals remain individual. One teammate losing defeats the team.
    if(g.config.commander)for(int member=first;member<first+g.config.heads;++member)
        for(int j=0;j<g.config.players;++j)for(int k=0;k<(g.config.partners?2:1);++k)
            if(g.players[member].damage[j][k]>=21)return true;
    return false;
}
}
