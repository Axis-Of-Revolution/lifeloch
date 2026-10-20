#include "pixel.hpp"
#include "medieval_atlas.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace deck {
namespace {
void line(Renderer& r,int x0,int y0,int x1,int y1,int step,Color c){
    int dx=std::abs(x1-x0),sx=x0<x1?1:-1,dy=-std::abs(y1-y0),sy=y0<y1?1:-1,e=dx+dy;
    for(;;){r.box({float(x0*step),float(y0*step),float(step),float(step)},c);if(x0==x1&&y0==y1)break;int e2=2*e;if(e2>=dy){e+=dy;x0+=sx;}if(e2<=dx){e+=dx;y0+=sy;}}
}
}
bool pixelCompatible(const std::string& s){
    for(size_t i=0;i<s.size();++i)if(static_cast<unsigned char>(s[i])>=128){if(s.compare(i,3,"→")==0)i+=2;else return false;}
    return true;
}
float pixelAdvance(const std::string& s){float total=0;for(size_t i=0;i<s.size();++i){if(s.compare(i,3,"→")==0){total+=13;i+=2;}else total+=medieval::widths[std::clamp(int(static_cast<unsigned char>(s[i])),32,126)-32]+1;}return total;}
void pixelText(Renderer& r,const std::string& s,float x,float y,float size,Color c,Align align,float maxWidth){
    // Raster serif atlas: keeps the handcrafted, pixel-stepped resolution on the 3DS
    // and in the browser while giving ordinary labels proper medieval serifs.
    struct Glyph{int id;float advance;};std::vector<Glyph> run;
    float unit=size/22.0f,total=0;
    for(size_t i=0;i<s.size();){
        unsigned char ch=s[i++];
        if(ch>127){bool arrow=ch==0xe2&&i+1<s.size()&&static_cast<unsigned char>(s[i])==0x86&&static_cast<unsigned char>(s[i+1])==0x92;
            while(i<s.size()&&(static_cast<unsigned char>(s[i])&0xc0)==0x80)++i;
            ch=arrow?127:'?';}
        int id=ch==127?95:std::clamp(int(ch),32,126)-32;
        float adv=(id==95?13.0f:(medieval::widths[id]+1.0f))*unit;
        run.push_back({id,adv});total+=adv;
    }
    float scale=(maxWidth>0&&total>maxWidth)?maxWidth/total:1.0f;
    unit*=scale;total*=scale;
    if(align==Align::Center)x-=total/2;else if(align==Align::Right)x-=total;
    for(auto glyph:run){
        int id=glyph.id;
        if(id==95){ // Real right arrow, not a greater-than substitution.
            r.box({x+1.5f*unit,y+6.25f*unit,7*unit,1.5f*unit},c);
            r.box({x+7*unit,y+3.5f*unit,1.5f*unit,7*unit},c);
            r.box({x+8.5f*unit,y+5*unit,1.5f*unit,4*unit},c);
            r.box({x+10*unit,y+6.25f*unit,unit,1.5f*unit},c);
            x+=13*unit;continue;
        }
        int w=medieval::widths[id];
        for(int row=0;row<medieval::height;++row){uint32_t mask=medieval::rows[id][row];for(int col=0;col<w;){
            if(!(mask&(1u<<col))){++col;continue;}
            int begin=col;do{++col;}while(col<w&&(mask&(1u<<col)));
            r.box({x+begin*unit,y+row*unit,(col-begin)*unit,unit},c);
        }}
        x+=(w+1)*unit;
    }
}

namespace {
const char* skullRows[]={"00000011111111000000","00001111111111110000","00011111111111111000","00011100111100111000","00011100111100111000","00011111100111111000","00001111100111110000","00000111111111100000","00000010101010000000","00000011111110000000","01100000000000000110","11110000000000001111","00111100000000111100","00001111000011110000","00000011111111000000","00001111000011110000","01111100000000111110","11110000000000001111"};
class NumeralEffect final:public Renderer {
    Renderer& target;Rect bounds;int life;uint64_t time;float step;bool intense;
    Color sample(float x,float y)const{
        float u=(x-bounds.x)/bounds.w,v=(y-bounds.y)/bounds.h;
        if(life<=0){
            int sx=std::clamp(int(u*20),0,19),sy=std::clamp(int(v*18),0,17);
            if(skullRows[sy][sx]=='1')return sy<6?0xf3e8d0:(sy<11?0xd6cbb8:0xb5a5a6);
            return ((sx+sy)%5==0)?0x65596d:0x3c344c;
        }
        // Tile-sampled flame sprites. Every moving component repeats after 2400ms;
        // avoid the discontinuous phase/flicker of the older 60-second clock.
        constexpr float tau=6.28318530718f;
        u=std::clamp((std::floor(u*24)+.5f)/24,0.f,1.f);
        v=std::clamp((std::floor(v*20)+.5f)/20,0.f,1.f);
        const float phase=tau*float(time%2400)/2400.f;
        float top=intense?.61f:.70f;
        for(int j=0;j<4;++j){
            const float center=(j+.5f)/4+.035f*std::sin(phase*(j%2+1)+j*1.6f);
            float reach=std::max(0.f,1.f-std::abs(u-center)/.20f);
            reach*=reach;
            float rise=(intense?.49f:.29f)*(.83f+.17f*std::sin(phase*(j%2+1)+j*1.7f));
            top=std::min(top,(intense?.63f:.75f)-rise*reach);
        }
        top+=.025f*std::sin(u*26.f-phase*2.f);
        float heat=(v-top)/(.99f-top);
        if(intense&&heat>.70f)return 0xfff1b5;
        if(heat>.85f)return 0xffda72;
        if(heat>.56f)return intense?0xffa337:0xeeae48;
        if(heat>.27f)return intense?0xee611e:0xd86f2b;
        if(heat>-.04f)return intense?0xc52b16:0xa84626;
        return intense?0x702019:0x5c2930;
    }
public:
    NumeralEffect(Renderer& target,Rect bounds,int life,uint64_t time,float step,bool intense):target(target),bounds(bounds),life(life),time(time),step(step),intense(intense){}
    void screen(int)override{}
    void text(const std::string&,float,float,float,Color,Align,float)override{}
    void box(Rect r,Color,float=0)override{
        // Each output rectangle is strictly contained in an original glyph run.
        for(float y=r.y;y<r.y+r.h-.001f;y+=step)for(float x=r.x;x<r.x+r.w-.001f;x+=step){
            float w=std::min(step,r.x+r.w-x),h=std::min(step,r.y+r.h-y);
            target.box({x,y,w,h},sample(x+w/2,y+h/2));
        }
    }
};
}
void drawMarker(Renderer& r,Rect a,bool initiative){
    // 14x10 sprites with dark outlines and a separate highlight ramp.
    const char* crown[]={"00000022000000","02000233200020","23200233200232","23322333322332","23333333333332","02333333333320","02333333333320","02222222222220","02333333333320","00222222222200"};
    const char* chest[]={"00022222222000","00233333333200","02344444443320","23333333333332","22222222222222","23333344333332","23111144111132","23111111111132","23333333333332","02222222222220"};
    const Color gold[]={0,0x755029,0xd6a841,0xffdf78,0xffedb0},brown[]={0,0x60412c,0x9d6946,0xc58c5b,0xe3b181};
    float sx=a.w/14,sy=a.h/10;auto rows=initiative?chest:crown;auto colors=initiative?brown:gold;
    for(int y=0;y<10;++y)for(int x=0;x<14;++x)if(rows[y][x]!='0')r.box({a.x+x*sx,a.y+y*sy,sx,sy},colors[rows[y][x]-'0']);
}

void lifeText(Renderer& r,int value,float x,float y,float size,Color color,Align align,float maxWidth,uint64_t time,int flameThreshold,bool effects,bool poisoned,float displayed,bool decreasing,int startingLife){
    std::string s=std::to_string(value);float base=size/22.0f,width=std::max<size_t>(2,s.size())*13*base;
    float scale=(maxWidth>0&&width>maxWidth)?maxWidth/width:1.0f;width*=scale;
    float start=x+(align==Align::Center?-width/2:(align==Align::Right?-width:0));
    float activeStart=start+(s.size()<2?13*base*scale:0),activeWidth=s.size()*13*base*scale;
    bool dead=value<=0||poisoned;
    if(effects&&dead){
        // Independently rasterized skull-and-crossbones *behind* the grey numeral.
        // Per-row offsets create the waving fabric silhouette, never a full-card overlay.
        float skullW=std::min(activeWidth*1.05f,size*.94f),skullH=size*14/22;
        float dx=activeStart+(activeWidth-skullW)/2,dy=y;
        for(int sy=0;sy<18;++sy){float wave=std::sin(float(time%3200)*6.283185307f/3200.0f+sy*.24f)*skullW*.055f;
            for(int sx=0;sx<20;){if(skullRows[sy][sx]!='1'){++sx;continue;}
                int first=sx;while(sx<20&&skullRows[sy][sx]=='1')++sx;
                r.box({dx+wave+first*skullW/20,dy+sy*skullH/18,(sx-first)*skullW/20,skullH/18},sy<10?0xc9c4b9:0x958f89);
            }
        }
        color=0x777b82;
    }
    auto drawNumber=[&](Renderer& target){
        if(!std::isfinite(displayed)||std::abs(displayed-value)<.001f){pixelText(target,s,activeStart,y,size*scale,color,Align::Left,activeWidth);return;}
        int first=decreasing?static_cast<int>(std::ceil(displayed)):static_cast<int>(std::floor(displayed)),next=first+(decreasing?-1:1);
        float phase=std::abs(displayed-first),unit=size/22*scale,step=15*unit,shift=phase*step*(decreasing?1:-1);
        std::string a=std::to_string(first),b=std::to_string(next);size_t length=std::max<size_t>({2,a.size(),b.size()});a.insert(0,length-a.size(),' ');b.insert(0,length-b.size(),' ');
        struct Clip:Renderer{Renderer& target;float top,bottom;Clip(Renderer&r,float y,float h):target(r),top(y),bottom(y+h){}void screen(int)override{}void text(const std::string&,float,float,float,Color,Align,float)override{}Color textColor(Color c,float x,float y)const override{return target.textColor(c,x,y);}void box(Rect r,Color c,float radius=0)override{float low=std::max(top,r.y),high=std::min(bottom,r.y+r.h);if(high>low)target.box({r.x,low,r.w,high-low},c,radius);}} clip(target,y,14*unit);
        float advance=13*unit,run=length*advance,origin=x+(align==Align::Center?-run/2:align==Align::Right?-run:0);
        for(size_t i=0;i<length;++i){float dx=origin+i*advance;
            if(a[i]==b[i]){if(a[i]!=' ')pixelText(clip,std::string(1,a[i]),dx,y,size*scale,color,Align::Left,advance);}
            else{if(a[i]!=' ')pixelText(clip,std::string(1,a[i]),dx,y+shift,size*scale,color,Align::Left,advance);if(b[i]!=' ')pixelText(clip,std::string(1,b[i]),dx,y+shift+(decreasing?-step:step),size*scale,color,Align::Left,advance);}
        }
    };
    if(effects&&!dead&&value<=flameThreshold){
        NumeralEffect effect(r,{activeStart,y,activeWidth,size*14/22},value,time,std::max(1.25f,size/17),value*4<=(startingLife>0?startingLife:flameThreshold*2));drawNumber(effect);
    }else drawNumber(r);
    if(!effects||(std::isfinite(displayed)&&std::abs(displayed-value)>=.001f))return;
    // A contrasting underline distinguishes 6 from 9 when viewed upside down.
    float unit=size/22*scale;float cursor=activeStart;
    for(char ch:s){int id=std::clamp(int(static_cast<unsigned char>(ch)),32,126)-32;
        float w=medieval::widths[id]*unit;
        if(ch=='6'||ch=='9')r.box({cursor,y+15*unit,w,unit},dead?0xc3c7c9:color);
        cursor+=(medieval::widths[id]+1)*unit;
    }
}
void drawDie(Renderer& r,Rect a,int sides,int result,float progress,Color face,Color ink){
    const bool rolling=progress<1;float cx=a.x+a.w/2,cy=a.y+a.h/2;
    if(sides==2){
        float squash=rolling?std::max(.10f,std::abs(std::cos(progress*22))):1;
        int radius=30;float scale=std::min(a.w,a.h)/68;
        for(int y=-radius;y<=radius;++y){int extent=static_cast<int>(std::sqrt(radius*radius-y*y));r.box({cx-extent*scale*squash,cy+y*scale,2*extent*scale*squash,scale},y<-22?0xffedba:face);}
        if(squash>.65f)pixelText(r,result==1?"H":"T",cx,cy-8,17,ink,Align::Center,30*squash);
        return;
    }
    int vertices=sides==4?3:(sides==6?4:(sides==8?4:(sides==10?4:(sides==12?5:6))));
    float rotation=rolling?std::floor(progress*14)*.48f:0;
    float initial=sides==6?0.785398f:-1.570796f;
    float rad=std::min(a.w,a.h)*.44f;
    std::array<std::array<int,2>,6> points{};
    for(int i=0;i<vertices;++i){float angle=initial+rotation+i*6.2831853f/vertices;points[i]={{static_cast<int>((cx+std::cos(angle)*rad)),static_cast<int>((cy+std::sin(angle)*rad))}};}
    // Pixel silhouette: scan-convert the polygon to solid horizontal runs.
    int ymin=10000,ymax=-10000;for(int i=0;i<vertices;++i){ymin=std::min(ymin,points[i][1]);ymax=std::max(ymax,points[i][1]);}
    for(int y=ymin;y<=ymax;++y){std::vector<int> xs;for(int i=0;i<vertices;++i){auto v=points[i],w=points[(i+1)%vertices];if((v[1]<=y&&w[1]>y)||(w[1]<=y&&v[1]>y))xs.push_back(v[0]+(y-v[1])*(w[0]-v[0])/(w[1]-v[1]));}if(xs.size()>=2){std::sort(xs.begin(),xs.end());r.box({float(xs.front()),float(y),float(xs.back()-xs.front()+1),1},face);}}
    if(sides>=8){for(int i=0;i<vertices;++i)line(r,points[i][0],points[i][1],static_cast<int>(cx),static_cast<int>(cy),1,ink);r.box({cx-15,cy-12,30,24},face);}
    if(sides==6&&!rolling){
        const int pos[][2]={{-9,-9},{9,9},{9,-9},{-9,9},{-9,0},{9,0}};
        if(result%2)r.box({cx-2,cy-2,4,4},ink);
        int pairs=result/2;for(int i=0;i<pairs*2;++i)r.box({cx+pos[i][0]-2,cy+pos[i][1]-2,4,4},ink);
    }else pixelText(r,std::to_string(result),cx,cy-(sides==4?0:8),16,ink,Align::Center,30);
}
}
