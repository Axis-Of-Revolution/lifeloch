// Tests the production adapter against real libctru declarations. Only the
// OS applet boundary is substituted; this does not emulate the 3DS keyboard UI.
extern "C" {
#include <3ds/applets/swkbd.h>
}
#include "native_keyboard.hpp"
#include "ui.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace deck;
namespace {
std::string reply,initial,hint;
SwkbdButton pressed=SWKBD_BUTTON_RIGHT;
int expectedType=SWKBD_TYPE_NORMAL,expectedLimit=16,calls=0;
void answer(App& app,const std::string& value,bool numeric=false,int limit=16,SwkbdButton button=SWKBD_BUTTON_RIGHT){
    reply=value;pressed=button;expectedType=numeric?SWKBD_TYPE_NUMPAD:SWKBD_TYPE_NORMAL;expectedLimit=limit;
    const auto original=app.input.initial;nativeKeyboard(app);assert(initial==original);assert(app.input.kind==InputKind::None);
}
}
// The declarations and inline configuration functions come from the SDK.
void swkbdInit(SwkbdState* keyboard,SwkbdType type,int count,int limit){
    std::memset(keyboard,0,sizeof(*keyboard));keyboard->type=type;keyboard->num_buttons_m1=count-1;keyboard->max_text_len=limit;
}
void swkbdSetHintText(SwkbdState*,const char* text){hint=text;}
void swkbdSetInitialText(SwkbdState*,const char* text){initial=text;}
void swkbdSetButton(SwkbdState* keyboard,SwkbdButton button,const char* text,bool submit){
    assert((button==SWKBD_BUTTON_LEFT&&std::string(text)=="Cancel"&&!submit)||(button==SWKBD_BUTTON_RIGHT&&std::string(text)=="Save"&&submit));
    keyboard->button_submits_text[button]=submit;
}
SwkbdButton swkbdInputText(SwkbdState* keyboard,char* buffer,size_t size){
    ++calls;assert(keyboard->type==expectedType&&keyboard->max_text_len==expectedLimit);
    assert(keyboard->num_buttons_m1==1&&keyboard->valid_input==SWKBD_NOTEMPTY_NOTBLANK);
    assert(keyboard->filter_flags==0&&keyboard->max_digits==0);
    assert(!keyboard->button_submits_text[SWKBD_BUTTON_LEFT]&&keyboard->button_submits_text[SWKBD_BUTTON_RIGHT]);
    assert(keyboard->numpad_keys[0]==(expectedLimit==6?'-':0)&&keyboard->numpad_keys[1]==0);
    assert(!hint.empty()&&size==65&&reply.size()<size);std::memcpy(buffer,reply.c_str(),reply.size()+1);return pressed;
}
int main(){
    App app;app.dispatch({Action::NewGame});app.dispatch({Action::Life});answer(app,"75",true,3);assert(app.model.state.setup.life==75);
    for(auto value:{"0","-1","1000","1.5",""}){app.dispatch({Action::Life});answer(app,value,true,3);assert(app.model.state.setup.life==75);}
    app.dispatch({Action::Life});answer(app,"999",true,3);assert(app.model.state.setup.life==999);
    app.dispatch({Action::Life});answer(app,"20",true,3,SWKBD_BUTTON_LEFT);assert(app.model.state.setup.life==999);
    app.dispatch({Action::Life});answer(app,"20",true,3,SWKBD_BUTTON_NONE);assert(app.model.state.setup.life==999);
    app.dispatch({Action::NewProfile});answer(app,"Zo\xc3\xab");assert(app.model.state.profiles.back().name=="Zo\xc3\xab");
    app.dispatch({Action::RenameProfile});assert(app.input.initial=="Zo\xc3\xab");answer(app,"1234567890123456");assert(app.model.state.profiles.back().name=="1234567890123456");
    app.dispatch({Action::RenameProfile});answer(app,"Cancelled",false,16,SWKBD_BUTTON_LEFT);assert(app.model.state.profiles.back().name=="1234567890123456");
    app.dispatch({Action::SavePreset});answer(app,"Friday Commander");assert(app.model.state.presets.back().name=="Friday Commander");
    app.dispatch({Action::Options});app.dispatch({Action::TimerInput});assert(app.input.initial=="50");answer(app,"25",true,3);assert(app.model.state.setup.timerSeconds==1500);
    app.dispatch({Action::TimerInput});answer(app,"0",true,3);assert(app.model.state.setup.timerSeconds==1500);
    app.dispatch({Action::TurnInput});answer(app,"9999",true,4);assert(app.model.state.setup.startTurn==9999);
    app.dispatch({Action::TurnInput});answer(app,"0",true,4);assert(app.model.state.setup.startTurn==0);
    assert(app.model.start());app.page=Page::Clock;app.wallSeconds=1800000000;
    app.dispatch({Action::TurnInput});answer(app,"137",true,4);assert(app.model.state.game.turn==137);
    app.dispatch({Action::TimerInput});answer(app,"50",true,3);assert(app.model.state.game.timerRemaining==3000);
    app.dispatch({Action::TimerInput});answer(app,"999",true,3);assert(app.model.state.game.timerRemaining==59940);
    app.page=Page::Game;app.dispatch({Action::SetLife});answer(app,"-42",true,6);assert(app.model.state.game.players[0].life==-42);
    for(const char* invalid:{"-10000","100000","1.5","--2","","-"}){app.dispatch({Action::SetLife});answer(app,invalid,true,6);assert(app.model.state.game.players[0].life==-42);}
    app.dispatch({Action::SetLife});answer(app,"99999",true,6);assert(app.model.state.game.players[0].life==99999);
    app.dispatch({Action::SetLife});answer(app,"0",true,6,SWKBD_BUTTON_LEFT);assert(app.model.state.game.players[0].life==99999);
    app.dispatch({Action::SetLife});answer(app,"0",true,6);assert(app.model.state.game.players[0].life==0);
    const int before=calls;nativeKeyboard(app);assert(calls==before);
    std::cout<<"PASS: native keyboard adapter with real libctru API headers: text/number selection, initial values, UTF-8 names, rename, presets, life/timer/turn entry, limits, Save/Cancel and dismissed applet. OS applet UI still requires hardware testing.\n";
}
