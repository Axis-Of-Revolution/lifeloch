#pragma once
#include "ui.hpp"
namespace deck {
std::array<Rect,MaxPlayers> playerPanels(int count,int layout);
std::vector<Rect> teamDividers(int count,int layout,int heads);
int layoutCount(int players);
std::string layoutName(int players,int layout);
void chooseLayout(Config& config,int layout);
class PanelRenderer final:public Renderer {
    Renderer& target;Rect area;int angle;
public:
    PanelRenderer(Renderer& target,Rect area,int angle):target(target),area(area),angle(angle){}
    float width()const{return angle%2?area.h:area.w;}
    float height()const{return angle%2?area.w:area.h;}
    void screen(int)override{}
    void box(Rect r,Color c,float radius=0)override;
    Color textColor(Color c,float x,float y)const override;
    void text(const std::string& s,float x,float y,float size,Color c,Align a,float width)override;
};
}
