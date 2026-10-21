#include "ui.hpp"
#include "pixel.hpp"
#include "layout.hpp"
#include "theme.hpp"
#include "dice.hpp"
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace deck {
namespace {
std::string number(int n){return std::to_string(n);}
std::string signedNumber(int n){return (n>0?"+":"")+number(n);}

}
Color playerColor(int n,bool){
    static constexpr Color colors[]={0x79a7eb,0xdf816b,0x78b58b,0xe9dfbc,0x81738e,0xd3dae2};return colors[std::clamp(n,0,5)];
}
std::string colorPairName(int pair){const char* pairs[]={"WU","WB","WR","WG","UB","UR","UG","BR","BG","RG"};return pairs[std::clamp(pair,0,9)];}
std::array<Color,2> playerColors(const Player& player,bool ink){
    static constexpr Color colors[]={0xe9dfbc,0x79a7eb,0x29252f,0xdf816b,0x78b58b};
    static constexpr int pairs[][2]={{0,1},{0,2},{0,3},{0,4},{1,2},{1,3},{1,4},{2,3},{2,4},{3,4}};
    if(!player.profileUsed){int id=std::clamp(player.color,0,5);Color c=playerColor(id);return {c,c};}
    int id=std::clamp(player.color,0,9);auto color=[&](int c){return c==2&&ink?0xa89eaf:colors[c];};return {color(pairs[id][0]),color(pairs[id][1])};
}
namespace {
Color colorMix(Color a,Color b,float t){Color out=0;for(int sh:{16,8,0})out|=static_cast<Color>(((a>>sh)&255)*(1-t)+((b>>sh)&255)*t)<<sh;return out;}
struct GradientInk:Renderer{
    Renderer& target;Color left,right;float start,width;
    GradientInk(Renderer&t,Color l,Color r,float s,float w):target(t),left(l),right(r),start(s),width(w){}
    void screen(int)override{}void text(const std::string&,float,float,float,Color,Align,float)override{}
    void box(Rect r,Color,float radius=0)override{target.box(r,colorMix(left,right,std::clamp((r.x+r.w/2-start)/std::max(1.0f,width),0.0f,1.0f)),radius);}
};
Player asProfile(const Profile& p){Player out;out.name=p.name;out.color=p.color;out.profileUsed=true;return out;}
}
void App::identityFill(Rect area,const Player& player,bool subdued){auto c=playerColors(player);if(subdued)for(auto&v:c)v=colorMix(0x101b20,v,.38f);
    if(c[0]==c[1]){draw->box(area,c[0]);return;}for(float x=0;x<area.w;x+=1)draw->box({area.x+x,area.y,std::min(1.0f,area.w-x),area.h},colorMix(c[0],c[1],x/std::max(1.0f,area.w-1)));
}
void App::identityText(const std::string& text,float x,float y,float size,const Player& player,Align align,float width){auto c=playerColors(player,true);
    if(c[0]==c[1]){label(text,x,y,size,c[0],align,width);return;}
    float span=std::min(width>0?width:400.0f,pixelAdvance(text)*size/22),start=x+(align==Align::Center?-span/2:align==Align::Right?-span:0);
    if(!pixelCompatible(text)){label(text,x,y,size,p.ink,align,width);identityFill({start,y+size*.72f,span,1},player);return;}
    GradientInk ink(*draw,c[0],c[1],start,span);pixelText(ink,text,x,y,size,c[0],align,width);
}
void App::identityShadowText(const std::string& text,float x,float y,float size,const Player& player,Align align,float width){
    // Offset glyph silhouette, not an outline or black backing rectangle.
    // Preserve the original solid/gradient ink on top.
    label(text,x+.95f,y+1.20f,size,0x081417,align,width);
    identityText(text,x,y,size,player,align,width);
}
std::string timeText(uint32_t t){char b[24];if(t>=3600)snprintf(b,sizeof(b),"%u:%02u:%02u",static_cast<unsigned>(t/3600),static_cast<unsigned>((t/60)%60),static_cast<unsigned>(t%60));else snprintf(b,sizeof(b),"%02u:%02u",static_cast<unsigned>(t/60),static_cast<unsigned>(t%60));return b;}
void App::label(const std::string& s,float x,float y,float size,Color color,Align a,float width){
    if(width<=0)width=a==Align::Left?viewport-x-8:(a==Align::Right?x-8:2*std::min(x-8,viewport-x-8));
    draw->text(s,x,y,size,color?color:p.ink,a,width);
}
void App::panel(Rect r,Color c){draw->box(r,c?c:p.panel,2);}
void App::button(Rect r,const std::string& s,Command cmd,bool active,bool enabled,bool repeating,float font){
    bool pressed=enabled&&((holding&&cmd==held)||(page==feedbackPage&&cmd==feedback&&now<feedbackUntil));
    Color fill=pressed?0x59736a:p.raised;
    if(active||pressed)draw->box(r,pressed?0xffefbb:p.accent,2);
    float inset=pressed?2:1;
    draw->box(active||pressed?Rect{r.x+inset,r.y+inset,r.w-2*inset,r.h-2*inset}:r,fill,2);
    label(s,r.x+r.w/2,r.y+(r.h-font)/2-1,font,enabled?p.ink:p.muted,Align::Center,r.w-10);
    buttons.push_back({r,cmd,s,enabled,repeating});
}
void App::header(const std::string& title,bool goBack){label(title,12,9,18,0,Align::Left,goBack?244:296);if(goBack)button({264,5,44,28},"Back",{Action::Back},false,true,false,11);draw->box({12,35,296,1},p.line);}
void App::message(const std::string& s){notice=s;noticeUntil=lastClock+2400;}
void App::request(InputKind k,const std::string& title,const std::string& initial,bool numeric){pointerUp();input={k,title,initial,numeric};}
void App::confirm(Command c,const std::string& title,const std::string& body){pointerUp();confirming=c;modalTitle=title;modalBody=body;}
float App::displayedLife(int player)const{
    const auto& motion=lifeMotion[player];if(!motion.seen||motion.duration<=0)return model.state.game.players[player].life;
    float t=std::clamp(float(now-motion.start)/motion.duration,0.f,1.f);float ease=1-std::pow(1-t,3);return motion.from+(motion.target-motion.from)*ease;
}
void App::syncLifeMotion(){
    if(!model.state.game.active)return;
    for(int i=0;i<model.state.game.config.players;++i){auto& motion=lifeMotion[i];int target=model.state.game.players[i].life;
        if(!motion.seen){motion={true,target,float(target),now,0};}
        else if(motion.target!=target){float from=displayedLife(i);motion={true,target,from,now,std::min(360.f,200+std::abs(target-from)*9)};}
    }
}
void App::render(Renderer& r){
    syncLifeMotion();
    bool configuring=page==Page::Rules||page==Page::Seats||(page==Page::Layout&&!editingGameLayout)||page==Page::Options||page==Page::Presets||(page==Page::Theme&&themeReturn==Page::Rules);
    ThemeRenderer themed(r,configuring?model.state.setup.themeColors:model.state.themeColors,now,model.state.themeSpeedStep);
    draw=&themed;buttons.clear();
    p=Palette{0x101b1c,0x19292a,0x263a3b,0xf0f2df,0xa4b6af,0x3b5350,0xc5dc9b,0x17251e,0x82d8a2,0xfa9d8d};
    viewport=400;themed.screen(0);themed.box({0,0,400,240},p.bg);top();
    viewport=320;themed.screen(1);themed.box({0,0,320,240},p.bg);bottom();
    if(confirming.action!=Action::None){
        buttons.clear();panel({8,50,304,166},p.raised);label(modalTitle,24,66,18,0,Align::Left,272);
        size_t split=modalBody.find('\n');label(modalBody.substr(0,split),24,102,12,0,Align::Left,272);
        if(split!=std::string::npos)label(modalBody.substr(split+1),24,121,12,0,Align::Left,272);
        button({24,166,128,34},"Cancel",{Action::Cancel});button({168,166,128,34},"Confirm",{Action::Confirm},true);
    }
    syncFocus();drawFocus();
    if(!notice.empty() && confirming.action==Action::None){panel({12,78,296,36},p.accent);label(notice,160,88,12,p.accentInk,Align::Center,276);}
}
void App::top(){
    if(page==Page::Dice){
        label("DICE TRAY",200,7,16,0xf1e5c5,Align::Center,376);
        auto pose=rollSimulation.pose();drawDiceTray(*draw,{8,30,384,187},dieSides,hasRollResult?&pose:nullptr);
        std::string result=rolling?"ROLLING":!hasRollResult?"Choose a die or coin, then roll":dieResult==0?(dieSides==2?"ON EDGE / FLIP AGAIN":"COCKED / ROLL AGAIN"):dieSides==2?(dieResult==1?"HEADS":"TAILS"):"RESULT "+number(dieResult);
        label(result,200,224,11,0xf1e5c5,Align::Center,376);return;
    }

    bool setupTheme=page==Page::Theme&&themeReturn==Page::Rules;
    bool live=!setupTheme&&model.state.game.active&&(page==Page::Game||page==Page::History||page==Page::Menu||page==Page::Help||page==Page::Dice||page==Page::Clock||page==Page::Theme||page==Page::Float||page==Page::GameTrackers||page==Page::ActiveSeats||(page==Page::Layout&&editingGameLayout)||(assigningLiveProfiles&&(page==Page::Profiles||page==Page::Profile)));
    bool preview=setupTheme||page==Page::Rules||page==Page::Seats||page==Page::Presets||(page==Page::Layout&&!editingGameLayout)||page==Page::Options||(pickSeat>=0&&!assigningLiveProfiles&&(page==Page::Profiles||page==Page::Profile));
    if(live){board(model.state.game);return;}
    if(preview){Game g;g.config=model.state.setup;g.active=true;g.turn=g.config.startTurn;g.timerRemaining=g.config.timerSeconds;g.timerRunning=g.config.timerEnabled;for(int i=0;i<6;++i){int seat=model.state.seats[i];g.players[i].profileUsed=seat>=0;g.players[i].name=seat<0?defaultPlayerName(i):model.state.profiles[seat].name;g.players[i].color=seat<0?i:model.state.profiles[seat].color;g.players[i].life=g.config.life;}board(g,true);return;}
    draw->homeArtwork({0,0,400,240});
    label("LIFELOCH",201,15,34,0x142027,Align::Center,370);
    label("LIFELOCH",200,13,34,0xf1e5c5,Align::Center,370);
    label("A Magic: The Gathering Game Tracker",201,49,12,0x142027,Align::Center,376);
    label("A Magic: The Gathering Game Tracker",200,48,12,0xf1e5c5,Align::Center,376);
    if(!saveOk)label("SAVE UNAVAILABLE",376,213,9,p.bad,Align::Right);
}
void App::miniHistory(int team,float x,float y,float width,int count,float spacing,float font){
    int shown=0;const auto& history=model.state.game.history;
    for(auto it=history.rbegin();it!=history.rend()&&shown<count;++it)if(it->team==team&&(it->kind==HistoryKind::Life||it->kind==HistoryKind::Poison)){
        auto text=(it->kind==HistoryKind::Poison?"P ":"")+signedNumber(it->after-it->before)+"→"+number(it->after);Color ink=it->kind==HistoryKind::Poison?0xc48cf1:it->after>=it->before?p.good:p.bad;
        label(text,x,y+shown*spacing,font,ink,Align::Left,width);
        if(shown==0)draw->box({x,y+font*.72f,std::min(width,pixelAdvance(text)*font/22),1},ink);
        ++shown;
    }
    if(shown==0)label("--",x+width/2,y+8,11,p.muted,Align::Center);
}
void App::damageRow(const Game& g,int who,Rect area){
    if(!g.config.commander)return;
    draw->box(area,0x607c80);draw->box({area.x+1,area.y+1,area.w-2,area.h-2},p.raised);
    std::vector<int> sources;
    for(int j=0;j<g.config.players;++j)if(g.players[who].damage[j][0]||(g.config.partners&&g.players[who].damage[j][1]))sources.push_back(j);
    std::stable_sort(sources.begin(),sources.end(),[&](int a,int b){const auto& d=g.players[who].damage;return d[a][0]+(g.config.partners?d[a][1]:0)>d[b][0]+(g.config.partners?d[b][1]:0);});
    // Fixed 2x2 grid: exactly two text rows and two columns regardless of
    // seat width. Scale the font uniformly to the longest of the top 4 entries.
    auto entry=[&](int j){const auto& d=g.players[who].damage[j];std::string t=playerAbbreviation(g.players[j],j)+":"+number(d[0]);if(g.config.partners)t+="/"+number(d[1]);return t;};
    constexpr float inset=4.f;
    float cell=(area.w-2*inset)/2.f, row=(area.h-4)/2.f;
    float font=std::min(9.f,row*.90f);
    // Reserve equal gutters around both centered cells and between columns.
    for(size_t i=0;i<std::min<size_t>(sources.size(),4);++i){
        float width=pixelAdvance(entry(sources[i]));
        if(width>0)font=std::min(font,std::max(2.f,cell-6.f)*22.f/width);
    }
    font=std::max(2.f,font);
    if(sources.empty()){
        label("--",area.x+inset+cell*.5f,area.y+2+row*.5f-font*.45f,9,p.muted,Align::Center,cell-6);
        return;
    }
    for(size_t index=0;index<std::min<size_t>(sources.size(),4);++index){
        int j=sources[index];
        float cx=area.x+inset+(index%2+.5f)*cell;
        float cy=area.y+2+(index/2+.5f)*row-font*.45f;
        identityText(entry(j),cx,cy,font,g.players[j],Align::Center,cell-6);
    }
}
void App::board(const Game& g,bool preview){
    label(preview?"PREVIEW":"LIFELOCH",6,4,9,p.accent);
    if(g.config.clockEnabled)label(currentClock,200,4,9,p.ink,Align::Center,75);
    std::string format=g.config.heads==1?number(g.config.players)+" PLAYERS":number(g.config.heads)+"-HEADED GIANT";
    label(format,394,4,9,p.muted,Align::Right,130);
    auto areas=playerPanels(g.config.players,g.config.layout);
    Config oriented=g.config;chooseLayout(oriented,g.config.layout);
    Renderer* original=draw;
    for(int i=0;i<g.config.players;++i){
        PanelRenderer card(*original,areas[i],oriented.rotations[i]);draw=&card;float w=card.width(),h=card.height();viewport=static_cast<int>(w);
        bool compact=h<120;draw->box({0,0,w,h},0x6d8580,2);panel({1,1,w-2,h-2});
        if(!preview&&i==selected)draw->box({0,5,2,h-10},p.accent,1);
        // Four central seats in side sections use standard geometry. Only
        // the physically small, rotated side seats move life below their name.
        bool sideSections=g.config.players==6&&g.config.layout==1&&areas[i].w<=76.5f;
        float margin=6.f, nameLimit=w*.5f, nameX=22.f;
        identityFill({5,6,14,12},g.players[i]);
        auto abbreviation=playerAbbreviation(g.players[i],i);
        // Badge initials use a displaced shadow, never a stroke outline.
        label(abbreviation,12+.85f,8+1.f,7,0x081417,Align::Center,13);
        label(abbreviation,12,8,7,0xfff4da,Align::Center,13);
        // Existing long profiles retain their saved names. Only the first
        // eight UTF-8 code points appear on the board, without cutting bytes.
        std::string title;int nameCodepoints=0;for(size_t at=0;at<g.players[i].name.size()&&title.size()<64;){
            unsigned char ch=g.players[i].name[at];size_t bytes=ch<128?1:ch<224?2:ch<240?3:4;
            if(title.size()+bytes>64)break;
            title+=g.players[i].name.substr(at,bytes);at+=bytes;
            if(++nameCodepoints>=8)break;
        }
        float titleSpace=sideSections?w-margin-nameX:nameLimit-nameX-3;
        titleSpace=std::max(7.f,titleSpace);
        float titleFont=std::min(10.f,titleSpace*22.f/std::max(1.f,pixelAdvance(title)));
        if(sideSections)titleFont=std::max(8.f,titleFont); // legibility wins on compact side seats
        identityShadowText(title,nameX,8,titleFont,g.players[i],Align::Left,titleSpace);
        const int values[]={g.players[i].poison,g.players[i].energy,g.players[i].experience};
        const char* codes[]={"P:","E:","X:"};const Color colors[]={0xc48cf1,0xffda58,0xc8d0dc};
        int badges=0;for(int v:values)if(v)++badges;
        bool roles=g.monarch==i||g.initiative==i;if(roles)++badges;
        float trackerX=w/3.f+2.f;
        float trackerW=w-margin-trackerX;
        float commandH=g.config.commander?std::clamp(h*.23f,20.f,25.f):0.f;
        float commandY=h-5-commandH;
        float badgeY=(g.config.commander?commandY:h-5)-10;
        float lifeY=sideSections?24.f:5.f;
        float lifeBottom=(badges?badgeY:(g.config.commander?commandY:h-5))-1;
        float lifeH=std::max(1.f,lifeBottom-lifeY);
        float lifeW=w/2.f-7.f;
        float lifeX=w-margin;
        float shown=preview?g.players[i].life:displayedLife(i);
        float digits=std::max({pixelAdvance("00"),pixelAdvance(number(g.players[i].life)),pixelAdvance(number(static_cast<int>(std::round(shown))))});
        float lifeSize=std::min(lifeH*22.f/16.f,lifeW*22.f/digits);
        float historyX=margin;
        float historyW=w/3.f-12.f;
        float historyFont=compact?9.25f:11.f,spacing=compact?11.25f:14.f;
        float historyBottom=h-8;
        int count=std::max(0,1+static_cast<int>(std::floor((historyBottom-24-6)/spacing)));
        if(!preview&&count)miniHistory(i/g.config.heads,historyX,24,historyW,count,spacing,historyFont);
        if(g.config.commander)damageRow(g,i,Rect{trackerX,commandY,trackerW,commandH});
        lifeText(*draw,g.players[i].life,lifeX,lifeY,lifeSize,(!preview&&model.atThreshold(i))?p.bad:p.ink,Align::Right,lifeW,now,g.config.life/2,!preview,!preview&&model.atThreshold(i),shown,!preview&&lifeMotion[i].from>lifeMotion[i].target,g.config.life);
        if(badges){
            float badgeW=trackerW/badges;int at=0;
            for(int j=0;j<3;++j)if(values[j]){
                std::string t=std::string(codes[j])+number(values[j]);
                float f=std::min(9.0f,(badgeW-2)*22.f/std::max(1.f,pixelAdvance(t)));
                label(t,trackerX+(at+1)*badgeW-2,badgeY,f,colors[j],Align::Right,badgeW-3);++at;
            }
            if(roles){float mw=std::min(11.0f,(badgeW-3)/2);
                float right=trackerX+(at+1)*badgeW-2;
                if(g.initiative==i)drawMarker(*draw,{right-mw,badgeY,mw,9},true);
                if(g.monarch==i)drawMarker(*draw,{right-mw-(g.initiative==i?mw+2:0),badgeY,mw,9},false);
            }
        }
        draw=original;viewport=400;
    }
    auto dividers=teamDividers(g.config.players,g.config.layout,g.config.heads);
    for(auto line:dividers)draw->box({line.x-.6f,line.y-.6f,line.w+1.2f,line.h+1.2f},0x102024);
    for(auto line:dividers)draw->box(line,0xe5d6b5);
    std::string foot;
    if(!saveOk)foot=saveBlocked?"SAVE UNREADABLE":"SAVE FAILED - CHECK SD";
    label(foot,6,230,7,!saveOk?p.bad:p.muted,Align::Left,190);
    if(g.config.turnEnabled)label("TURN "+number(g.turn),257,229,8,p.accent,Align::Right,69);
    if(g.config.timerEnabled){std::string timer=preview?timeText(g.config.timerSeconds):timeText(g.timerRemaining);label(timer+(g.timerRunning?"":" PAUSED"),394,228,10,g.timerRemaining==0&&!preview?p.bad:p.accent,Align::Right,116);}
}
void App::playerStrip(){
    const auto& g=model.state.game;int n=g.config.players;float w=(296.0f-(n-1)*4)/n;
    for(int i=0;i<n;++i){Rect area{12+i*(w+4),30,w,24};button(area,"",{Action::SelectPlayer,i},i==selected,true,false,11);identityFill({area.x+2,area.y+2,area.w-4,area.h-4},g.players[i],true);identityFill({area.x+2,area.y+20,area.w-4,2},g.players[i]);label(playerAbbreviation(g.players[i],i),area.x+w/2+1,37.15f,11,0x081417,Align::Center,w-6);
        label(playerAbbreviation(g.players[i],i),area.x+w/2,36,11,0xfff4da,Align::Center,w-6);buttons.back().label=g.players[i].name;}
    if(g.config.heads>1)for(int first=0;first<n;first+=g.config.heads){float x=10.5f+first*(w+4),width=g.config.heads*w+(g.config.heads-1)*4+3;Color border=0xe5d6b5;
        draw->box({x,28.5f,width,1},border);draw->box({x,54.5f,width,1},border);draw->box({x,28.5f,1,27},border);draw->box({x+width-1,28.5f,1,27},border);
    }
}
void App::quickControls(){
    button({12,179,94,25},"Dice",{Action::Dice},false,true,false,11);
    button({113,179,94,25},"History",{Action::History},false,true,false,11);
    button({214,179,94,25},"Float",{Action::Float},false,true,false,11);
}
void App::footer(){
    const auto& g=model.state.game;
    button({12,211,g.config.turnEnabled?59.0f:94.0f,25},"Undo",{Action::Undo},false,model.canUndo(),false,11);
    button({g.config.turnEnabled?77.0f:113.0f,211,g.config.turnEnabled?59.0f:94.0f,25},"Redo",{Action::Redo},false,model.canRedo(),false,11);
    if(g.config.turnEnabled){
        button({142,211,23,25},"-",{Action::QuickTurn,-1},false,true,true,15);buttons.back().label="Previous turn";
        button({168,211,61,25},"Turn "+number(g.turn),{Action::TurnInput},false,true,false,10);
        button({232,211,23,25},"+",{Action::QuickTurn,1},false,true,true,15);buttons.back().label="Next turn";
    }
    button({g.config.turnEnabled?261.0f:214.0f,211,g.config.turnEnabled?47.0f:94.0f,25},"Menu",{Action::Menu},false,true,false,11);
}

namespace {
// History is stored in chronological order. Keep that order in both views;
// the All tab presents one timeline rather than parallel player timelines.
std::vector<const LifeEvent*> historyRows(const Game& g,int team){
    std::vector<const LifeEvent*> result;
    for(const auto& e:g.history)if(team<0||e.team==team)result.push_back(&e);
    return result;
}
}
int App::maxHistoryOffset()const{
    const auto& g=model.state.game;
    int count=static_cast<int>(historyRows(g,historyAll?-1:selected/g.config.heads).size());
    return std::max(0,count-5);
}
void App::scrollHistory(float pixels){
    if(page!=Page::History||input.kind!=InputKind::None||modalOpen())return;
    scrollRemainder+=pixels;
    int rows=static_cast<int>(scrollRemainder/23);
    if(rows){historyPage=std::clamp(historyPage+rows,0,maxHistoryOffset());scrollRemainder-=rows*23;}
}
void App::historyView(){
    const auto& g=model.state.game;header("Game history");
    button({12,41,42,25},"All",{Action::HistoryAll},historyAll,true,false,11);
    float width=(248.0f-(g.config.players-1)*4)/g.config.players;
    for(int i=0;i<g.config.players;++i)button({60+i*(width+4),41,width,25},playerAbbreviation(g.players[i],i),{Action::SelectPlayer,i},!historyAll&&selected==i,true,false,11);
    auto rows=historyRows(g,historyAll?-1:selected/g.config.heads);
    historyPage=std::clamp(historyPage,0,maxHistoryOffset());
    // Four columns when turn tracking is on; otherwise reclaim its full width.
    float playerX=15,turnX=68,changeX=g.config.turnEnabled?128:112,detailX=299;
    label("Player",playerX,75,9,p.accent,Align::Left,49);
    if(g.config.turnEnabled)label("Turn #",turnX,75,9,p.accent,Align::Left,52);
    label("Change",changeX,75,9,p.accent,Align::Left,100);
    label("Type",detailX,75,9,p.accent,Align::Right,65);
    draw->box({12,88,289,1},p.line);
    for(int row=0;row<5;++row){int index=historyPage+row;float y=93+row*23;
        if(index>=static_cast<int>(rows.size()))break;
        const auto& e=*rows[index];int who=std::clamp(e.player,0,g.config.players-1);
        identityText(playerAbbreviation(g.players[who],who),playerX,y+4,11,g.players[who],Align::Left,47);
        if(g.config.turnEnabled)label(number(std::max(0,e.turn)),turnX,y+4,11,p.ink,Align::Left,50);
        bool roleEvent=e.kind==HistoryKind::Monarch||e.kind==HistoryKind::Initiative;
        std::string change=roleEvent?(e.before==1&&e.after==0?"Loses role":"Takes role"):(e.kind==HistoryKind::Poison?"P ":"")+signedNumber(e.after-e.before)+"→"+number(e.after);
        Color ink=roleEvent?p.accent:e.kind==HistoryKind::Poison?0xc48cf1:e.after>e.before?p.good:p.bad;
        label(change,changeX,y+2,roleEvent?10:12,ink,Align::Left,g.config.turnEnabled?104:121);
        std::string description=e.kind==HistoryKind::Monarch?"Monarch":e.kind==HistoryKind::Initiative?"Initiative":e.kind==HistoryKind::Poison?"Poison":e.source>=0?playerAbbreviation(g.players[e.source],e.source)+" CMD "+(e.slot?"B":"A"):"Life";
        label(description,detailX,y+4,9,p.muted,Align::Right,72);
        draw->box({12,y+20,289,.5f},p.line);
    }
    if(rows.empty())label("No game history recorded yet.",155,126,13,p.muted,Align::Center,280);
    // Persistent right-hand scrollbar: track and thumb scale with item count.
    int maximum=maxHistoryOffset();float trackY=93,trackH=111;
    draw->box({305,trackY,3,trackH},p.line);
    float thumb=maximum?std::max(12.f,trackH*5.0f/(maximum+5)):trackH;
    draw->box({305,trackY+(maximum?(trackH-thumb)*historyPage/maximum:0),3,thumb},p.accent);
    button({12,211,66,25},"Undo",{Action::Undo},false,model.canUndo(),false,11);
    button({84,211,66,25},"Redo",{Action::Redo},false,model.canRedo(),false,11);
    button({157,211,30,25},"",{Action::HistoryScroll,-1},false,historyPage>0,false);
    button({193,211,30,25},"",{Action::HistoryScroll,1},false,historyPage<maximum,false);
    // Four stepped rows of square pixels match the game's bitmap alphabet.
    for(int step=0;step<4;++step){float w=4+step*4;
        draw->box({172.0f-w/2,218.f+step*2,w,2},historyPage>0?p.ink:p.muted);
        draw->box({208.0f-w/2,225.f-step*2,w,2},historyPage<maximum?p.ink:p.muted);
    }
    button({232,211,76,25},"Menu",{Action::Menu},false,true,false,11);
}
void App::bottom(){
    auto& s=model.state;auto& g=s.game;
    switch(page){
    case Page::Home:
        button({12,12,296,50},"Create a new game",{Action::NewGame},true,true,false,18);
        button({12,73,296,43},g.active?"Resume game":"No game in progress",{Action::Resume},false,g.active,false,15);
        button({12,127,144,45},"Player profiles",{Action::Profiles},false,true,false,12);button({164,127,144,45},"Color theme",{Action::Theme},false,true,false,12);
        button({12,183,144,45},"Controls",{Action::Help},false,true,false,13);button({164,183,144,45},"Exit app",{Action::Exit},false,true,false,13);break;
    case Page::Rules:{
        header("Create a new game");
        button({12,43,190,29},"Load preset",{Action::Presets});button({210,43,98,29},"Save preset",{Action::SavePreset},false,s.presets.size()<MaxPresets,false,10);
        button({12,79,144,32},"Players / layout: "+number(s.setup.players),{Action::Players});
        button({164,79,144,32},(s.setup.heads>1?"Team life: ":"Starting life: ")+number(s.setup.life),{Action::Life});
        for(int h=1;h<=3;++h)button({12.0f+(h-1)*101,118,94,28},h==1?"Individual":number(h)+"-headed Giant",{Action::Heads,h},s.setup.heads==h,true,false,12);
        button({12,153,144,28},std::string("Commander: ")+(s.setup.commander?"On":"Off"),{Action::Commander},s.setup.commander,true,false,11);
        button({164,153,144,28},std::string("Partners: ")+(s.setup.partners?"On":"Off"),{Action::Partners},s.setup.partners,s.setup.commander,false,11);
        button({12,188,144,19},"Theme",{Action::Theme},false,true,false,10);
        button({164,188,144,19},"Turn Counter / Clock",{Action::Options},false,true,false,9);
        button({12,213,144,24},"Profiles (optional)",{Action::Seats},false,true,false,10);button({164,213,144,24},"Start game",{Action::Begin},true,true,false,12);break;}
    case Page::Seats:
    case Page::ActiveSeats:{
        bool live=page==Page::ActiveSeats;header("Set player profiles");
        int count=live?g.config.players:s.setup.players;
        for(int i=0;i<count;++i){int seat=s.seats[i];
            std::string identity=live?(g.players[i].profileUsed?g.players[i].name:"No profile"):(seat<0?"No profile":s.profiles[seat].name);
            button({12,43.0f+i*26,296,22},defaultPlayerName(i)+" / "+identity,{Action::PickSeat,i},false,true,false,11);
            Player color=live?g.players[i]:(seat<0?Player{}:asProfile(s.profiles[seat]));if(!live&&seat<0)color.color=i;identityFill({16,48.0f+i*26,12,12},color);
        }
        if(live)button({12,211,296,26},"Manage saved profiles",{Action::ManageProfiles},false,true,false,12);
        else{button({12,211,94,26},"Game setup",{Action::Rules},false,true,false,11);button({114,211,194,26},"Start game",{Action::Begin},true,true,false,13);}break;}
    case Page::Profiles:{
        header(pickSeat>=0?"Profile for player "+std::string(1,char('A'+pickSeat)):"Player profiles");
        int count=std::max(0,static_cast<int>(s.profiles.size())-NamedProfileStart),perPage=pickSeat>=0?4:5;listPage=std::clamp(listPage,0,std::max(0,(count-1)/perPage));
        if(pickSeat>=0)button({12,43,296,23},"No profile ("+defaultPlayerName(pickSeat)+")",{Action::Profile,-1},(assigningLiveProfiles?!g.players[pickSeat].profileUsed:s.seats[pickSeat]<0),true,false,12);
        float start=pickSeat>=0?72:43;
        for(int k=0;k<perPage;++k){int i=NamedProfileStart+listPage*perPage+k;if(i>=static_cast<int>(s.profiles.size()))break;const auto& pp=s.profiles[i];button({12,start+k*27,296,23},pp.name,{Action::Profile,i},false,true,false,12);identityFill({17,start+5+k*27,14,12},asProfile(pp));}
        button({12,185,64,21},"<",{Action::Page,-1},false,listPage>0,false,11);label(number(listPage+1)+" / "+number(std::max(1,(count+perPage-1)/perPage)),160,189,10,p.muted,Align::Center);button({244,185,64,21},">",{Action::Page,1},false,(listPage+1)*perPage<count,false,11);
        button({12,212,296,25},"Create player profile",{Action::NewProfile},true,count<MaxNamedProfiles,false,12);break;}
    case Page::Profile:{
        header("Edit profile");auto& pp=s.profiles[profile];
        label("NAME",12,48,10,p.muted);button({12,66,296,38},pp.name,{Action::RenameProfile},false,true,false,16);
        label("COLOR PAIR",12,117,10,p.muted);
        for(int i=0;i<10;++i){float x=12+(i%5)*60,y=132+(i/5)*33;button({x,y,56,28},colorPairName(i),{Action::Color,i},i==pp.color,true,false,10);Player swatch;swatch.profileUsed=true;swatch.color=i;identityFill({x+6,y+21,44,4},swatch);}
        button({12,211,144,25},"Delete profile",{Action::DeleteProfile},false,true,false,11);
        button({164,211,144,25},"Done",{Action::ProfileDone},true,true,false,12);break;}
    case Page::Presets:{
        header("Game presets");int count=static_cast<int>(s.presets.size());listPage=std::clamp(listPage,0,std::max(0,(count-1)/5));
        for(int k=0;k<5;++k){int i=listPage*5+k;if(i>=count)break;button({12,43.0f+k*28,252,24},s.presets[i].name,{Action::UsePreset,i},false,true,false,12);button({272,43.0f+k*28,36,24},"Del",{Action::DeletePreset,i},false,true,false,9);}
        if(count==0){label("No saved presets",160,94,16,p.ink,Align::Center,296);label("Save a preset from game setup.",160,121,11,p.muted,Align::Center,296);}
        button({12,190,90,26},"Previous",{Action::Page,-1},false,listPage>0,false,10);button({218,190,90,26},"Next",{Action::Page,1},false,(listPage+1)*5<count,false,10);break;}
    case Page::Game:{
        selected=std::clamp(selected,0,g.config.players-1);const auto& who=g.players[selected];if(!g.config.commander&&tab==1)tab=0;
        identityShadowText(who.name,12,8,15,who,Align::Left,218);
        if(g.config.timerEnabled)label(timeText(g.timerRemaining),308,10,9,p.muted,Align::Right,74);
        playerStrip();
        if(g.config.commander){button({12,60,94,24},"Life",{Action::Tab,0},tab==0,true,false,11);button({113,60,94,24},"Commander",{Action::Tab,1},tab==1,true,false,10);button({214,60,94,24},"Counters",{Action::Tab,2},tab==2,true,false,10);}
        else{button({12,60,144,24},"Life",{Action::Tab,0},tab==0,true,false,11);button({164,60,144,24},"Counters",{Action::Tab,2},tab==2,true,false,11);}
        if(tab==0){
            button({12,99,44,38},"-5",{Action::Change,-5,int(Counter::Life)},false,true,true,13);
            button({62,99,44,38},"-",{Action::Change,-1,int(Counter::Life)},false,true,true,25);
            button({114,90,92,56},"",{Action::SetLife});buttons.back().label="Set life total";
            lifeText(*draw,who.life,160,94,30,p.ink,Align::Center,80,now,g.config.life/2,false,false,displayedLife(selected),lifeMotion[selected].from>lifeMotion[selected].target);
            button({214,99,44,38},"+",{Action::Change,1,int(Counter::Life)},false,true,true,25);
            button({264,99,44,38},"+5",{Action::Change,5,int(Counter::Life)},false,true,true,13);
            label("Tap life to set / hold +/- to repeat",160,154,8,p.muted,Align::Center,296);
        }else if(tab==1){
            source=std::clamp(source,0,g.config.players-1);if(!g.config.partners)slot=0;
            // All incoming commanders stay visible, including sources with zero damage.
            // The selected attack source remains a separate, compact control on the left.
            button({12,90,151,24},"From: "+g.players[source].name,{Action::Source},false,true,false,11);
            if(g.config.partners)button({12,117,151,17},slot==0?"Commander A":"Commander B",{Action::Slot},false,true,false,9);
            button({12,155,40,19},"-",{Action::Change,-1,int(Counter::Commander)},false,true,true,19);
            identityText(number(who.damage[source][slot]),86,153,21,g.players[source],Align::Center,62);
            button({123,155,40,19},"+",{Action::Change,1,int(Counter::Commander)},false,true,true,19);
            button({12,g.config.partners?136.0f:116.0f,151,17},g.config.linkLife?"Life linked: ON":"Life linked: OFF",{Action::LinkLife},false,true,false,9);
            panel({170,90,138,82},p.raised);
            label("Commander damage",239,92,10,p.muted,Align::Center,132);
            for(int j=0;j<g.config.players;++j){
                int a=who.damage[j][0],b=who.damage[j][1];
                std::string entry=playerAbbreviation(g.players[j],j)+":"+number(a);
                if(g.config.partners)entry+="/"+number(b);
                identityText(entry,176,105+j*10.5f,10,g.players[j],Align::Left,126);
            }
        }else{
            const char* names[]={g.config.heads>1?"Team poison":"Poison","Energy","Experience"};int values[]={who.poison,who.energy,who.experience};const Color colors[]={0xc48cf1,0xffda58,0xc8d0dc};
            for(int j=0;j<3;++j){float y=88+j*21;label(names[j],12,y+3,10,colors[j]);button({164,y,35,19},"-",{Action::Change,-1,j+1},false,true,true,13);label(number(values[j]),232,y+2,13,colors[j],Align::Center,44);button({273,y,35,19},"+",{Action::Change,1,j+1},false,true,true,13);}
            button({12,155,144,18},g.monarch==selected?"Remove monarch":"Take monarch",{Action::Monarch},g.monarch==selected,true,false,9);
            button({164,155,144,18},g.initiative==selected?"Remove initiative":"Take initiative",{Action::Initiative},g.initiative==selected,true,false,9);
        }
        quickControls();footer();break;}
    case Page::History:historyView();break;
    case Page::Menu:
        header("Settings");button({12,43,296,27},"Return to game",{Action::Resume},true,g.active,false,13);
        button({12,77,144,27},"Game settings",{Action::Clock},false,g.active,false,11);button({164,77,144,27},"Seat layout",{Action::GameLayout},false,g.active,false,12);
        button({12,111,144,27},"Player profiles",{Action::Profiles},false,g.active,false,11);button({164,111,144,27},"Color theme",{Action::Theme},false,true,false,12);
        button({12,145,144,27},"Controls",{Action::Help},false,true,false,12);button({164,145,144,27},"Reset life & counters",{Action::ResetTable},false,g.active,false,10);
        button({12,179,296,25},"Create a new game",{Action::NewGame},false,true,false,12);
        button({12,211,144,25},"Home",{Action::Home},false,true,false,12);button({164,211,144,25},"Exit app",{Action::Exit},false,true,false,12);break;
    case Page::Help:
        header("Controls");
        label("MENUS",12,46,10,p.accent);
        label("D-pad: move focus / A: select / B: back",12,62,11,p.ink,Align::Left,296);
        label("GAME SHORTCUTS",12,87,10,p.accent);
        label("A / B or Up / Down: add / subtract",12,103,11,p.ink,Align::Left,296);
        label("Current tab; Counters adjusts poison",12,120,10,p.muted,Align::Left,296);
        label("L / R or Left / Right: select player",12,139,11,p.ink,Align::Left,296);
        label("X: undo / Y: history / START: settings",12,158,11,p.ink,Align::Left,296);
        label("SELECT: toggle button navigation",12,181,11,p.accent,Align::Left,296);
        label("Touch: select / hold + or - to repeat",12,201,10,p.muted,Align::Left,296);
        button({12,219,296,18},"Back",{Action::Back},true,true,false,10);break;
    case Page::Dice:{
        header("Dice tray");const int sides[]={2,4,6,8,10,12,20};
        for(int i=0;i<7;++i){int row=i/4,col=i%4;button({12.0f+col*76,43.0f+row*31,68,26},i==0?"Coin":"d"+number(sides[i]),{Action::Die,sides[i]},dieSides==sides[i],!rolling,false,12);}
        label("Watch the upper screen.",160,118,13,p.ink,Align::Center,296);
        label("Tilt the 3DS to move the die, even at rest.",160,143,11,p.muted,Align::Center,296);
        button({12,186,296,43},rolling?"Rolling...":(dieSides==2?"Flip coin":"Roll d"+number(dieSides)),{Action::Roll},true,!rolling,false,16);break;}
    case Page::Float:{
        header("Floating mana tracker");const char* names[]={"WHITE","BLUE","BLACK","RED","GREEN","COLORLESS"};
        for(int j=0;j<6;++j){Rect tile{12.0f+(j%3)*101,44.0f+(j/3)*63,94,57};panel(tile,p.raised);
            Color color=j==5?0xc8d0dc:magicColor(j);draw->box({tile.x+5,tile.y+7,7,7},p.muted);draw->box({tile.x+6,tile.y+8,5,5},color);label(names[j],tile.x+52,tile.y+6,9,p.ink,Align::Center,78);
            button({tile.x+5,tile.y+28,24,23},"-",{Action::ChangeFloat,j,-1},false,true,true,16);buttons.back().label=std::string("Remove ")+names[j];
            label(number(g.floatPool[j]),tile.x+47,tile.y+31,15,0,Align::Center,32);
            button({tile.x+65,tile.y+28,24,23},"+",{Action::ChangeFloat,j,1},false,true,true,16);buttons.back().label=std::string("Add ")+names[j];
        }
        label("Storm",12,183,12);button({100,177,35,25},"-",{Action::ChangeFloat,6,-1},false,true,true,16);buttons.back().label="Remove storm";
        label(number(g.floatPool[6]),160,181,16,0,Align::Center,40);button({185,177,35,25},"+",{Action::ChangeFloat,6,1},false,true,true,16);buttons.back().label="Add storm";
        button({232,177,76,25},"Reset all",{Action::ResetFloat},false,true,false,10);break;}
    case Page::Layout:{
        auto& config=editingGameLayout?g.config:s.setup;
        header(editingGameLayout?"Seat layout":"Players and layout");
        if(!editingGameLayout)for(int n=2;n<=6;++n)button({12.0f+(n-2)*60,41,56,23},number(n),{Action::PlayerCount,n},config.players==n,n%s.setup.heads==0&&n/s.setup.heads>=2,false,12);
        if(editingGameLayout)label(number(config.players)+" players",160,46,12,p.muted,Align::Center,296);
        int count=layoutCount(config.players);
        for(int k=0;k<count;++k){Rect tile{12.0f+(k%3)*101,72.0f+(k/3)*51,94,45};button(tile,"",{Action::Layout,k},config.layout==k);
            buttons.back().label="Layout: "+layoutName(config.players,k);
            auto shapes=playerPanels(config.players,k);
            for(int i=0;i<config.players;++i){auto r=shapes[i];float x=tile.x+10+(r.x-4)*.18f,y=tile.y+4+(r.y-20)*.105f;draw->box({x,y,r.w*.18f-1,r.h*.105f-1},config.layout==k?p.accentInk:p.line);label(std::string("P")+char('A'+i),x+3,y+2,6,config.layout==k?p.accent:p.ink,Align::Left,15);}
            label(layoutName(config.players,k),tile.x+47,tile.y+34,7,p.ink,Align::Center,88);
        }
        label("Preview player orientation on the top screen.",160,185,9,p.muted,Align::Center,296);
        button({12,215,296,22},"Done",{Action::LayoutDone},true,true,false,11);break;}
    case Page::Options:
        header("Turn counter and clock");
        button({12,44,196,31},std::string("Turn counter: ")+(s.setup.turnEnabled?"ON":"OFF"),{Action::TurnToggle},s.setup.turnEnabled,true,false,11);
        button({216,44,92,31},"Start: "+number(s.setup.startTurn),{Action::TurnInput},false,s.setup.turnEnabled,false,10);
        button({12,83,296,31},std::string("Countdown timer: ")+(s.setup.timerEnabled?"ON":"OFF"),{Action::TimerToggle},s.setup.timerEnabled,true,false,12);
        button({12,122,296,31},"Duration: "+number(s.setup.timerSeconds/60)+" minutes",{Action::TimerInput},false,s.setup.timerEnabled,false,12);
        button({12,161,296,31},std::string("Show current time: ")+(s.setup.clockEnabled?"ON":"OFF"),{Action::ClockToggle},s.setup.clockEnabled,true,false,12);
        button({12,211,296,26},"Done",{Action::Rules},true,true,false,12);break;
    case Page::Theme:{
        header("Choose a color theme");
        label("Choose one to five colors",160,47,13,p.ink,Align::Center,296);
        const char* names[]={"White","Blue","Black","Red","Green"};
        int selectedColors=themeReturn==Page::Rules?s.setup.themeColors:s.themeColors;
        for(int j=0;j<5;++j){Rect tile{12.0f+j*60,77,56,50};bool active=selectedColors&(1<<j);
            button(tile,names[j],{Action::ThemeColor,j},active,true,false,11);
            draw->box({tile.x+8,116,40,3},magicColor(j));
        }
        label("Rotation speed",12,143,12,p.ink,Align::Left,210);
        const char* marks[]={"1", "1.5", "2", "2.5", "3"};
        draw->box({40,172,240,2},p.line);
        for(int j=0;j<5;++j){
            const float x=40.f+j*60;const bool chosen=s.themeSpeedStep==j;
            draw->box({x-2,169,4,8},chosen?p.accent:p.muted);
            draw->box({x-(chosen?6.f:4.f),172-(chosen?6.f:4.f),chosen?12.f:8.f,chosen?12.f:8.f},chosen?p.accent:p.raised);
            label(marks[j],x,188,10,chosen?p.ink:p.muted,Align::Center,44);
            buttons.push_back({{x-24,158,48,44},{Action::ThemeSpeed,j},std::string("Rotation speed ")+marks[j]+"x",true,false});
        }
        button({12,208,296,28},"Done",{Action::Back},true,true,false,13);break;}
    case Page::Clock:case Page::GameTrackers:
        header("Game settings");
        button({12,43,187,27},std::string("Commander damage: ")+(g.config.commander?"ON":"OFF"),{Action::Commander},g.config.commander,true,false,11);
        button({207,43,101,27},std::string("Partners: ")+(g.config.partners?"ON":"OFF"),{Action::Partners},g.config.partners,g.config.commander,false,11);
        button({12,78,187,27},std::string("Turn counter: ")+(g.config.turnEnabled?"ON":"OFF"),{Action::TurnToggle},g.config.turnEnabled,true,false,11);
        button({207,78,101,27},"Turn "+number(g.turn),{Action::TurnInput},false,g.config.turnEnabled,false,11);
        button({12,113,187,27},std::string("Countdown timer: ")+(g.config.timerEnabled?"ON":"OFF"),{Action::TimerToggle},g.config.timerEnabled,true,false,11);
        label(timeText(g.timerRemaining),258,119,17,g.config.timerEnabled?p.ink:p.muted,Align::Center,95);
        button({12,148,144,27},g.timerRunning?"Pause timer":"Start timer",{Action::TimerPause},false,g.config.timerEnabled,false,11);
        button({164,148,144,27},"Set minutes",{Action::TimerInput},false,g.config.timerEnabled,false,11);
        button({12,183,296,27},std::string("Show current time: ")+(g.config.clockEnabled?"ON":"OFF"),{Action::ClockToggle},g.config.clockEnabled,true,false,12);
        button({12,218,296,19},"Done",{Action::Back},true,true,false,11);break;
    }
}
void App::back(){
    if(confirming.action!=Action::None){confirming={};return;}
    if(page==Page::Rules)newGameApproved=false;
    switch(page){
    case Page::Rules:case Page::Menu:page=model.state.game.active?Page::Game:Page::Home;break;
    case Page::Seats:case Page::Presets:case Page::Options:page=Page::Rules;break;
    case Page::Layout:page=editingGameLayout?Page::Menu:Page::Rules;break;
    case Page::ActiveSeats:page=Page::Menu;break;
    case Page::GameTrackers:page=Page::Menu;break;
    case Page::Profiles:page=pickSeat>=0?(assigningLiveProfiles?Page::ActiveSeats:Page::Seats):returnPage;pickSeat=-1;break;
    case Page::Profile:page=Page::Profiles;break;
    case Page::History:page=Page::Game;break;
    case Page::Dice:page=diceReturn;break;
    case Page::Float:page=Page::Game;break;
    case Page::Clock:page=Page::Menu;break;
    case Page::Theme:page=themeReturn;break;
    case Page::Help:page=returnPage;break;
    default:page=Page::Home;break;
    }
}
void App::dispatch(Command c){
    if(input.kind!=InputKind::None)return;
    if(confirming.action!=Action::None&&c.action!=Action::Confirm&&c.action!=Action::Cancel&&c.action!=Action::Back)return;
    auto& s=model.state;auto& g=s.game;
    switch(c.action){
    case Action::NewGame:
        if(!c.a&&g.active&&!g.history.empty()){confirm({Action::NewGame,1},"Create a new game?","This will replace the current game\nand its life change history.");break;}
        newGameApproved=g.active&&!g.history.empty();soundTeam=-1;s.setup=Config{};s.seats.fill(-1);model.touch();page=Page::Rules;pickSeat=-1;editingGameLayout=assigningLiveProfiles=false;break;
    case Action::Home:newGameApproved=false;page=Page::Home;break;
    case Action::Resume:if(g.active){newGameApproved=false;page=Page::Game;tab=0;focusVisible=false;}break;
    case Action::Rules:page=Page::Rules;break;
    case Action::Seats:assigningLiveProfiles=false;page=Page::Seats;break;
    case Action::Begin:
        soundTeam=-1;
        if(g.active&&c.a==0&&!newGameApproved){confirm({Action::Begin,1},"Start a new game?","This replaces the current game\nand its life history.");break;}
        if(model.start()){newGameApproved=false;lifeMotion={};s.themeColors=s.setup.themeColors;page=Page::Game;focusVisible=false;selected=0;tab=0;source=1;slot=0;historyPage=0;}break;
    case Action::SelectPlayer:selected=c.a;historyPage=0;scrollRemainder=0;if(page==Page::History)historyAll=false;break;
    case Action::Tab:if(c.a!=1||g.config.commander)tab=c.a;break;
    case Action::Change:{bool poison=c.b==int(Counter::Poison);int before=poison?g.players[selected].poison:g.players[selected].life;
        if(model.change(selected,static_cast<Counter>(c.b),c.a,source,slot)){
            int delta=(poison?g.players[selected].poison:g.players[selected].life)-before;
            feedback=c;feedbackPage=page;feedbackUntil=now+170;
            if(delta)lifeSound(delta,c.b);else soundTeam=-1;
        }break;}
    case Action::Undo:model.undo();soundTeam=-1;notice.clear();break;
    case Action::Redo:model.redo();soundTeam=-1;notice.clear();break;
    case Action::History:page=Page::History;historyPage=0;historyAll=true;scrollRemainder=0;break;
    case Action::HistoryAll:historyAll=true;historyPage=0;scrollRemainder=0;break;
    case Action::HistoryScroll:historyPage=std::clamp(historyPage+c.a,0,maxHistoryOffset());scrollRemainder=0;break;
    case Action::ResetTable:soundTeam=-1;model.resetTable();page=Page::Game;tab=0;message("Life and counters reset");break;
    case Action::Menu:page=Page::Menu;break;
    case Action::Help:returnPage=page;page=Page::Help;break;
    case Action::Theme:themeReturn=page;page=Page::Theme;break;
    case Action::ThemeColor:if(c.a>=0&&c.a<5){
        int& mask=themeReturn==Page::Rules?s.setup.themeColors:s.themeColors;
        int next=mask^(1<<c.a);if(next){mask=next;if(g.active&&themeReturn==Page::Menu)g.config.themeColors=next;model.touch();}
        }break;
    case Action::ThemeSpeed:if(c.a>=0&&c.a<5&&s.themeSpeedStep!=c.a){s.themeSpeedStep=c.a;model.touch();}break;
    case Action::LinkLife:g.config.linkLife=!g.config.linkLife;model.touch();break;
    case Action::Players:editingGameLayout=false;page=Page::Layout;break;
    case Action::GameLayout:if(g.active){editingGameLayout=true;page=Page::Layout;}break;
    case Action::LayoutDone:page=editingGameLayout?Page::Menu:Page::Rules;break;
    case Action::PlayerCount:if(editingGameLayout&&page==Page::Layout)break;if(c.a>=2&&c.a<=6&&c.a%s.setup.heads==0&&c.a/s.setup.heads>=2){if(s.setup.players!=c.a){s.setup.life=c.a>=3?40:20;s.setup.commander=c.a>=3;}s.setup.players=c.a;chooseLayout(s.setup,s.setup.players==3?2:s.setup.players==5?1:0);model.touch();}break;
    case Action::Layout:chooseLayout(editingGameLayout?g.config:s.setup,c.a);model.touch();break;
    case Action::Life:request(InputKind::Life,s.setup.heads>1?"Starting team life (1-999)":"Starting life (1-999)",number(s.setup.life),true);break;
    case Action::Heads:{int old=s.setup.players;s.setup.heads=c.a;s.setup.players=std::max(2*c.a,(s.setup.players/c.a)*c.a);if(old!=s.setup.players){s.setup.life=s.setup.players>=3?40:20;s.setup.commander=s.setup.players>=3;}chooseLayout(s.setup,s.setup.players==3?2:s.setup.players==5?1:0);model.touch();break;}
    case Action::Commander:if(page==Page::Clock||page==Page::GameTrackers){g.config.commander=!g.config.commander;if(!g.config.commander&&tab==1)tab=0;}else s.setup.commander=!s.setup.commander;model.touch();break;
    case Action::Partners:{auto& config=(page==Page::Clock||page==Page::GameTrackers)?g.config:s.setup;if(config.commander){config.partners=!config.partners;if(!config.partners)slot=0;model.touch();}break;}
    case Action::PickSeat:assigningLiveProfiles=page==Page::ActiveSeats;pickSeat=c.a;listPage=0;page=Page::Profiles;break;
    case Action::Profiles:returnPage=page;pickSeat=-1;assigningLiveProfiles=page==Page::Menu&&g.active;page=assigningLiveProfiles?Page::ActiveSeats:Page::Profiles;listPage=0;break;
    case Action::ManageProfiles:returnPage=page;pickSeat=-1;page=Page::Profiles;listPage=0;break;
    case Action::Profile:
        if(pickSeat>=0){if(assigningLiveProfiles)model.assignActive(pickSeat,c.a);else model.assign(pickSeat,c.a);page=assigningLiveProfiles?Page::ActiveSeats:Page::Seats;pickSeat=-1;}
        else{profile=c.a;page=Page::Profile;}break;
    case Action::NewProfile:request(InputKind::NewProfile,"Name your player profile");break;
    case Action::RenameProfile:request(InputKind::RenameProfile,"Rename player profile",s.profiles[profile].name);break;
    case Action::ProfileDone:
        if(pickSeat>=0){if(assigningLiveProfiles)model.assignActive(pickSeat,profile);else model.assign(pickSeat,profile);pickSeat=-1;page=assigningLiveProfiles?Page::ActiveSeats:Page::Seats;}else page=Page::Profiles;break;
    case Action::Color:model.colorProfile(profile,c.a);break;
    case Action::DeleteProfile:
        if(!c.a){confirm({Action::DeleteProfile,1},"Delete this profile?","Current game names stay unchanged.\nThis removes the saved profile.");break;}
        model.deleteProfile(profile);page=Page::Profiles;break;
    case Action::Presets:page=Page::Presets;listPage=0;break;
    case Action::UsePreset:s.setup=s.presets[c.a].config;model.touch();page=Page::Rules;break;
    case Action::SavePreset:request(InputKind::Preset,"Name this game preset");break;
    case Action::DeletePreset:
        if(!c.b){confirm({Action::DeletePreset,c.a,1},"Delete this preset?","Your current setup and game\nwill stay unchanged.");break;}
        model.deletePreset(c.a);break;
    case Action::Page:if(page==Page::History){historyPage=std::clamp(historyPage+c.a,0,maxHistoryOffset());scrollRemainder=0;}else listPage+=c.a;break;
    case Action::Source:source=(source+1)%g.config.players;break;
    case Action::Slot:if(g.config.partners)slot=1-slot;break;
    case Action::Monarch:soundTeam=-1;model.marker(selected,false);break;
    case Action::Initiative:soundTeam=-1;model.marker(selected,true);break;
    case Action::Back:back();break;
    case Action::Cancel:confirming={};break;
    case Action::Confirm:{auto command=confirming;confirming={};dispatch(command);break;}
    case Action::Exit:
        if(!saveOk&&!c.a){confirm({Action::Exit,1},"Exit without saving?","Saving is unavailable.\nSession changes will be lost.");break;}
        exitWithoutSave=c.a!=0;quitRequested=true;break;
    case Action::Dice:diceReturn=page;page=Page::Dice;break;
    case Action::Float:if(g.active)page=Page::Float;break;
    case Action::ChangeFloat:model.changeFloat(c.a,c.b);soundTeam=-1;break;
    case Action::ResetFloat:model.resetFloat();soundTeam=-1;break;
    case Action::SetLife:request(InputKind::SetLife,"Life total (-9999 to 99999)",number(g.players[selected].life),true);break;
    case Action::QuickTurn:if(g.config.turnEnabled)model.change(selected,Counter::Turn,c.a);soundTeam=-1;break;
    case Action::Die:if(!rolling){dieSides=c.a;dieResult=0;hasRollResult=false;}break;
    case Action::Roll:if(!rolling){
        if(randomState==0)randomState=0x4d474943;
        randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;
        rollSeed=randomState;rollSimulation.start(dieSides,rollSeed);dieResult=0;hasRollResult=true;rolling=true;rollStart=lastRollTick=now;neutralTiltX=tiltX;neutralTiltY=tiltY;rollHitCount=0;sounds.push_back(Sound::DiceShake);
        }break;
    case Action::Options:page=Page::Options;break;
    case Action::Clock:page=Page::Clock;break;
    case Action::GameTrackers:page=Page::Clock;break;
    case Action::TurnToggle:if(page==Page::Options)s.setup.turnEnabled=!s.setup.turnEnabled;else g.config.turnEnabled=!g.config.turnEnabled;model.touch();break;
    case Action::TimerToggle:
        if(page==Page::Options)s.setup.timerEnabled=!s.setup.timerEnabled;
        else{g.config.timerEnabled=!g.config.timerEnabled;g.timerRunning=false;g.timerDeadline=0;}model.touch();break;
    case Action::TimerInput:request(page==Page::Options?InputKind::TimerSetup:InputKind::TimerGame,"Timer minutes (1-999)",number((page==Page::Options?s.setup.timerSeconds:g.config.timerSeconds)/60),true);break;
    case Action::TurnInput:if(page==Page::Game&&!g.config.turnEnabled)break;request(page==Page::Options?InputKind::TurnSetup:InputKind::Turn,"Turn number (0-9999)",number(page==Page::Options?s.setup.startTurn:g.turn),true);break;
    case Action::ClockToggle:if(page==Page::Clock||page==Page::GameTrackers)g.config.clockEnabled=!g.config.clockEnabled;else s.setup.clockEnabled=!s.setup.clockEnabled;model.touch();break;
    case Action::TimerPause:
        if(g.config.timerEnabled){if(g.timerRunning){g.timerRunning=false;g.timerDeadline=0;}else{if(g.timerRemaining==0)g.timerRemaining=g.config.timerSeconds;g.timerRunning=true;g.timerDeadline=wallSeconds+g.timerRemaining;}model.touch();}break;
    default:break;
    }
}
void App::lifeSound(int delta,int kind){
    if(kind==int(Counter::Poison)){
        int team=selected/model.state.game.config.heads;
        if(soundTeam!=team||soundKind!=kind||now<soundTime||now-soundTime>=2000)soundAmount=0;
        soundAmount+=delta;
        if(delta>0)sounds.push_back(soundAmount>=3?Sound::Impact:Sound::Tick);
        soundTeam=team;soundKind=kind;soundTime=now;return;
    }
    int direction=delta>0?1:-1,team=kind==static_cast<int>(Counter::Commander)?selected+MaxPlayers:selected/model.state.game.config.heads;
    int origin=kind==static_cast<int>(Counter::Commander)?source:-1,which=origin>=0?slot:0;
    if(soundTeam!=team||soundKind!=kind||soundSource!=origin||soundSlot!=which||soundDirection!=direction||now<soundTime||now-soundTime>=2000)soundAmount=0;
    int previous=soundAmount;soundAmount+=std::abs(delta);
    int chunks=soundAmount/5-previous/5;
    if(chunks)for(int i=0;i<std::min(chunks,8);++i)sounds.push_back(direction<0?Sound::Impact:Sound::Heal);
    else sounds.push_back(direction<0?Sound::Tick:Sound::Chime);
    soundTeam=team;soundKind=kind;soundSource=origin;soundSlot=which;soundDirection=direction;soundTime=now;
}
void App::pointerDown(float x,float y,uint64_t ms){
    pointerUp();now=ms;focusVisible=false;model.setInputTime(ms);notice.clear();
    if(page==Page::History&&y>=86&&y<205&&!modalOpen()){historyDragging=true;historyDragY=y;scrollRemainder=0;return;}
    for(auto it=buttons.rbegin();it!=buttons.rend();++it)if(it->enabled&&it->rect.contains(x,y)){
        Button b=*it;held=b.command;heldRect=b.rect;holding=true;repeat=b.repeat;keyboardHold=false;nextRepeat=ms+450;
        if(changeCommand(held))model.beginGesture();
        dispatch(held);break;
    }
}
void App::pointerMove(float x,float y){if(historyDragging){scrollHistory(historyDragY-y);historyDragY=y;return;}if(holding&&!keyboardHold&&!heldRect.contains(x,y))pointerUp();}
void App::pointerUp(){historyDragging=false;if(holding){model.endGesture();holding=false;repeat=false;keyboardHold=false;held={};}}
void App::tick(uint64_t ms){
    now=ms;model.setInputTime(ms);
    auto& game=model.state.game;
    if(game.active&&game.config.timerEnabled&&game.timerRunning&&wallSeconds){
        if(game.timerDeadline==0){game.timerDeadline=wallSeconds+game.timerRemaining;model.touch();}
        game.timerRemaining=game.timerDeadline>wallSeconds?static_cast<int>(std::min<uint64_t>(59940,game.timerDeadline-wallSeconds)):0;
        if(game.timerRemaining==0){game.timerRunning=false;game.timerDeadline=0;model.touch();}
    }
    if(rolling||(page==Page::Dice&&hasRollResult)){
        bool wasRolling=rolling;
        rollSimulation.advance(now>=lastRollTick?float(now-lastRollTick)/1000:0,tiltX-neutralTiltX,tiltY-neutralTiltY,trayYawRate);lastRollTick=now;auto motion=rollSimulation.pose();
        if(motion.hits>rollHitCount){sounds.push_back(dieSides==2?Sound::CoinClink:Sound::DiceTap);rollHitCount=motion.hits;}
        rolling=!motion.settled;dieResult=rolling?0:motion.result;
        if(motion.settled&&wasRolling){if(rolls.size()==5)rolls.erase(rolls.begin());rolls.push_back(dieSides==2?(dieResult==1?"H":"T"):"d"+number(dieSides)+":"+number(dieResult));}
    }
    lastRollTick=now;
    if(lastClock==0)lastClock=ms;
    if(ms>=lastClock+1000){uint64_t secs=(ms-lastClock)/1000;lastClock+=secs*1000;
        if(model.state.game.active&&page!=Page::Home&&page!=Page::Rules&&page!=Page::Seats&&secs<5){model.state.game.elapsed+=static_cast<uint32_t>(secs);if(model.state.game.elapsed%15==0)model.touch();}
    }
    if(holding&&repeat&&ms>=nextRepeat){dispatch(held);nextRepeat=ms+110;}
    if(!notice.empty()&&ms>noticeUntil)notice.clear();
}
void App::syncFocus(){
    if(focusPage!=page||focusModal!=modalOpen()){focusPage=page;focusModal=modalOpen();focused={};}
    auto found=std::find_if(buttons.begin(),buttons.end(),[&](const Button& b){return b.enabled&&b.command==focused&&b.rect==focusedRect;});
    if(found!=buttons.end())return;
    auto first=std::find_if(buttons.begin(),buttons.end(),[](const Button& b){return b.enabled&&b.command.action!=Action::Back;});
    if(first==buttons.end())first=std::find_if(buttons.begin(),buttons.end(),[](const Button& b){return b.enabled;});
    focused=first==buttons.end()?Command{}:first->command;focusedRect=first==buttons.end()?Rect{}:first->rect;
}
void App::drawFocus(){
    if(!focusVisible)return;
    for(const auto& b:buttons)if(b.enabled&&b.command==focused&&b.rect==focusedRect){Rect r=b.rect;
        const Color ink=0xfff1b5;draw->box({r.x,r.y,r.w,2},ink);draw->box({r.x,r.y+r.h-2,r.w,2},ink);draw->box({r.x,r.y,2,r.h},ink);draw->box({r.x+r.w-2,r.y,2,r.h},ink);break;}
}
void App::moveFocus(Key direction){
    syncFocus();std::vector<const Button*> ordered;for(const auto& b:buttons)if(b.enabled)ordered.push_back(&b);
    if(ordered.empty())return;
    std::stable_sort(ordered.begin(),ordered.end(),[](auto a,auto b){if(a->rect.y!=b->rect.y)return a->rect.y<b->rect.y;return a->rect.x<b->rect.x;});
    int current=0;for(size_t i=0;i<ordered.size();++i)if(ordered[i]->command==focused&&ordered[i]->rect==focusedRect)current=i;
    int step=direction==Key::Left||direction==Key::Up?-1:1;
    int next=(current+step+static_cast<int>(ordered.size()))%ordered.size();
    if(direction==Key::Up||direction==Key::Down){auto from=ordered[current]->rect;float cx=from.x+from.w/2,cy=from.y+from.h/2,best=std::numeric_limits<float>::max();
        for(size_t i=0;i<ordered.size();++i){auto r=ordered[i]->rect;float dy=(r.y+r.h/2-cy)*step;if(dy<3)continue;float dx=std::abs(r.x+r.w/2-cx);float score=dy+dx*1.8f;if(score<best){best=score;next=i;}}
    }
    focused=ordered[next]->command;focusedRect=ordered[next]->rect;
}
void App::activateFocus(uint64_t ms){
    syncFocus();for(const auto& b:buttons)if(b.enabled&&b.command==focused&&b.rect==focusedRect){Button copy=b;held=copy.command;holding=true;keyboardHold=true;repeat=copy.repeat;nextRepeat=ms+450;if(changeCommand(held))model.beginGesture();dispatch(held);return;}
}
void App::keyDown(Key k,uint64_t ms){
    if(input.kind!=InputKind::None)return;
    pointerUp();now=ms;model.setInputTime(ms);notice.clear();
    if(k==Key::Select&&!modalOpen()){focusVisible=!focusVisible;syncFocus();return;}
    if(modalOpen()){
        focusVisible=true;if(k==Key::B){dispatch({Action::Cancel});return;}
        if(k==Key::A)activateFocus(ms);else if(k==Key::Up||k==Key::Down||k==Key::Left||k==Key::Right)moveFocus(k);return;
    }
    if(k==Key::Start){dispatch({model.state.game.active?Action::Menu:Action::Home});focusVisible=true;return;}
    if((page==Page::Game||page==Page::History)&&!focusVisible){
        if(page==Page::History&&(k==Key::Up||k==Key::Down)){historyPage=std::clamp(historyPage+(k==Key::Down?1:-1),0,maxHistoryOffset());scrollRemainder=0;return;}
        if(k==Key::L||k==Key::Left){if(page==Page::History)historyAll=false;selected=(selected+model.state.game.config.players-1)%model.state.game.config.players;historyPage=0;return;}
        if(k==Key::R||k==Key::Right){if(page==Page::History)historyAll=false;selected=(selected+1)%model.state.game.config.players;historyPage=0;return;}
        if(k==Key::X){dispatch({Action::Undo});return;}
        if(k==Key::Y){dispatch({page==Page::History?Action::Back:Action::History});return;}
        if(page==Page::Game&&(k==Key::A||k==Key::B||k==Key::Up||k==Key::Down)){
            int kind=tab==1?static_cast<int>(Counter::Commander):(tab==2?static_cast<int>(Counter::Poison):static_cast<int>(Counter::Life));
            held={Action::Change,(k==Key::A||k==Key::Up)?1:-1,kind};holding=true;keyboardHold=true;repeat=true;nextRepeat=ms+450;model.beginGesture();dispatch(held);return;
        }
        if(k==Key::B)back();
        return;
    }
    if(k==Key::B){if(page==Page::Game)focusVisible=false;else back();return;}
    focusVisible=true;
    if(k==Key::A)activateFocus(ms);
    else if(k==Key::Up||k==Key::Down||k==Key::Left||k==Key::Right)moveFocus(k);
}
void App::keyUp(){pointerUp();}
void App::submitText(const std::string& value,bool accepted){
    InputKind kind=input.kind;input={};if(!accepted)return;
    if(kind==InputKind::Turn||kind==InputKind::TurnSetup||kind==InputKind::TimerSetup||kind==InputKind::TimerGame){
        bool turn=kind==InputKind::Turn||kind==InputKind::TurnSetup;int max=turn?9999:999,min=turn?0:1;
        if(value.empty()||value.size()>(turn?4u:3u)||value.find_first_not_of("0123456789")!=std::string::npos){message("Enter a valid whole number");return;}
        int n=std::atoi(value.c_str());if(n<min||n>max){message("Number is outside the allowed range");return;}
        if(kind==InputKind::TurnSetup){model.state.setup.startTurn=n;model.touch();}
        else if(turn)model.change(selected,Counter::Turn,n-model.state.game.turn);
        else if(kind==InputKind::TimerSetup){model.state.setup.timerSeconds=n*60;model.touch();}
        else{auto& g=model.state.game;g.config.timerSeconds=n*60;g.timerRemaining=n*60;g.timerDeadline=g.timerRunning?wallSeconds+g.timerRemaining:0;model.touch();}return;
    }
    if(kind==InputKind::SetLife){
        size_t first=!value.empty()&&value[0]=='-'?1:0;
        if(value.size()<=first||value.size()>6||value.find_first_not_of("0123456789",first)!=std::string::npos){message("Enter life from -9999 to 99999");return;}
        long n=std::strtol(value.c_str(),nullptr,10);if(n<MinLife||n>MaxLife){message("Enter life from -9999 to 99999");return;}
        int before=model.state.game.players[selected].life;soundTeam=-1;
        if(model.setLife(selected,static_cast<int>(n)))lifeSound(static_cast<int>(n)-before,int(Counter::Life));
        soundTeam=-1;return;
    }
    if(kind==InputKind::Life){if(value.empty()||value.size()>3||value.find_first_not_of("0123456789")!=std::string::npos){message("Enter a number from 1 to 999");return;}int n=std::atoi(value.c_str());if(n<1||n>999){message("Enter a number from 1 to 999");return;}model.state.setup.life=n;model.touch();return;}
    bool ok=false;
    if(kind==InputKind::NewProfile){ok=model.addProfile(value);if(ok){profile=static_cast<int>(model.state.profiles.size())-1;page=Page::Profile;}}
    else if(kind==InputKind::RenameProfile)ok=model.renameProfile(profile,value);
    else if(kind==InputKind::Preset)ok=model.savePreset(value);
    if(!ok)message("Use a name of 1-8 characters");else if(kind==InputKind::Preset)message("Preset saved");
}
std::string App::summary()const{
    const auto& g=model.state.game;if(!g.active)return "Lifeloch. Create a game or player profile.";
    std::string s;for(int i=0;i<g.config.players;++i){if(i)s+=". ";s+=g.players[i].name+": "+number(g.players[i].life)+" life";}return s;
}
}
