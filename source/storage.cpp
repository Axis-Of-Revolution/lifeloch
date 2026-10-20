#include "storage.hpp"
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

namespace deck {
namespace {
uint32_t crc(const uint8_t* p,size_t n) {
    uint32_t c=0xffffffff;
    while(n--) {c^=*p++; for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u & (0u-(c&1)));}
    return ~c;
}
struct Writer {
    std::vector<uint8_t> b;
    void number(int32_t n) {uint32_t v=static_cast<uint32_t>(n);for(int i=0;i<4;++i)b.push_back((v>>(8*i))&255);}
    void boolean(bool v) {number(v?1:0);}
    void string(const std::string& s) {number(static_cast<int>(s.size())); b.insert(b.end(),s.begin(),s.end());}
    void config(const Config& c) {number(c.players);number(c.life);boolean(c.commander);boolean(c.partners);boolean(c.linkLife);number(c.heads);boolean(c.turnEnabled);boolean(c.timerEnabled);boolean(c.clockEnabled);number(c.timerSeconds);number(c.layout);number(c.startTurn);for(int r:c.rotations)number(r);number(c.themeColors);}
};
struct Reader {
    const std::vector<uint8_t>& b; size_t pos=0; bool ok=true; int version=8;
    int32_t number() {if(pos+4>b.size()){ok=false;return 0;}uint32_t v=0;for(int i=0;i<4;++i)v|=uint32_t(b[pos++])<<(8*i);return static_cast<int32_t>(v);}
    bool boolean() {int n=number();if(n<0||n>1)ok=false;return n==1;}
    std::string string() {int n=number();if(n<0||n>64||pos+static_cast<size_t>(n)>b.size()){ok=false;return "";}std::string s(b.begin()+pos,b.begin()+pos+n);pos+=n;return s;}
    Config config() {Config c;c.players=number();c.life=number();c.commander=boolean();c.partners=boolean();c.linkLife=boolean();c.heads=number();c.turnEnabled=boolean();c.timerEnabled=boolean();c.clockEnabled=boolean();c.timerSeconds=number();c.layout=number();c.startTurn=number();for(int& x:c.rotations)x=number();if(version>=4)c.themeColors=number();if(version<5&&c.players==5){const int map[]={0,0,1,1,2,3};if(c.layout>=0&&c.layout<6)c.layout=map[c.layout];else ok=false;}return c;}
};
std::vector<uint8_t> read(const std::string& path,bool& exists) {
    FILE* f=fopen(path.c_str(),"rb");exists=f!=nullptr;if(!f)return {};
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return {};}
    long n=ftell(f);if(n<0||n>65536){fclose(f);return {};}
    rewind(f);std::vector<uint8_t> b(static_cast<size_t>(n));size_t got=fread(b.data(),1,b.size(),f);fclose(f);if(got!=b.size())return {};return b;
}
}
std::vector<uint8_t> encode(const State& s,uint32_t sequence) {
    if(!validState(s))return {};
    Writer p;p.number(static_cast<int>(s.profiles.size()));
    for(auto& x:s.profiles){p.string(x.name);p.number(x.color);}
    p.number(static_cast<int>(s.presets.size()));for(auto& x:s.presets){p.string(x.name);p.config(x.config);}
    p.config(s.setup);for(int n:s.seats)p.number(n);p.boolean(s.light);
    const auto& g=s.game;p.boolean(g.active);p.config(g.config);p.number(g.monarch);p.number(g.initiative);
    for(auto& x:g.players){p.string(x.name);p.number(x.color);p.number(x.life);p.number(x.poison);p.number(x.energy);p.number(x.experience);for(auto a:x.damage)for(int n:a)p.number(n);}
    p.number(g.elapsed);p.number(static_cast<int>(g.history.size()));for(auto& e:g.history){p.number(e.team);p.number(e.player);p.number(e.before);p.number(e.after);p.number(e.source);p.number(e.slot);p.number(e.seconds);}
    p.number(g.turn);p.number(g.timerRemaining);p.boolean(g.timerRunning);p.number(static_cast<uint32_t>(g.timerDeadline));p.number(static_cast<uint32_t>(g.timerDeadline>>32));
    Writer out;const char magic[8]={'C','D','3','D','S','A','V','E'};out.b.insert(out.b.end(),magic,magic+8);
    p.number(s.themeColors);
    for(const auto& player:g.players)p.boolean(player.profileUsed);
    for(int value:g.floatPool)p.number(value);
    for(const auto& player:g.players)p.number(player.profileIndex);
    for(const auto& event:g.history)p.number(static_cast<int>(event.kind));
    out.number(8);out.number(sequence);out.number(static_cast<int>(p.b.size()));out.number(0);out.b.insert(out.b.end(),p.b.begin(),p.b.end());
    uint32_t sum=crc(out.b.data(),out.b.size());for(int i=0;i<4;++i)out.b[20+i]=(sum>>(8*i))&255;return out.b;
}
bool decode(const std::vector<uint8_t>& b,State& s,uint32_t& sequence) {
    if(b.size()<24||b.size()>65536||memcmp(b.data(),"CD3DSAVE",8)!=0)return false;
    Reader r{b,8,true};int version=r.number();if(version<1||version>8)return false;r.version=version;uint32_t seq=r.number();int len=r.number();uint32_t sum=r.number();
    if(len<0||static_cast<size_t>(len)!=b.size()-24)return false;
    auto checked=b;for(int i=20;i<24;++i)checked[i]=0;if(crc(checked.data(),checked.size())!=sum)return false;
    State next;int count=r.number();if(count<6||count>MaxStoredProfiles)return false;
    for(int i=0;i<count;++i){Profile p;p.name=r.string();p.color=r.number();next.profiles.push_back(p);}
    count=r.number();if(count<0||count>MaxPresets)return false;
    for(int i=0;i<count;++i){Preset p;p.name=r.string();p.config=r.config();next.presets.push_back(p);}
    next.setup=r.config();for(auto& n:next.seats)n=r.number();next.light=r.boolean();
    auto& g=next.game;g.active=r.boolean();g.config=r.config();g.monarch=r.number();g.initiative=r.number();
    for(auto& p:g.players){p.name=r.string();p.color=r.number();p.life=r.number();p.poison=r.number();p.energy=r.number();p.experience=r.number();for(auto& a:p.damage)for(auto& n:a)n=r.number();}
    g.elapsed=static_cast<uint32_t>(r.number());count=r.number();if(count<0||count>256)return false;
    for(int i=0;i<count;++i){LifeEvent e;e.team=r.number();e.player=r.number();e.before=r.number();e.after=r.number();e.source=r.number();e.slot=r.number();e.seconds=r.number();g.history.push_back(e);}
    g.turn=r.number();g.timerRemaining=r.number();g.timerRunning=r.boolean();uint32_t low=r.number(),high=r.number();g.timerDeadline=static_cast<uint64_t>(low)|(static_cast<uint64_t>(high)<<32);
    if(version>=2)next.themeColors=r.number();
    else {next.themeColors=next.light?1:16;next.light=false;}
    if(version>=3){for(auto& player:g.players)player.profileUsed=r.boolean();for(int& value:g.floatPool)value=r.number();}
    else {
        const char* previous[]={"Player One","Player Two","Player Three","Player Four","Player Five","Player Six"};
        for(int i=0;i<MaxPlayers;++i){auto& player=g.players[i];int seat=next.seats[i];
            bool generic=player.name==previous[i]||player.name=="Player "+std::to_string(i+1)||player.name==defaultPlayerName(i);
            bool selected=seat>=0&&seat<static_cast<int>(next.profiles.size())&&next.profiles[seat].name==player.name;
            player.profileUsed=!generic||selected;
            if(!player.profileUsed&&g.active)player.name=defaultPlayerName(i);
        }
    }
    if(version<4){next.setup.themeColors=next.themeColors;g.config.themeColors=next.themeColors;for(auto& preset:next.presets)preset.config.themeColors=next.themeColors;}
    if(version<5){
        const char* generic[]={"Player One","Player Two","Player Three","Player Four","Player Five","Player Six"};
        for(int i=0;i<6&&i<static_cast<int>(next.profiles.size());++i){auto name=next.profiles[i].name;
            bool builtIn=name==defaultPlayerName(i)||name==generic[i]||name=="Player "+std::to_string(i+1);
            if(builtIn){for(int seat=0;seat<6;++seat)if(next.seats[seat]==i){next.seats[seat]=-1;if(g.players[seat].name==name){g.players[seat].profileUsed=false;g.players[seat].name=defaultPlayerName(seat);g.players[seat].color=seat;}}next.profiles[i]={defaultPlayerName(i),i};}
            else if(next.profiles.size()<MaxStoredProfiles){int promoted=static_cast<int>(next.profiles.size());next.profiles.push_back(next.profiles[i]);next.profiles[i]={defaultPlayerName(i),i};for(auto& seat:next.seats)if(seat==i)seat=promoted;}
        }
    }
    if(version<6){
        // Remove only unchanged factory presets; retain renamed/customized presets.
        next.presets.erase(std::remove_if(next.presets.begin(),next.presets.end(),[](const Preset& p){
            Config expected;bool known=true;
            if(p.name=="Commander")expected=Config{4,40,true,false,true};
            else if(p.name=="Two-player")expected=Config{2,20,false,false,true};
            else if(p.name=="Two-headed Giant")expected=Config{4,30,false,false,true,2};
            else if(p.name=="Three-head Giant")expected=Config{6,45,false,false,true,3};
            else known=false;
            expected.themeColors=16;Config actual=p.config;actual.rotations=expected.rotations;
            return known&&actual==expected;
        }),next.presets.end());
    }
    if(version>=7){for(auto& p:g.players)p.profileIndex=r.number();}
    else for(int i=0;i<MaxPlayers;++i){auto& p=g.players[i];if(!p.profileUsed)continue;
        // Recover an old snapshot only when its source can be identified without guessing.
        int match=-1;for(int j=NamedProfileStart;j<static_cast<int>(next.profiles.size());++j)if(next.profiles[j].name==p.name){if(match!=-1){match=-2;break;}match=j;}
        if(match>=0){p.profileIndex=match;p.color=next.profiles[match].color;}
    }
    if(version>=8)for(auto& e:g.history)e.kind=static_cast<HistoryKind>(r.number());
    if(!r.ok||r.pos!=b.size()||!validState(next))return false;
    s=next;sequence=seq;return true;
}
LoadResult Storage::load(State& s) {
    State a,b;uint32_t sa=0,sb=0;bool ea=false,eb=false;
    bool va=decode(read(dir+"/save-a.bin",ea),a,sa),vb=decode(read(dir+"/save-b.bin",eb),b,sb);
    if(!va&&!vb){blocked=ea||eb;return blocked?LoadResult::Unreadable:LoadResult::Missing;}
    bool useB=vb&&(!va||static_cast<int32_t>(sb-sa)>0);s=useB?b:a;sequence=useB?sb:sa;nextSlot=useB?0:1;
    blocked=false;return (ea&&!va)||(eb&&!vb)?LoadResult::Recovered:LoadResult::Loaded;
}
bool Storage::save(const State& s) {
    if(blocked)return false;
    auto b=encode(s,sequence+1);if(b.empty())return false;
    std::string target=dir+(nextSlot?"/save-b.bin":"/save-a.bin"),temp=target+".tmp";
    FILE* f=fopen(temp.c_str(),"wb");if(!f)return false;
    bool ok=fwrite(b.data(),1,b.size(),f)==b.size();if(fflush(f)!=0)ok=false;
    // newlib SD devoptab supports fsync; do not destroy the other valid slot on failure.
    if(fsync(fileno(f))!=0)ok=false;
    if(fclose(f)!=0)ok=false;
    if(!ok){remove(temp.c_str());return false;}
    // FAT rename cannot always replace a destination. Only the older slot is replaced.
    remove(target.c_str());if(rename(temp.c_str(),target.c_str())!=0)return false;
    ++sequence;nextSlot=1-nextSlot;return true;
}
}
