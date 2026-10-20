#include "layout.hpp"
#include "pixel.hpp"
#include <algorithm>
#include <cmath>
namespace deck {
int layoutCount(int players){return players==3?6:players==5?4:2;}
std::string layoutName(int players,int layout){
    if(players==2)return layout==0?"Upright":"Face sides";
    if(players==5){const char* names[]={"Top pair","Bottom pair","Side left","Side right"};return names[std::clamp(layout,0,3)];}
    if(players==3){const char* names[]={"Top left","Top right","Bottom left","Bottom right","Side left","Side right"};return names[std::clamp(layout,0,5)];}
    return layout==0?(players==6?"Six seats":"Quadrants"):"Side sections";
}
static std::array<Rect,MaxPlayers> rawPanels(int n,int layout){
    std::array<Rect,MaxPlayers> out{};
    if(n==2){out[0]={4,20,193,204};out[1]={203,20,193,204};return out;}
    if(n==5&&layout<2){
        int k=0;for(int row=0;row<2;++row){int count=(row==layout)?2:3;float w=(392.0f-(count-1)*6)/count;
            for(int col=0;col<count;++col)out[k++]={4+col*(w+6),20.0f+row*105,w,99};}return out;
    }
    if(n==5)layout+=2; // Side variants retain their established geometry.
    const Rect q[]={{4,20,193,99},{203,20,193,99},{4,125,193,99},{203,125,193,99}};
    if((n==3||n==5)&&layout<4){
        int last=n-1,k=0,opposite=layout^3;out[last]=q[layout];
        for(int j=0;j<4;++j){if(j==layout)continue;if(n==3){if(j!=opposite)out[k++]=q[j];}
            else if(j==opposite){out[k++]={q[j].x,q[j].y,93.5f,99};out[k++]={q[j].x+99.5f,q[j].y,93.5f,99};}
            else out[k++]=q[j];}
        return out;
    }
    if((n==3||n==5)&&layout>=4){
        bool left=layout==4;out[n-1]={left?4.0f:304.0f,20,92,204};float x=left?102:4;
        if(n==3){out[0]={x,20,294,99};out[1]={x,125,294,99};}
        else for(int i=0;i<4;++i)out[i]={x+(i%2)*150,20.0f+(i/2)*105,144,99};
        return out;
    }
    if(layout==1){
        out[n-2]={4,20,92,204};out[n-1]={304,20,92,204};
        if(n==4){out[0]={102,20,196,99};out[1]={102,125,196,99};}
        else for(int i=0;i<4;++i)out[i]={102.0f+(i%2)*101,20.0f+(i/2)*105,95,99};
        return out;
    }
    int cols=n==6?3:2;float w=(392.0f-(cols-1)*6)/cols,h=99;
    for(int i=0;i<n;++i)out[i]={4+(i%cols)*(w+6),20.0f+(i/cols)*(h+6),w,h};
    return out;
}
std::array<Rect,MaxPlayers> playerPanels(int n,int layout){
    auto out=rawPanels(n,layout);if(n==2)return out;
    auto angle=[](Rect r){return std::fmod(3.141592654f-.001f-std::atan2(r.y+r.h/2-122,r.x+r.w/2-200)+6.283185307f,6.283185307f);};
    std::stable_sort(out.begin(),out.begin()+n,[&](Rect a,Rect b){return angle(a)<angle(b);});
    return out;
}
std::vector<Rect> teamDividers(int count,int layout,int heads){
    std::vector<Rect> lines;if(heads<=1)return lines;auto panels=playerPanels(count,layout);
    // Adjacent card edges are screen-space geometry, independent of their text rotation.
    for(int i=0;i<count;++i)for(int j=i+1;j<count;++j){if(i/heads==j/heads)continue;auto a=panels[i],b=panels[j];
        for(int axis=0;axis<2;++axis){float a0=axis?a.y:a.x,a1=a0+(axis?a.h:a.w),b0=axis?b.y:b.x,b1=b0+(axis?b.h:b.w);
            float gap=b0>=a1?b0-a1:a0>=b1?a0-b1:999,mid=b0>=a1?(a1+b0)/2:(b1+a0)/2;
            float low=std::max(axis?a.x:a.y,axis?b.x:b.y),high=std::min(axis?a.x+a.w:a.y+a.h,axis?b.x+b.w:b.y+b.h);
            if(gap<=8&&high>low){low=std::max(axis?4.f:20.f,low-gap/2-1);high=std::min(axis?396.f:224.f,high+gap/2+1);lines.push_back(axis?Rect{low,mid-1,high-low,2}:Rect{mid-1,low,2,high-low});}
        }
    }
    // Join collinear pieces so a team border is one continuous line through the gutters.
    for(size_t i=0;i<lines.size();++i)for(size_t j=i+1;j<lines.size();){auto&a=lines[i];auto b=lines[j];bool vertical=a.w==2&&b.w==2&&std::abs(a.x-b.x)<.01f&&std::max(a.y,b.y)<=std::min(a.y+a.h,b.y+b.h)+.01f;bool horizontal=a.h==2&&b.h==2&&std::abs(a.y-b.y)<.01f&&std::max(a.x,b.x)<=std::min(a.x+a.w,b.x+b.w)+.01f;
        if(vertical){float end=std::max(a.y+a.h,b.y+b.h);a.y=std::min(a.y,b.y);a.h=end-a.y;lines.erase(lines.begin()+j);}else if(horizontal){float end=std::max(a.x+a.w,b.x+b.w);a.x=std::min(a.x,b.x);a.w=end-a.x;lines.erase(lines.begin()+j);}else ++j;
    }return lines;
}
void chooseLayout(Config& c,int layout){
    c.layout=std::clamp(layout,0,layoutCount(c.players)-1);c.rotations.fill(0);
    auto panels=playerPanels(c.players,c.layout);
    for(int i=0;i<c.players;++i){auto r=panels[i];
        if(c.players==2)c.rotations[i]=c.layout==1?(i==0?1:3):0;
        else if(r.h>r.w*1.5f)c.rotations[i]=r.x<200?1:3;
        else c.rotations[i]=r.y+r.h/2<122?2:0;
    }
}
void PanelRenderer::box(Rect r,Color c,float radius){
    Rect mapped{};switch(angle){
    case 1:mapped={area.x+area.w-r.y-r.h,area.y+r.x,r.h,r.w};break;
    case 2:mapped={area.x+area.w-r.x-r.w,area.y+area.h-r.y-r.h,r.w,r.h};break;
    case 3:mapped={area.x+r.y,area.y+area.h-r.x-r.w,r.h,r.w};break;
    default:mapped={area.x+r.x,area.y+r.y,r.w,r.h};break;
    }target.box(mapped,c,radius);
}
Color PanelRenderer::textColor(Color c,float x,float y)const{
    switch(angle){
    case 1:return target.textColor(c,area.x+area.w-y,area.y+x);
    case 2:return target.textColor(c,area.x+area.w-x,area.y+area.h-y);
    case 3:return target.textColor(c,area.x+y,area.y+area.h-x);
    default:return target.textColor(c,area.x+x,area.y+y);
    }
}
void PanelRenderer::text(const std::string& s,float x,float y,float size,Color c,Align a,float width){
    if(angle==0)target.text(s,area.x+x,area.y+y,size,c,a,width);
    else pixelText(*this,s,x,y,size,textColor(c,x+(a==Align::Left?width*.25f:(a==Align::Right?-width*.25f:0)),y+size/2),a,width);
}
}
