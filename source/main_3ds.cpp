#ifdef __3DS__
#include <3ds.h>
#include <citro2d.h>
#include "ui.hpp"
#include "pixel.hpp"
#include "storage.hpp"
#include "native_keyboard.hpp"
#include "home_art.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <sys/stat.h>

namespace {
deck::App app;
deck::Storage storage("sdmc:/3ds/command-deck");
uint32_t savedRevision=0;
bool initialized=false;
bool persist(){
    app.pointerUp();
    if(app.saveBlocked)return false;
    app.saveOk=storage.save(app.model.state);
    if(app.saveOk)savedRevision=app.model.revision;
    return app.saveOk;
}
void lifecycle(APT_HookType type,void*) {
    if(initialized&&(type==APTHOOK_ONSUSPEND||type==APTHOOK_ONSLEEP||type==APTHOOK_ONEXIT))persist();
}
uint32_t color(deck::Color c){return C2D_Color32((c>>16)&255,(c>>8)&255,c&255,255);}
class NativeAudio {
    struct Sample {int16_t* data=nullptr;uint32_t count=0;};
    Sample samples[deck::SoundCount];ndspWaveBuf waves[12]{};int next=0;bool ready=false;
public:
    NativeAudio(){
        ready=R_SUCCEEDED(ndspInit());if(!ready)return;
        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        for(int i=0;i<deck::SoundCount;++i){auto pcm=deck::soundPcm(static_cast<deck::Sound>(i));samples[i].count=pcm.size();samples[i].data=static_cast<int16_t*>(linearAlloc(pcm.size()*sizeof(int16_t)));
            if(samples[i].data){std::copy(pcm.begin(),pcm.end(),samples[i].data);DSP_FlushDataCache(samples[i].data,pcm.size()*sizeof(int16_t));}}
        for(int i=0;i<12;++i){ndspChnReset(i);ndspChnSetInterp(i,NDSP_INTERP_POLYPHASE);ndspChnSetRate(i,deck::SoundRate);ndspChnSetFormat(i,NDSP_FORMAT_MONO_PCM16);float mix[12]={.65f,.65f};ndspChnSetMix(i,mix);}
    }
    ~NativeAudio(){if(ready){for(int i=0;i<12;++i)ndspChnWaveBufClear(i);ndspExit();for(auto& s:samples)if(s.data)linearFree(s.data);}}
    void play(deck::Sound sound){
        if(!ready)return;
        auto& sample=samples[static_cast<int>(sound)];if(!sample.data)return;
        int channel=next++%12;ndspChnWaveBufClear(channel);auto& wave=waves[channel];wave={};wave.data_vaddr=sample.data;wave.nsamples=sample.count;ndspChnWaveBufAdd(channel,&wave);
    }
};
class NativeRenderer final:public deck::Renderer {
    C3D_RenderTarget* targets[2];
    C2D_TextBuf textBuffer;
    C3D_Tex homeTexture{};
    Tex3DS_SubTexture homeSub{320,192,0.0f,1.0f,320.0f/512,1.0f-192.0f/256};
    bool homeReady=false;
public:
    NativeRenderer(){
        targets[0]=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);targets[1]=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);textBuffer=C2D_TextBufNew(4096);
        homeReady=C3D_TexInit(&homeTexture,512,256,GPU_RGB565);
        if(homeReady){
            auto* texture=static_cast<uint16_t*>(homeTexture.data);std::fill_n(texture,512*256,0);
            // Render at 320x192: 25% larger pixels on the 400x240 screen.
            // PICA stores each 8x8 tile in Morton order. Image row zero is at v=1.
            for(unsigned y=0;y<192;++y)for(unsigned x=0;x<320;++x){unsigned morton=0;for(unsigned bit=0;bit<3;++bit){morton|=((x>>bit)&1)<<(2*bit);morton|=((y>>bit)&1)<<(2*bit+1);}unsigned offset=((y/8)*64+x/8)*64+morton;// Grid-lock the original 400x240 artwork to ~200x120 texels.
                unsigned srcX=((x*5+2)/4)/2*2,srcY=((y*5+2)/4)/2*2;
                texture[offset]=deck::homeArtPixels[std::min(srcY,239u)*400+std::min(srcX,399u)];}
            C3D_TexSetFilter(&homeTexture,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(&homeTexture,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);C3D_TexFlush(&homeTexture);
        }
    }
    ~NativeRenderer(){if(homeReady)C3D_TexDelete(&homeTexture);C2D_TextBufDelete(textBuffer);}
    void homeArtwork(deck::Rect area)override{if(homeReady)C2D_DrawImageAt({&homeTexture,&homeSub},area.x,area.y,0,nullptr,area.w/320,area.h/192);}
    void begin(){C2D_TextBufClear(textBuffer);}
    void screen(int which)override{C2D_TargetClear(targets[which],C2D_Color32(16,27,28,255));C2D_SceneBegin(targets[which]);}
    void box(deck::Rect r,deck::Color c,float=0)override{C2D_DrawRectSolid(r.x,r.y,0,r.w,r.h,color(c));}
    void gradient(deck::Rect r,deck::Color tl,deck::Color tr,deck::Color bl,deck::Color br)override{C2D_DrawRectangle(r.x,r.y,0,r.w,r.h,color(tl),color(tr),color(bl),color(br));}
    void text(const std::string& s,float x,float y,float size,deck::Color c,deck::Align align,float width)override{
        bool unicode=!deck::pixelCompatible(s);
        if(!unicode){deck::pixelText(*this,s,x,y,size,c,align,width);return;}
        C2D_Text t;C2D_TextParse(&t,textBuffer,s.c_str());C2D_TextOptimize(&t);float scale=size/30,w=0,h=0;
        C2D_TextGetDimensions(&t,scale,scale,&w,&h);if(width>0&&w>width){scale*=width/w;w=width;}
        if(align==deck::Align::Center)x-=w/2;else if(align==deck::Align::Right)x-=w;
        C2D_DrawText(&t,C2D_WithColor,x,y,0,scale,scale,color(c));
    }
};
}
int main(){
    gfxInitDefault();gfxSet3D(false);
    bool accelerometerReady=R_SUCCEEDED(HIDUSER_EnableAccelerometer());
    bool gyroscopeEnabled=R_SUCCEEDED(HIDUSER_EnableGyroscope());float gyroUnitsPerDps=0;
    bool gyroscopeReady=gyroscopeEnabled&&R_SUCCEEDED(HIDUSER_GetGyroscopeRawToDpsCoefficient(&gyroUnitsPerDps))&&std::isfinite(gyroUnitsPerDps)&&gyroUnitsPerDps>0;
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE*2)){gfxExit();return 1;}
    if(!C2D_Init(32768)){C3D_Fini();gfxExit();return 1;}
    C2D_Prepare();
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/command-deck",0777);
    auto loaded=storage.load(app.model.state);app.saveBlocked=loaded==deck::LoadResult::Unreadable;app.saveOk=!app.saveBlocked;
    app.randomState=static_cast<uint32_t>(svcGetSystemTick())^static_cast<uint32_t>(osGetTime());
    aptHookCookie hook;aptHook(&hook,lifecycle,nullptr);initialized=true;
    {
        NativeAudio audio;NativeRenderer renderer;uint64_t lastSave=0;uint32_t heldKey=0;
        while(aptMainLoop()&&!app.quitRequested){
            uint64_t now=osGetTime();time_t wall=time(nullptr);app.wallSeconds=static_cast<uint64_t>(wall);
            char clock[8];tm* parts=localtime(&wall);strftime(clock,sizeof(clock),"%H:%M",parts);app.currentClock=clock;
            hidScanInput();
            if(accelerometerReady){accelVector acceleration;hidAccelRead(&acceleration);float magnitude=std::sqrt(float(acceleration.x)*acceleration.x+float(acceleration.y)*acceleration.y+float(acceleration.z)*acceleration.z);if(magnitude>1)app.setTilt(acceleration.x/magnitude,-acceleration.y/magnitude);}
            if(gyroscopeReady){angularRate rate;hidGyroRead(&rate);app.setTrayRotation(rate.z/gyroUnitsPerDps*.01745329252f);}
            app.tick(now);
            for(auto sound:app.takeSounds())audio.play(sound);
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);renderer.begin();app.render(renderer);C3D_FrameEnd(0);
            uint32_t down=hidKeysDown(),up=hidKeysUp(),held=hidKeysHeld();
            touchPosition touch;hidTouchRead(&touch);
            if(down&KEY_TOUCH)app.pointerDown(touch.px,touch.py,now);
            else if(held&KEY_TOUCH)app.pointerMove(touch.px,touch.py);
            if(up&KEY_TOUCH)app.pointerUp();
            const struct {uint32_t bit;deck::Key key;} keys[]={{KEY_START,deck::Key::Start},{KEY_SELECT,deck::Key::Select},{KEY_L,deck::Key::L},{KEY_R,deck::Key::R},{KEY_LEFT,deck::Key::Left},{KEY_RIGHT,deck::Key::Right},{KEY_UP,deck::Key::Up},{KEY_DOWN,deck::Key::Down},{KEY_A,deck::Key::A},{KEY_B,deck::Key::B},{KEY_X,deck::Key::X},{KEY_Y,deck::Key::Y}};
            for(auto k:keys)if(down&k.bit){app.keyDown(k.key,now);heldKey=k.bit;break;}
            if(heldKey&&(up&heldKey)){app.keyUp();heldKey=0;}
            if(app.input.kind!=deck::InputKind::None){persist();deck::nativeKeyboard(app);}
            if(app.model.revision!=savedRevision&&now-lastSave>=500&&!(held&(KEY_TOUCH|KEY_A|KEY_B|KEY_UP|KEY_DOWN))){persist();lastSave=now;}
            if(app.quitRequested&&!app.exitWithoutSave&&!persist()){app.quitRequested=false;app.page=deck::Page::Menu;}
        }
        persist();
    }
    initialized=false;aptUnhook(&hook);if(accelerometerReady)HIDUSER_DisableAccelerometer();if(gyroscopeEnabled)HIDUSER_DisableGyroscope();C2D_Fini();C3D_Fini();gfxExit();return 0;
}
#endif
