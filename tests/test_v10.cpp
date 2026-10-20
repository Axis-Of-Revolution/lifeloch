#include "ui.hpp"
#include "layout.hpp"
#include "pixel.hpp"
#include "storage.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
using namespace deck;
struct Capture:Renderer{
 int n=0,quads=0;struct Word{int n;std::string text;float x,y,size;};std::vector<Word> words;std::vector<std::pair<Rect,Color>> upper,lower;
 void screen(int s)override{n=s;}
 void box(Rect r,Color c,float=0)override{assert(std::isfinite(r.x+r.y+r.w+r.h));assert(r.w>=0&&r.h>=0&&r.x>=-1&&r.y>=-1&&r.x+r.w<=(n?321:401)&&r.y+r.h<=241);++quads;if(!n)upper.push_back({r,c});else lower.push_back({r,c});}
 void text(const std::string&s,float x,float y,float size,Color c,Align a,float w)override{words.push_back({n,s,x,y,size});pixelText(*this,s,x,y,size,c,a,w);}
 bool has(const std::string&s)const{return std::any_of(words.begin(),words.end(),[&](auto w){return w.text==s;});}
};
void render(App&a){Capture r;a.render(r);for(size_t i=0;i<a.buttons.size();i++)for(size_t j=i+1;j<a.buttons.size();j++){auto x=a.buttons[i].rect,y=a.buttons[j].rect;assert(std::min(x.x+x.w,y.x+y.w)<=std::max(x.x,y.x)||std::min(x.y+x.h,y.y+y.h)<=std::max(x.y,y.y));}}
std::string key(Command c){return std::to_string(int(c.action))+":"+std::to_string(c.a)+":"+std::to_string(c.b);}
void reachable(App&a){render(a);auto buttons=a.buttons;if(!a.buttonNavigation())a.keyDown(Key::Select,100);std::set<std::string> seen;for(size_t i=0;i<buttons.size()+2;i++){a.keyDown(Key::Right,110+i);a.keyUp();render(a);seen.insert(key(a.focusedCommand()));}for(auto b:buttons)if(b.enabled)assert(seen.count(key(b.command)));}
int main(){
 // Shared defeat uses a per-commander, per-teammate threshold; Giant poison is 15.
 for(int heads:{1,2,3}){Model m;m.state.setup={heads*2,40,true,true,false,heads};assert(m.start());auto&g=m.state.game;
  assert(m.poisonLimit()==(heads==1?10:15));m.change(heads-1,Counter::Poison,m.poisonLimit()-1);for(int i=0;i<heads;++i)assert(!m.atThreshold(i));m.change(0,Counter::Poison,1);for(int i=0;i<heads;++i)assert(m.atThreshold(i));assert(!m.atThreshold(heads));m.undo();
  g.players[heads-1].damage[heads]={20,20};for(int i=0;i<heads;++i)assert(!m.atThreshold(i));m.change(heads-1,Counter::Commander,1,heads,1);for(int i=0;i<heads;++i)assert(m.atThreshold(i));assert(!m.atThreshold(heads));g.config.partners=false;for(int i=0;i<heads;++i)assert(!m.atThreshold(i));g.players[heads-1].damage[heads][0]=21;for(int i=0;i<heads;++i)assert(m.atThreshold(i));
 }
 // Direction reversals are one net event, across teammates, with matching undo/redo.
 {Model m;m.state.setup={4,40,true,true,true,2};assert(m.start());auto&g=m.state.game;
  m.setInputTime(100);m.change(0,Counter::Life,4);m.setInputTime(1000);m.change(1,Counter::Life,-6);assert(g.history.size()==1&&g.history[0].before==40&&g.history[0].after==38);m.undo();assert(g.players[0].life==40&&g.history.empty());m.redo();assert(g.players[1].life==38);m.setInputTime(1500);m.change(0,Counter::Life,1);assert(g.history.size()==2); // undo/redo ends a batch
  m.setInputTime(4000);m.change(0,Counter::Poison,2);m.setInputTime(4200);m.change(1,Counter::Poison,-1);m.setInputTime(4500);m.change(0,Counter::Poison,2);assert(g.history.back().kind==HistoryKind::Poison&&g.history.back().before==0&&g.history.back().after==3);assert(g.players[1].poison==3);
  auto saved=encode(m.state);State r;uint32_t sequence;assert(decode(saved,r,sequence)&&r.game.history==g.history);m.undo();assert(g.players[0].poison==0);m.redo();assert(g.players[0].poison==3);
  m.setInputTime(7000);m.change(0,Counter::Life,1);m.setInputTime(7100);m.change(1,Counter::Life,-1);assert(g.history.size()==3); // exact reversal removes the empty event
  m.setInputTime(10000);m.change(0,Counter::Poison,1);m.setInputTime(12000);m.change(0,Counter::Poison,1);assert(g.history.size()==5); // the gap boundary is exclusive
 }
 // Touch and hardware shortcuts visibly flash the actual adjustment button.
 {App a;a.model.state.setup={4,40,true,true,true};a.model.start();a.page=Page::Game;
  for(int tab:{0,1,2}){a.tab=tab;a.tick(20000+tab*1000);render(a);int kind=int(tab==0?Counter::Life:tab==1?Counter::Commander:Counter::Poison);auto found=std::find_if(a.buttons.begin(),a.buttons.end(),[&](const auto&b){return b.command==Command{Action::Change,1,kind};});assert(found!=a.buttons.end());auto rect=found->rect;a.pointerDown(rect.x+rect.w/2,rect.y+rect.h/2,20000+tab*1000);a.pointerUp();Capture flash;a.render(flash);assert(std::any_of(flash.lower.begin(),flash.lower.end(),[](auto b){return b.second==0x59736a;}));a.tick(20200+tab*1000);Capture idle;a.render(idle);assert(std::none_of(idle.lower.begin(),idle.lower.end(),[](auto b){return b.second==0x59736a;}));a.keyDown(Key::B,20210+tab*1000);a.keyUp();Capture keyFlash;a.render(keyFlash);assert(std::any_of(keyFlash.lower.begin(),keyFlash.lower.end(),[](auto b){return b.second==0x59736a;}));}
 }
 // Poison uses the same hit samples: net +3 is critical; removal is silent.
 {App a;a.model.start();a.page=Page::Game;auto poison=[&](int n,uint64_t t){a.tick(t);a.dispatch({Action::Change,n,int(Counter::Poison)});return a.takeSounds();};assert(poison(1,1000)==std::vector<Sound>{Sound::Tick});assert(poison(1,1100)==std::vector<Sound>{Sound::Tick});assert(poison(-1,1200).empty());assert(poison(1,1300)==std::vector<Sound>{Sound::Tick});assert(poison(1,1400)==std::vector<Sound>{Sound::Impact});assert(poison(3,5000)==std::vector<Sound>{Sound::Impact});}
 // A level yaw rotation wakes every settled solid; stopping lets it settle naturally.
 for(int sides:{2,4,6,8,10,12,20})for(float yaw:{-3.f,3.f}){RollSimulation body;body.start(sides,12345);body.advance(4);assert(body.pose().settled);auto start=body.pose().position;body.advance(.7f,0,0,yaw);auto end=body.pose().position;assert(std::hypot(start.x-end.x,start.y-end.y)>.05f&&!body.pose().settled);body.advance(4);assert(body.pose().settled);if(body.pose().result)assert(body.pose().result==body.faceUp());}
 {RollSimulation body;body.start(6,12345);body.advance(4);auto p=body.pose();body.advance(2,0,0,.02f);assert(body.pose().settled&&body.pose().position.x==p.position.x&&body.pose().rotation==p.rotation);}
 // Exact glyph ink is uniform and the outline follows the letters, not a backing box.
 {App a;a.model.addProfile("Alan");a.model.colorProfile(6,1);a.model.state.seats[0]=6;a.model.start();a.page=Page::Game;Capture c;a.render(c);assert(std::any_of(c.upper.begin(),c.upper.end(),[](auto box){return box.second==0xfff4da;}));}
 std::cout<<"PASS: v0.10 Giant defeat/poison, typed net history/save, feedback, poison audio and bidirectional yaw motion.\n";

 assert(defaults().presets.empty());
 for(int n=2;n<=6;n++)for(int k=0;k<layoutCount(n);k++){auto panels=playerPanels(n,k);Config c;c.players=n;chooseLayout(c,k);assert(validConfig(c));if(n==5&&k<2)for(float y:{20.f,125.f}){float width=-1;for(int i=0;i<n;i++)if(panels[i].y==y){if(width<0)width=panels[i].w;assert(std::abs(width-panels[i].w)<.001f);}}if(n==6&&k==0)for(int i=0;i<n;i++)assert(panels[i].h==99&&c.rotations[i]==(panels[i].y==20?2:0));}
 App a;for(int i=0;i<10;i++)assert(a.model.addProfile("Named "+std::to_string(i)));assert(!a.model.addProfile("Eleventh"));a.model.state.setup={6,40,true,true,true};chooseLayout(a.model.state.setup,0);assert(a.model.start());auto&g=a.model.state.game;a.page=Page::Game;g.players[0].damage[2][1]=18;g.timerRunning=true;g.timerDeadline=1800001000;g.timerRemaining=1000;
 a.dispatch({Action::Clock});a.dispatch({Action::Partners});assert(!g.config.partners&&g.players[0].damage[2][1]==18);a.dispatch({Action::Partners});assert(g.config.partners);a.dispatch({Action::Commander});a.dispatch({Action::Partners});assert(g.config.partners);a.dispatch({Action::Commander});assert(g.players[0].damage[2][1]==18);a.dispatch({Action::Back});
 a.dispatch({Action::GameLayout});render(a);assert(std::none_of(a.buttons.begin(),a.buttons.end(),[](auto b){return b.command.action==Action::PlayerCount;}));auto before=g;a.dispatch({Action::Layout,1});a.dispatch({Action::PlayerCount,2});assert(g.config.players==6&&g.config.layout==1);before.config=g.config;assert(before==g);assert(g.timerRunning&&g.timerDeadline==1800001000);a.dispatch({Action::Back});
 a.dispatch({Action::Profiles});assert(a.page==Page::ActiveSeats);a.model.change(0,Counter::Life,-3);before=g;a.dispatch({Action::PickSeat,0});a.dispatch({Action::Profile,15});assert(a.page==Page::ActiveSeats&&g.players[0].name=="Named 9"&&g.players[0].life==37&&g.players[0].damage[2][1]==18);a.model.undo();assert(g.players[0].life==40&&g.players[0].name=="Named 9");a.model.redo();assert(g.players[0].life==37&&g.players[0].name=="Named 9");a.dispatch({Action::PickSeat,0});a.dispatch({Action::Profile,-1});assert(g.players[0].name=="Player A"&&!g.players[0].profileUsed);a.dispatch({Action::ManageProfiles});a.dispatch({Action::Profile,6});render(a);assert(std::count_if(a.buttons.begin(),a.buttons.end(),[](auto b){return b.command.action==Action::Color;})==10);
 Player e,f;e.color=4;f.color=5;assert(playerColors(e)[0]==0x81738e&&playerColors(f)[0]==0xd3dae2);
 // No-touch paths across all settings and every enabled button on each page.
 for(auto p:{Page::Home,Page::Rules,Page::Options,Page::Seats,Page::Profiles,Page::Profile,Page::Presets,Page::Game,Page::History,Page::Menu,Page::Help,Page::Dice,Page::Layout,Page::Clock,Page::GameTrackers,Page::ActiveSeats,Page::Theme,Page::Float}){a.page=p;reachable(a);}
 a.page=Page::Home;render(a);a.keyDown(Key::A,1000);a.keyUp();assert(a.page==Page::Home&&a.modalOpen());a.dispatch({Action::Cancel});assert(!a.modalOpen());a.dispatch({Action::NewGame});assert(a.modalOpen());a.dispatch({Action::Confirm});assert(a.page==Page::Rules&&!a.modalOpen());render(a);a.keyDown(Key::B,1010);assert(a.page==Page::Game);render(a);if(a.buttonNavigation())a.keyDown(Key::Select,1020);int life=g.players[a.selected].life;a.keyDown(Key::A,1100);a.keyUp();assert(g.players[a.selected].life==life+1);
 a.page=Page::Game;a.model.state.game.config.players=2;chooseLayout(g.config,0);g.config.commander=false;g.config.heads=1;g.history.clear();for(int i=0;i<16;i++)g.history.push_back({0,0,40,39,-1,0,static_cast<uint32_t>(i)});Capture history;a.render(history);int shown=0;for(auto w:history.words)if(w.n==0&&w.text=="-1→39")shown++;assert(shown>=10);
 assert(!a.model.atThreshold(0));g.players[0].poison=9;assert(!a.model.atThreshold(0));g.players[0].poison=10;assert(a.model.atThreshold(0));
 // A history-bearing game must be confirmed at New Game, and confirmation
 // must not be requested again when finishing its setup.
 {App replace;assert(replace.model.start());replace.model.change(0,Counter::Life,-2);auto old=replace.model.state.game;
  replace.dispatch({Action::NewGame});assert(replace.modalOpen());replace.dispatch({Action::Cancel});assert(replace.model.state.game==old);
  replace.dispatch({Action::NewGame});replace.dispatch({Action::Confirm});assert(!replace.modalOpen()&&replace.page==Page::Rules);
  replace.dispatch({Action::Begin});assert(!replace.modalOpen()&&replace.page==Page::Game&&replace.model.state.game.history.empty());}
 // Events retain the actual turn number, including after serialization, and
 // both history modes render a separator when the turn counter is enabled.
 {App timeline;timeline.model.state.setup.turnEnabled=true;assert(timeline.model.start());
  timeline.page=Page::Game;auto& gg=timeline.model.state.game;
  assert(timeline.model.change(0,Counter::Life,-2));
  assert(gg.history.back().turn==0);
  timeline.dispatch({Action::QuickTurn,1});
  assert(timeline.model.change(0,Counter::Life,-3));
  assert(gg.history.back().turn==1);
  State restored;uint32_t seq=0;assert(decode(encode(timeline.model.state),restored,seq));
  assert(restored.game.history[0].turn==0&&restored.game.history[1].turn==1);
  timeline.dispatch({Action::History});Capture all;timeline.render(all);
  assert(all.has("Game history")&&all.has("TURN 1")&&all.has("TURN 0"));
  timeline.dispatch({Action::SelectPlayer,0});Capture single;timeline.render(single);
  assert(single.has("TURN 1")&&single.has("TURN 0"));
  gg.config.turnEnabled=false;Capture hidden;timeline.render(hidden);
  assert(!hidden.has("TURN 1")&&!hidden.has("TURN 0"));
 }
 // Loop boundary is exact in native flame rasterization (every 2400 ms).
 {Capture a,b;lifeText(a,10,6,24,30,0xffed98,Align::Left,165,0,20,true,false,10,false,40);
  lifeText(b,10,6,24,30,0xffed98,Align::Left,165,2400,20,true,false,10,false,40);
  assert(a.upper==b.upper);}
 // The commander box leaves the history column clear on a two-seat board.
 g.config.commander=true;Capture withCommander;a.render(withCommander);
 assert(std::count_if(withCommander.words.begin(),withCommander.words.end(),[](auto w){return w.n==0&&w.text=="-1→39";})>=10);
 g.config.commander=false;
 // The visual transition never delays the authoritative life total.
 a.tick(10000);render(a);a.dispatch({Action::Change,-11,int(Counter::Life)});int target=g.players[0].life;render(a);a.tick(10100);Capture rolling;a.render(rolling);assert(g.players[0].life==target);a.tick(10500);render(a);
 // Profile creation includes color before assigning; edits update every linked identity.
 App profiles;profiles.dispatch({Action::NewGame});assert(profiles.model.state.setup.themeColors==31);profiles.dispatch({Action::Players});profiles.dispatch({Action::PlayerCount,3});assert(profiles.model.state.setup.commander&&profiles.model.state.setup.life==40);profiles.dispatch({Action::PlayerCount,2});assert(!profiles.model.state.setup.commander&&profiles.model.state.setup.life==20);profiles.dispatch({Action::Seats});profiles.dispatch({Action::PickSeat,0});profiles.dispatch({Action::NewProfile});profiles.submitText("Zara",true);assert(profiles.page==Page::Profile&&profiles.model.state.seats[0]==-1);profiles.dispatch({Action::Color,7});profiles.dispatch({Action::ProfileDone});assert(profiles.page==Page::Seats&&profiles.model.state.seats[0]==6);profiles.dispatch({Action::Begin});auto& pg=profiles.model.state.game;assert(pg.players[0].profileIndex==6&&pg.players[0].color==7);profiles.model.change(0,Counter::Life,-3);assert(profiles.model.renameProfile(6,"Zelda")&&profiles.model.colorProfile(6,1));assert(pg.players[0].name=="Zelda"&&pg.players[0].color==1);profiles.model.undo();assert(pg.players[0].name=="Zelda"&&pg.players[0].color==1);profiles.model.redo();assert(pg.players[0].profileIndex==6);State persisted;uint32_t seq;assert(decode(encode(profiles.model.state),persisted,seq)&&persisted.game.players[0].profileIndex==6);auto oldV6=encode(profiles.model.state);oldV6.resize(oldV6.size()-24-8*profiles.model.state.game.history.size());auto put=[&](int offset,uint32_t value){for(int i=0;i<4;++i)oldV6[offset+i]=(value>>(i*8))&255;};put(8,6);put(16,oldV6.size()-24);put(20,0);uint32_t checksum=0xffffffff;for(auto byte:oldV6){checksum^=byte;for(int i=0;i<8;++i)checksum=(checksum>>1)^(0xedb88320u&(0u-(checksum&1)));}put(20,~checksum);assert(decode(oldV6,persisted,seq));assert(persisted.game.players[0].profileIndex==6);
 profiles.model.addProfile("Second");profiles.model.assignActive(1,7);profiles.model.deleteProfile(6);assert(pg.players[1].profileIndex==6);profiles.model.renameProfile(6,"Second updated");assert(pg.players[1].name=="Second updated");assert(pg.players[0].profileIndex==-1&&pg.players[0].name=="Zelda");profiles.model.undo();assert(pg.players[0].profileIndex==-1);
 profiles.dispatch({Action::Clock});render(profiles);for(auto action:{Action::Commander,Action::Partners,Action::TurnToggle,Action::TimerToggle,Action::ClockToggle})assert(std::any_of(profiles.buttons.begin(),profiles.buttons.end(),[&](const auto& b){return b.command.action==action;}));assert(std::none_of(profiles.buttons.begin(),profiles.buttons.end(),[](const auto&b){return b.command.action==Action::GameTrackers;}));bool clock=pg.config.clockEnabled;profiles.dispatch({Action::ClockToggle});assert(pg.config.clockEnabled!=clock);
 pg.config.commander=pg.config.partners=true;pg.players[0].life=40;pg.players[0].damage[1]={20,20};assert(!profiles.model.atThreshold(0));pg.players[0].damage[1][1]=21;assert(profiles.model.atThreshold(0));pg.config.partners=false;assert(!profiles.model.atThreshold(0));pg.players[0].damage[1][0]=21;assert(profiles.model.atThreshold(0));profiles.page=Page::Game;Capture death;profiles.render(death);assert(std::any_of(death.upper.begin(),death.upper.end(),[](auto box){return box.second==0x777b82;}));
 // A large deliberate tilt must move a settled body; leveling must let it settle again.
 for(int sides:{2,4,6,8,10,12,20}){RollSimulation body;body.start(sides,987654);body.advance(4);assert(body.pose().settled);auto p=body.pose().position;body.advance(.5f,.65f,.2f);auto moved=body.pose().position;assert(std::hypot(moved.x-p.x,moved.y-p.y)>.05f);body.advance(4);assert(body.pose().settled);if(body.pose().result)assert(body.pose().result==body.faceUp());}
 assert(pixelAdvance("1→2")==39);
 for(auto pair:{std::pair<int,int>{4,2},{6,2},{6,3}})for(int layout=0;layout<2;++layout){auto lines=teamDividers(pair.first,layout,pair.second);auto cards=playerPanels(pair.first,layout);assert(!lines.empty());for(auto line:lines)for(int i=0;i<pair.first;++i){auto p=cards[i];assert(std::min(line.x+line.w,p.x+p.w)<=std::max(line.x,p.x)||std::min(line.y+line.h,p.y+p.h)<=std::max(line.y,p.y));}}
 for(auto pair:{std::pair<int,int>{4,2},{6,2},{6,3}})for(int layout:{0,1}){auto lines=teamDividers(pair.first,layout,pair.second);for(auto a:lines)if(a.h==2)for(auto b:lines)if(b.w==2){float x=b.x+1,y=a.y+1;if(x<a.x||x>a.x+a.w||y<b.y||y>b.y+b.h)continue;for(float dx:{-.9f,.9f})for(float dy:{-.9f,.9f})assert(std::any_of(lines.begin(),lines.end(),[&](auto r){return r.contains(x+dx,y+dy);}));}}
 assert(teamDividers(6,0,3).size()==1);assert(teamDividers(4,0,2).size()==1);assert(teamDividers(6,1,1).empty());
 int maximum=0,tiltChanged=0,cocked=0;float longest=0;
 for(int sides:{2,4,6,8,10,12,20}){std::set<int> outcomes;for(int i=1;i<=120;i++){
   RollSimulation body;body.start(sides,i*67879);assert(body.pose().result==0&&!body.pose().settled);for(int t=0;t<240&&!body.pose().settled;t++)body.advance(1.f/60);auto p=body.pose();assert(p.settled);longest=std::max(longest,body.seconds());if(p.result==0)++cocked;else{assert(p.result>=1&&p.result<=sides&&p.result==body.faceUp());outcomes.insert(p.result);}auto q=p.rotation;body.advance(1);assert(body.pose().rotation==q);
   if(i<12){RollSimulation tilted;tilted.start(sides,i*67879);tilted.advance(.5f,.25f,-.15f);RollSimulation flat;flat.start(sides,i*67879);flat.advance(.5f);auto x=tilted.pose().position,y=flat.pose().position;assert(std::abs(x.x-y.x)+std::abs(x.y-y.y)>.01);tilted.advance(3.5f);assert(tilted.pose().settled);if(tilted.pose().result!=p.result)++tiltChanged;}
  }assert(outcomes.size()>=static_cast<size_t>(sides==2?2:sides-2));
  a.page=Page::Dice;a.tick(20000+sides*10000);a.dispatch({Action::Die,sides});a.dispatch({Action::Roll});assert(a.dieResult==0&&a.rolling);for(int t=0;t<=4000;t+=40){a.tick(20000+sides*10000+t);Capture frame;a.render(frame);maximum=std::max(maximum,frame.quads);assert(frame.quads<32768);}assert(!a.rolling);
 }
 assert(tiltChanged>0);for(auto sound:{Sound::DiceShake,Sound::DiceTap,Sound::CoinClink}){auto pcm=soundPcm(sound);assert(std::any_of(pcm.begin(),pcm.end(),[](auto x){return x!=0;}));}
 auto bytes=encode(a.model.state);State restored;uint32_t sequence;assert(decode(bytes,restored,sequence));assert(encode(restored)==bytes);std::ifstream fixture("tests/fixtures/v06-save.bin",std::ios::binary);std::vector<uint8_t> old((std::istreambuf_iterator<char>(fixture)),{});assert(decode(old,restored,sequence));assert(sequence==42&&restored.setup.layout==3&&restored.game.config.layout==3&&restored.game.players[0].damage[2][1]==17&&restored.game.players[1].name=="Brenda");assert(restored.presets.empty());
 std::cout<<"PASS: native menus/navigation, partners/layout/profile preservation, poison death, rolling life, expanded history, save v9/migration; 840 physical rolls, longest "<<longest<<"s, cocked "<<cocked<<", tilt changed "<<tiltChanged<<" outcomes. Max quads "<<maximum<<".\n";
}
