#pragma once
#include "ui.hpp"
#include <limits>
namespace deck {
bool pixelCompatible(const std::string& text);
float pixelAdvance(const std::string& text);
void pixelText(Renderer& r,const std::string& text,float x,float y,float size,Color color,Align align,float maxWidth);
void lifeText(Renderer& r,int value,float x,float y,float size,Color color,Align align,float maxWidth,uint64_t time=0,int flameThreshold=10,bool effects=true,bool poisoned=false,float displayed=std::numeric_limits<float>::quiet_NaN(),bool decreasing=false,int startingLife=0);
void drawMarker(Renderer& r,Rect area,bool initiative);
void drawDie(Renderer& r,Rect area,int sides,int result,float progress,Color face,Color ink);
}
