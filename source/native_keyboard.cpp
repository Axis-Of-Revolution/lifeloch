#ifdef __3DS__
extern "C" {
#include <3ds/applets/swkbd.h>
}
#include "native_keyboard.hpp"
#include "ui.hpp"

namespace deck {
void nativeKeyboard(App& app){
    const auto request=app.input;
    if(request.kind==InputKind::None)return;
    SwkbdState keyboard;char result[65]={0};
    const int digits=request.kind==InputKind::SetLife?6:(request.kind==InputKind::Turn||request.kind==InputKind::TurnSetup?4:3);
    swkbdInit(&keyboard,request.numeric?SWKBD_TYPE_NUMPAD:SWKBD_TYPE_NORMAL,2,request.numeric?digits:16);
    swkbdSetHintText(&keyboard,request.title.c_str());
    swkbdSetInitialText(&keyboard,request.initial.c_str());
    swkbdSetValidation(&keyboard,SWKBD_NOTEMPTY_NOTBLANK,0,0);
    if(request.numeric)swkbdSetNumpadKeys(&keyboard,request.kind==InputKind::SetLife?'-':0,0);
    swkbdSetButton(&keyboard,SWKBD_BUTTON_LEFT,"Cancel",false);
    swkbdSetButton(&keyboard,SWKBD_BUTTON_RIGHT,"Save",true);
    const auto button=swkbdInputText(&keyboard,result,sizeof(result));
    app.submitText(result,button==SWKBD_BUTTON_RIGHT);
}
}
#endif
