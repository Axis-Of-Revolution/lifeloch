#include "ui.hpp"
#include "pixel.hpp"
#include "storage.hpp"
#include "layout.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <iostream>
using namespace deck;
struct Capture:Renderer {
    struct Word{std::string text;float x,y;Color color;int screen;};
    int which=0,objects=0;std::vector<Word> words;std::vector<Rect> rectangles;
    void screen(int n)override{which=n;}
    void box(Rect r,Color,float=0)override{assert(r.x>=-.01&&r.y>=-.01&&r.w>=0&&r.h>=0);assert(r.x+r.w<=(which?320:400)+.01);assert(r.y+r.h<=240.01);++objects;rectangles.push_back(r);}
    void gradient(Rect r,Color,Color,Color,Color)override{box(r,0);}
    void text(const std::string& s,float x,float y,float,Color c,Align,float)override{words.push_back({s,x,y,c,which});}
    bool has(const std::string& s)const{return std::any_of(words.begin(),words.end(),[&](auto w){return w.text==s;});}
};
struct Raster:Renderer {
    std::map<std::pair<int,int>,Color> pixels;
    void screen(int)override{}
    void text(const std::string&,float,float,float,Color,Align,float)override{}
    void box(Rect r,Color c,float=0)override{for(int y=int(std::floor(r.y));y<int(std::ceil(r.y+r.h-.0001f));++y)for(int x=int(std::floor(r.x));x<int(std::ceil(r.x+r.w-.0001f));++x)pixels[{x,y}]=c;}
};
uint32_t crc(const std::vector<uint8_t>& b){uint32_t c=0xffffffff;for(auto x:b){c^=x;for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u&(0u-(c&1)));}return ~c;}
void put(std::vector<uint8_t>& b,int at,uint32_t n){for(int i=0;i<4;++i)b[at+i]=(n>>(8*i))&255;}
bool button(const App& app,Action action){return std::any_of(app.buttons.begin(),app.buttons.end(),[&](const auto& b){return b.command.action==action;});}
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
    for(int n:{-9999,-6,0,6,9,10,20,21}){
        Raster glyph,styled,plain;
        pixelText(glyph,std::to_string(n),20+(n>=0&&n<10?13*28.f/22:0),20,28,0xffffff,Align::Left,200);
        lifeText(styled,n,20,20,28,0xffffff,Align::Left,200,1000,20,true);
        lifeText(plain,n,20,20,28,0xffffff,Align::Left,200,1000,20,false);
        assert(!glyph.pixels.empty());assert(plain.pixels==glyph.pixels);
        if(n<=0){assert(styled.pixels.size()>glyph.pixels.size());assert(styled.pixels!=plain.pixels);}
        else if(n<=20){assert(styled.pixels!=plain.pixels);}
        else assert(styled.pixels==plain.pixels);
    }
    Raster flameA,flameB;lifeText(flameA,18,20,20,28,0xffffff,Align::Left,200,1000,20);lifeText(flameB,18,20,20,28,0xffffff,Align::Left,200,1700,20);assert(flameA.pixels!=flameB.pixels);
    Raster skullA,skullB;lifeText(skullA,0,20,20,28,0xffffff,Align::Left,200,1000,20);lifeText(skullB,0,20,20,28,0xffffff,Align::Left,200,1700,20);assert(skullA.pixels!=skullB.pixels);
    Model m;m.start();for(int i=0;i<6;++i){assert(m.state.game.players[i].name==std::string("Player ")+char('A'+i));assert(playerAbbreviation(m.state.game.players[i],i)==std::string("P")+char('A'+i));}
    m.addProfile("Alan");m.assign(1,6);m.start();assert(m.state.game.players[1].profileUsed);assert(playerAbbreviation(m.state.game.players[1],1)=="AL");
    m.renameProfile(6,"Player A");m.start();assert(playerAbbreviation(m.state.game.players[1],1)=="PL");
    App app;app.model.state.setup.commander=true;app.model.state.setup.timerEnabled=true;app.model.start();app.page=Page::Game;
    auto& g=app.model.state.game;g.timerRemaining=1500;g.timerDeadline=1800001500;g.players[1].damage[0][0]=10;g.players[0].damage[1][0]=10;
    Capture board;app.render(board);assert(board.has("Commander damage"));assert(board.has("PB:10"));assert(std::any_of(board.words.begin(),board.words.end(),[](auto w){return w.text=="PB:10"&&w.color==playerColor(1);}));
    for(int i=0;i<20;++i){app.model.setInputTime(1000+i*3000);app.model.change(0,Counter::Life,-1);}Capture history;app.render(history);assert(std::any_of(history.words.begin(),history.words.end(),[](auto w){return w.screen==0&&w.text.find("→")!=std::string::npos&&w.y>=35;}));
    assert(!button(app,Action::QuickTurn));app.dispatch({Action::QuickTurn,1});assert(g.turn==0&&!g.config.turnEnabled);g.config.turnEnabled=true;
    for(int tab=0;tab<3;++tab){app.tab=tab;Capture view;app.render(view);assert(button(app,Action::QuickTurn));assert(button(app,Action::Float));assert(button(app,Action::Dice));}
    app.dispatch({Action::QuickTurn,1});assert(g.turn==1&&g.config.turnEnabled);app.dispatch({Action::QuickTurn,-1});assert(g.turn==0);
    app.dispatch({Action::Float});for(int i=0;i<7;++i)app.dispatch({Action::ChangeFloat,i,i+1});assert(g.floatPool[6]==7);auto pool=g.floatPool;app.selected=1;assert(g.floatPool==pool);
    auto remaining=g.timerRemaining;app.dispatch({Action::ResetFloat});assert(std::all_of(g.floatPool.begin(),g.floatPool.end(),[](int n){return n==0;}));assert(g.timerRemaining==remaining&&g.timerDeadline==1800001500);app.dispatch({Action::Undo});assert(g.floatPool==pool);
    assert(!app.model.changeFloat(-1,1));app.model.changeFloat(0,10000);assert(g.floatPool[0]==999);app.model.changeFloat(0,-10000);assert(g.floatPool[0]==0);
    app.dispatch({Action::Back});assert(app.page==Page::Game);app.dispatch({Action::Dice});app.dispatch({Action::Back});assert(app.page==Page::Game);
    app.selected=0;app.dispatch({Action::SetLife});assert(app.input.kind==InputKind::SetLife&&app.input.numeric);app.submitText("-6",true);assert(g.players[0].life==-6);assert(g.history.back().after==-6);app.dispatch({Action::Undo});assert(g.players[0].life==0);
    app.dispatch({Action::SetLife});app.submitText("123",false);assert(g.players[0].life==0);
    for(const char* invalid:{"-10000","100000","-","--2","5.5",""}){app.dispatch({Action::SetLife});app.submitText(invalid,true);assert(g.players[0].life==0);}app.notice.clear();
    app.dispatch({Action::SetLife});app.submitText("99999",true);assert(g.players[0].life==99999);
    app.model.state.setup={4,30,true,false,true,2};app.model.start();app.dispatch({Action::SetLife});app.submitText("-2",true);assert(g.players[0].life==-2&&g.players[1].life==-2&&g.players[2].life==30);
    g.config.commander=false;app.page=Page::Game;app.tab=1;Capture noCommander;app.render(noCommander);assert(app.tab==0&&!noCommander.has("Commander"));assert(!button(app,Action::Commander));
    auto encoded=encode(app.model.state);State decoded;uint32_t sequence;assert(decode(encoded,decoded,sequence));assert(encode(decoded)==encoded);
    for(int version:{1,2}){State old=app.model.state;old.game.players[0].name="Player One";old.game.players[1].name="Alan";old.seats.fill(-1);auto bytes=legacyBytes(encode(old),version);assert(decode(bytes,decoded,sequence));assert(decoded.game.players[0].name=="Player A"&&!decoded.game.players[0].profileUsed);assert(decoded.game.players[1].profileUsed&&playerAbbreviation(decoded.game.players[1],1)=="AL");}
    // Every layout including maximum counters, both commander slots, all rotations, and all pages fits.
    int maximum=0;
    for(int n=2;n<=6;++n)for(int layout=0;layout<layoutCount(n);++layout){app.model.state.setup={n,40,true,true,true};chooseLayout(app.model.state.setup,layout);app.model.start();for(int i=0;i<n;++i){g.players[i].poison=5;g.players[i].energy=2;g.players[i].experience=9;for(auto& damage:g.players[i].damage){damage[0]=10;damage[1]=4;}}g.monarch=0;g.initiative=0;
        for(auto page:{Page::Game,Page::Float,Page::History,Page::Dice}){app.page=page;Capture layoutView;app.render(layoutView);maximum=std::max(maximum,layoutView.objects);assert(layoutView.objects<32768);for(size_t a=0;a<app.buttons.size();++a)for(size_t b=a+1;b<app.buttons.size();++b){auto x=app.buttons[a].rect,y=app.buttons[b].rect;assert(std::min(x.x+x.w,y.x+y.w)<=std::max(x.x,y.x)||std::min(x.y+x.h,y.y+y.h)<=std::max(x.y,y.y));}}
    }
    std::cout<<"PASS: numeral-only flame/death masks, animation, letter/default versus profile initials, framed colored commander damage, full-height history, shared Float/undo/reset/save, quick turn/dice, signed life entry, team synchronization, v1/v2 migration, all layouts and disjoint touch targets. Max objects: "<<maximum<<"\n";
}
