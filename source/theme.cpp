#include "theme.hpp"
#include "pixel.hpp"
#include <algorithm>
#include <array>
#include <cmath>
namespace deck {
namespace {
Color mix(Color a,Color b,float t){Color c=0;for(int shift:{16,8,0})c|=static_cast<Color>(((a>>shift)&255)*(1-t)+((b>>shift)&255)*t)<<shift;return c;}
}
Color magicColor(int i){static const Color colors[]={0xf4e5bb,0x427fe0,0x25262d,0xdb5746,0x46945d};return colors[std::clamp(i,0,4)];}
std::vector<ThemePoint> themePoints(int mask,float aspect){
    std::vector<int> colors;for(int i=0;i<5;++i)if(mask&(1<<i))colors.push_back(i);
    std::vector<ThemePoint> out;int n=colors.size();
    for(int i=0;i<n;++i){float x=.5f,y=.5f;
        if(n==2){x=i?1:0;}
        else if(n==4){const float corners[][2]={{0,0},{1,0},{1,1},{0,1}};x=corners[i][0];y=corners[i][1];}
        else if(n>=3){float angle=-1.570796327f+i*6.283185307f/n;x=.5f+(.48f/aspect)*std::cos(angle);y=.5f+.48f*std::sin(angle);}
        out.push_back({x,y,colors[i]});
    }return out;
}
Color themeSample(int mask,float x,float y,float aspect,uint64_t time,float speed){
    struct MotionPoint{float x,y;Color color;};
    struct Cache{
        int mask=-1,count=0;uint64_t frame=~uint64_t(0);float aspect=0,speed=-1;
        std::array<MotionPoint,5> points{};
        std::array<float,129> warpX{},warpY{};
        uint64_t lastTime=0;
        float previousSpeed=1.f;
        double orbitMs=0;
    };
    static Cache cache;
    speed=std::clamp(speed,1.f,2.f);
    // 50ms frame quantization is imperceptible with a 125s orbital period,
    // while avoiding thousands of trig calls per raster frame on 3DS.
    uint64_t frame=time/50;
    // Integrate speed over elapsed time: changes affect velocity, not position.
    // Never calculate phase from absoluteTime * currentSpeed; that jumps as
    // the user drags the slider. Use the speed before the current update.
    if(cache.lastTime&&time>=cache.lastTime)
        cache.orbitMs+=double(std::min<uint64_t>(time-cache.lastTime,5000))*cache.previousSpeed;
    cache.lastTime=time;
    cache.previousSpeed=speed;
    if(cache.mask!=mask||cache.frame!=frame||cache.aspect!=aspect||cache.speed!=speed){
        cache.mask=mask;cache.frame=frame;cache.aspect=aspect;cache.speed=speed;
        auto base=themePoints(mask,aspect);cache.count=static_cast<int>(base.size());
        constexpr float tau=6.28318530718f;
        float phase=tau*static_cast<float>(std::fmod(cache.orbitMs,125000.0)/125000.0);
        float cs=std::cos(phase),sn=std::sin(phase);
        for(int j=0;j<cache.count;++j){auto& pt=base[j];float ox=(pt.x-.5f)*aspect,oy=pt.y-.5f;
            cache.points[j]={ox*cs-oy*sn,ox*sn+oy*cs,magicColor(pt.color)};
        }
        for(int i=0;i<=128;++i){float u=i/128.f;
            // Multiple scales curl the pigment seams like ink drawn through water.
            cache.warpX[i]=.064f*std::sin(tau*2.1f*u+phase*.82f)+.026f*std::sin(tau*5.2f*u-phase*1.13f);
            cache.warpY[i]=.061f*std::sin(tau*1.8f*u-phase*.74f)+.024f*std::sin(tau*4.9f*u+phase*1.24f);
        }
    }
    if(cache.count==0)return magicColor(4);
    if(cache.count==1)return cache.points[0].color;
    auto interpolate=[](const std::array<float,129>& a,float u){float q=std::clamp(u*128.f,0.f,128.f);int i=std::min(int(q),127);return a[i]+(a[i+1]-a[i])*(q-i);};
    float px=(x-.5f)*aspect+interpolate(cache.warpX,y);
    float py=y-.5f+interpolate(cache.warpY,x);
    float first=1e9f,second=1e9f;Color a=0,b=0;
    for(int j=0;j<cache.count;++j){auto& node=cache.points[j];float dx=px-node.x,dy=py-node.y;
        float dist=std::sqrt(dx*dx+dy*dy);
        if(dist<first){second=first;b=a;first=dist;a=node.color;}
        else if(dist<second){second=dist;b=node.color;}}
    // Saturated pigments separated by rippling, narrow marbling seams.
    float t=std::clamp(.5f+(second-first)/.22f,.5f,1.f);
    t=t*t*(3.f-2.f*t);
    return mix(b,a,t);
}
void Renderer::gradient(Rect r,Color tl,Color tr,Color bl,Color br){
    // Portable fallback; native and browser adapters draw interpolated quads.
    for(float y=0;y<r.h;y+=6)for(float x=0;x<r.w;x+=6){float w=std::min(6.0f,r.w-x),h=std::min(6.0f,r.h-y);float u=(x+w/2)/r.w,v=(y+h/2)/r.h;
        box({r.x+x,r.y+y,w,h},mix(mix(tl,tr,u),mix(bl,br,u),v));}
}
Color ThemeRenderer::contrast(Color c,float x,float y)const{
    if(c!=0xf0f2df&&c!=0xa4b6af&&c!=0x82d8a2&&c!=0xfa9d8d&&c!=0xc48cf1&&c!=0xffda58&&c!=0xc8d0dc&&!(c==0xc5dc9b&&textMode))return c;
    Color sample=themeSample(mask,x/width,y/240,width/240,now,speed);
    // Relative luminance is evaluated on the actual panel color.
    Color surface=mix(0x0a1218,sample,.30f);float luminance=0;
    const float weights[]={.2126f,.7152f,.0722f};int k=0;
    for(int shift:{16,8,0}){float channel=((surface>>shift)&255)/255.0f;luminance+=weights[k++]*channel*channel;}
    bool bright=luminance>.30f;
    if(c==0xf0f2df)return bright?0x152025:c;
    if(c==0xa4b6af)return bright?0x38484b:0xd8ddd3;
    if(c==0xc5dc9b&&textMode)return bright?0x253e39:0xf7edcb;
    if(!bright)return c;
    if(c==0x82d8a2)return 0x175e3d;
    if(c==0xfa9d8d)return 0x8c262f;
    if(c==0xc48cf1)return 0x7628a4;
    if(c==0xffda58)return 0x886100;
    if(c==0xc8d0dc)return 0x566575;
    return c;
}
Color ThemeRenderer::textColor(Color c,float x,float y)const{
    // Resolve a complete glyph run once, so a gradient cannot split a numeral's ink.
    return contrast(c,x,y)^1u;
}
void ThemeRenderer::text(const std::string& s,float x,float y,float size,Color c,Align a,float w){
    textMode=true;Color ink=contrast(c,x+(a==Align::Left?w*.25f:(a==Align::Right?-w*.25f:0)),y+size/2);textMode=false;
    target.text(s,x,y,size,ink,a,w);
}
void ThemeRenderer::box(Rect r,Color c,float radius){
    Color adjusted=contrast(c,r.x+r.w/2,r.y+r.h/2);
    if(adjusted!=c||textMode){target.box(r,adjusted,radius);return;}
    float strength=0;Color base=0x0a1218;
    switch(c){
    case 0x101b1c:strength=.42f;break;
    case 0x19292a:strength=.30f;break;
    case 0x263a3b:strength=.24f;break;
    case 0x3b5350:strength=.38f;break;
    case 0xc5dc9b:strength=.68f;base=0xeeeece;break;
    default:target.box(r,c,radius);return;
    }
    auto sample=[&](float x,float y){return mix(base,themeSample(mask,x/width,y/240,width/240,now,speed),strength);};
    for(float y=r.y;y<r.y+r.h;y+=16)for(float x=r.x;x<r.x+r.w;x+=16){float w=std::min(16.0f,r.x+r.w-x),h=std::min(16.0f,r.y+r.h-y);
        target.gradient({x,y,w,h},sample(x,y),sample(x+w,y),sample(x,y+h),sample(x+w,y+h));}
}
}
