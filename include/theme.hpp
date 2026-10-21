#pragma once
#include "ui.hpp"
namespace deck {
Color magicColor(int index);
struct ThemePoint {float x,y;int color;};
std::vector<ThemePoint> themePoints(int mask,float aspect=400.0f/240);
Color themeSample(int mask,float x,float y,float aspect=400.0f/240,uint64_t time=0,float speed=1.f);
// Theme at physical screen coordinates, after a player panel's rotation.
class ThemeRenderer final:public Renderer {
    Renderer& target;int mask;float speed;uint64_t now=0;float width=400;bool textMode=false;
    Color contrast(Color c,float x,float y)const;
public:
    ThemeRenderer(Renderer& target,int mask,uint64_t time=0,float speed=1.f):target(target),mask(mask),speed(speed),now(time){}
    void screen(int n)override{width=n?320:400;target.screen(n);}
    void box(Rect r,Color c,float radius=0)override;
    void homeArtwork(Rect area)override{target.homeArtwork(area);}
    Color textColor(Color c,float x,float y)const override;
    void text(const std::string& s,float x,float y,float size,Color c,Align a,float w)override;
};
}
