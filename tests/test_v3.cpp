#include "ui.hpp"
#include "pixel.hpp"
#include "theme.hpp"
#include "storage.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
using namespace deck;
struct Capture:Renderer {
    int screenIndex=0,quads=0;std::vector<std::string> words[2];std::vector<Color> colors[2];
    void screen(int n)override{screenIndex=n;}
    void box(Rect,Color c,float=0)override{colors[screenIndex].push_back(c);}
    void gradient(Rect,Color,Color,Color,Color)override{++quads;}
    void text(const std::string& s,float,float,float,Color c,Align,float)override{words[screenIndex].push_back(s);colors[screenIndex].push_back(c);}
};
uint32_t crc(const std::vector<uint8_t>& b){uint32_t c=0xffffffff;for(auto x:b){c^=x;for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u&(0u-(c&1)));}return ~c;}
void put(std::vector<uint8_t>& b,int at,uint32_t n){for(int i=0;i<4;++i)b[at+i]=(n>>(8*i))&255;}
// Convert a current save into a real legacy layout, including the old
// config size, rather than merely relabeling a newer binary.
std::vector<uint8_t> legacyBytes(std::vector<uint8_t> b,int version){

    auto read=[&](size_t pos){int n=0;for(int k=0;k<4;++k)n|=int(b[pos+k])<<(k*8);return n;};
    size_t p=24;int profiles=read(p);p+=4;
    for(int i=0;i<profiles;++i){int bytes=read(p);p+=4+bytes+4;}
    int presets=read(p);p+=4;std::vector<size_t> removed;
    for(int i=0;i<presets;++i){int bytes=read(p);p+=4+bytes;removed.push_back(p+72);p+=76;}
    removed.push_back(p+72);p+=76; // setup config
    p+=24+4+4; // seats, legacy theme boolean, active
    removed.push_back(p+72);p+=76; // game config
    size_t q=p+8; // monarch / initiative, then six player records
    for(int i=0;i<6;++i){int bytes=read(q);q+=4+bytes+68;}
    q+=4;int events=read(q);b.resize(b.size()-24-events*4); // v7 links and v8 event kinds
    for(auto it=removed.rbegin();it!=removed.rend();++it)b.erase(b.begin()+*it,b.begin()+*it+4);
    if(version==1)b.resize(b.size()-56);
    if(version==2)b.resize(b.size()-52);
    put(b,8,version);put(b,16,b.size()-24);put(b,20,0);put(b,20,crc(b));return b;
}
int main(){
    App app;app.dispatch({Action::NewGame});assert(!app.model.state.setup.commander);app.model.start();app.page=Page::Game;
    assert(app.model.state.game.players[0].name=="Player A");assert(app.model.state.game.players[5].name=="Player F");
    app.model.savePreset("New preset");assert(!app.model.state.presets.back().config.commander);
    Capture blank;app.render(blank);for(auto c:{0xc48cf1u,0xffda58u,0xc8d0dcu})assert(std::find(blank.colors[0].begin(),blank.colors[0].end(),c)==blank.colors[0].end());
    auto& p=app.model.state.game.players[0];p.poison=2;p.energy=3;p.experience=4;app.model.state.game.monarch=0;app.model.state.game.initiative=0;
    Capture badges;app.render(badges);for(auto c:{0xc48cf1u,0xffda58u,0xc8d0dcu,0xffdf78u,0x9d6946u})assert(std::find(badges.colors[0].begin(),badges.colors[0].end(),c)!=badges.colors[0].end());
    for(int mask=1;mask<32;++mask){auto points=themePoints(mask);int last=-1;for(auto point:points){assert(point.color>last);last=point.color;assert(themeSample(mask,point.x,point.y)==magicColor(point.color));}
        if(points.size()==1)assert(themeSample(mask,.1,.9)==themeSample(mask,.9,.1));
        app.model.state.themeColors=mask;Capture c;app.render(c);assert(c.quads>0);auto bytes=encode(app.model.state);State loaded;uint32_t seq;assert(decode(bytes,loaded,seq));assert(loaded.themeColors==mask);
    }
    for(float aspect:{400.0f/240,320.0f/240}){auto triangle=themePoints(7,aspect);auto distance=[&](int a,int b){float dx=(triangle[a].x-triangle[b].x)*240*aspect,dy=(triangle[a].y-triangle[b].y)*240;return std::sqrt(dx*dx+dy*dy);};assert(std::abs(distance(0,1)-distance(1,2))<.01f);assert(std::abs(distance(0,2)-distance(1,2))<.01f);}
    app.dispatch({Action::Theme});for(int j=0;j<4;++j)app.dispatch({Action::ThemeColor,j});assert(app.model.state.themeColors==16);app.dispatch({Action::ThemeColor,4});assert(app.model.state.themeColors==16);
    // v1 saves omitted only the final theme field; verify real backwards decoding and CRC.
    State old=app.model.state;old.game.players[0].name="Player 1";auto legacy=legacyBytes(encode(old),1);State recovered;uint32_t sequence;assert(decode(legacy,recovered,sequence));assert(recovered.themeColors==16);assert(recovered.game.players[0].name=="Player A");
    app.page=Page::Game;app.tick(1000);
    for(int i=0;i<10;++i){app.tick(1000+i*100);app.dispatch({Action::Change,-1,int(Counter::Life)});auto sounds=app.takeSounds();assert(sounds.size()==1);assert(sounds[0]==(i%5==4?Sound::Impact:Sound::Tick));}
    app.dispatch({Action::Undo});assert(app.notice.empty());assert(app.takeSounds().empty());app.dispatch({Action::Redo});assert(app.notice.empty());
    app.tick(5000);app.dispatch({Action::Change,5,int(Counter::Life)});assert(app.takeSounds()==std::vector<Sound>{Sound::Heal});
    app.tick(7100);app.dispatch({Action::Change,1,int(Counter::Life)});assert(app.takeSounds()==std::vector<Sound>{Sound::Chime});
    for(int i=0;i<3;++i){app.tick(7200+i*100);app.dispatch({Action::Change,1,int(Counter::Life)});assert(app.takeSounds()==std::vector<Sound>{Sound::Chime});}
    app.tick(9400);app.dispatch({Action::Change,1,int(Counter::Life)});assert(app.takeSounds()==std::vector<Sound>{Sound::Chime}); // exactly two seconds resets threshold
    app.dispatch({Action::Change,-12,int(Counter::Life)});assert(app.takeSounds()==std::vector<Sound>({Sound::Impact,Sound::Impact}));
    app.model.state.game.history={{0,0,29,17,-1,0,0},{0,0,17,18,-1,0,0}};Capture recent;app.render(recent);assert(std::find(recent.words[0].begin(),recent.words[0].end(),"-12→17")!=recent.words[0].end());
    app.dispatch({Action::History});Capture history;app.render(history);for(const char* s:{"-12→17","+1→18"})assert(std::find(history.words[1].begin(),history.words[1].end(),s)!=history.words[1].end());
    app.dispatch({Action::SelectPlayer,0});Capture single;app.render(single);assert(std::find(single.words[1].begin(),single.words[1].end(),"-12→17")!=single.words[1].end());
    for(auto sound:{Sound::Tick,Sound::Impact,Sound::Chime,Sound::Heal}){auto pcm=soundPcm(sound);assert(pcm.size()>800);assert(pcm.front()==0&&pcm.back()==0);int peak=0;for(int n:pcm)peak=std::max(peak,std::abs(n));assert(peak>1000&&peak<30000);}
    assert(pixelCompatible("-12→17"));assert(!pixelCompatible("é"));
    std::cout<<"PASS: conditional badges, monarch/initiative pixels, spelled names, commander-off defaults, all 31 WUBRG themes, save migration, single-line histories, silent undo/redo, per-five audio thresholds and PCM integrity.\n";
}
