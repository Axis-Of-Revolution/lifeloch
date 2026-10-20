#include "dice.hpp"
#include "ui.hpp"
#include "medieval_atlas.hpp"
#include "pixel.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include "dice_meshes.hpp"
namespace deck {
namespace {
constexpr float Pi=3.14159265359f;
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 mul(Vec3 a,float f){return {a.x*f,a.y*f,a.z*f};}
Vec3 sub(Vec3 a,Vec3 b){return add(a,mul(b,-1));}
float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3 unit(Vec3 v){return mul(v,1/std::max(.00001f,std::sqrt(dot(v,v))));}
using Q=std::array<float,4>;
Q normalized(Q q){float s=0;for(float x:q)s+=x*x;for(float&x:q)x/=std::sqrt(std::max(s,.000001f));return q;}
Q product(Q a,Q b){return {a[0]*b[0]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3],a[0]*b[1]+a[1]*b[0]+a[2]*b[3]-a[3]*b[2],a[0]*b[2]-a[1]*b[3]+a[2]*b[0]+a[3]*b[1],a[0]*b[3]+a[1]*b[2]-a[2]*b[1]+a[3]*b[0]};}
Vec3 rotate(Q q,Vec3 v){Vec3 t=mul(cross({q[1],q[2],q[3]},v),2);return add(v,add(mul(t,q[0]),cross({q[1],q[2],q[3]},t)));}
Vec3 center(const Mesh&m,const Face&f){Vec3 out;for(int i:f.ids)out=add(out,m.vertices[i]);return mul(out,1.f/f.ids.size());}
Vec3 normal(const Mesh&m,const Face&f){return unit(cross(sub(m.vertices[f.ids[1]],m.vertices[f.ids[0]]),sub(m.vertices[f.ids[2]],m.vertices[f.ids[0]])));}
struct Point{float x,y;};
Point project(Vec3 p,Rect area){float scale=std::min(area.w/16.6f,area.h/10.4f),k=1+p.z*.055f;return {area.x+area.w/2+p.x*scale*k,area.y+area.h/2-p.y*scale*k};}
void polygonPoints(Renderer&r,const Point*p,size_t count,Color color){
 float lo=10000,hi=-10000;for(size_t i=0;i<count;i++){lo=std::min(lo,p[i].y);hi=std::max(hi,p[i].y);}
 for(float y=std::floor(lo);y<=std::ceil(hi);y+=1){std::array<float,32> xs{};int size=0;for(size_t i=0;i<count;i++){auto a=p[i],b=p[(i+1)%count];if((a.y<=y+.5f&&b.y>y+.5f)||(b.y<=y+.5f&&a.y>y+.5f))xs[size++]=a.x+(y+.5f-a.y)*(b.x-a.x)/(b.y-a.y);}if(size>=2){auto range=std::minmax_element(xs.begin(),xs.begin()+size);r.box({*range.first,y,std::max(0.f,*range.second-*range.first),1},color);}}
}
void polygon(Renderer&r,const std::vector<Point>&p,Color color){polygonPoints(r,p.data(),p.size(),color);}
void polygon(Renderer&r,std::initializer_list<Point>p,Color color){polygonPoints(r,p.begin(),p.size(),color);}
void edge(Renderer&r,Point a,Point b,Color c){float steps=std::max(std::abs(a.x-b.x),std::abs(a.y-b.y));for(float t=0;t<=steps;t+=1){float u=steps?t/steps:0;r.box({a.x+(b.x-a.x)*u,a.y+(b.y-a.y)*u,1.0f,1.0f},c);}}
Color shade(Color c,float f){Color out=0;for(int s:{16,8,0})out|=static_cast<Color>(std::clamp(((c>>s)&255)*f,0.f,255.f))<<s;return out;}
void surfaceText(Renderer&r,const std::string&s,Vec3 c,Vec3 u,Vec3 v,float height,Rect area){float scale=height/14,total=pixelAdvance(s)*scale,cursor=-total/2;
 for(char ch:s){int id=std::clamp(int(ch),32,126)-32,w=medieval::widths[id];for(int y=0;y<22;y++)for(int x=0;x<w;){if(!(medieval::rows[id][y]>>x&1)){++x;continue;}int first=x;while(x<w&&(medieval::rows[id][y]>>x&1))++x;
 float x0=cursor+first*scale,x1=cursor+x*scale,y0=(y-7)*scale,y1=(y-6)*scale;
 auto p=[&](float a,float b){return project(add(c,add(mul(u,a),mul(v,b))),area);};polygon(r,{p(x0,y0),p(x1,y0),p(x1,y1),p(x0,y1)},0x201e25);}
 cursor+=(w+1)*scale;}
 if(s.find_first_of("69")!=std::string::npos){auto point=[&](float x,float y){return project(add(c,add(mul(u,x),mul(v,y))),area);};polygon(r,{point(-total/2,8*scale),point(total/2,8*scale),point(total/2,9*scale),point(-total/2,9*scale)},0x201e25);}
}
}
const Mesh& diceMesh(int sides){static const Mesh meshes[]={dice_data::mesh2(),dice_data::mesh4(),dice_data::mesh6(),dice_data::mesh8(),dice_data::mesh10(),dice_data::mesh12(),dice_data::mesh20()};int id=sides==2?0:sides==4?1:sides==6?2:sides==8?3:sides==10?4:sides==12?5:6;return meshes[id];}
void RollSimulation::start(int count,uint32_t seed){
 sides=count;uint32_t rng=seed?seed:1;auto random=[&](){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng/4294967296.0f;};
 position={-2.8f+random()*1.8f,(random()-.5f)*2.4f,2.1f+random()*.5f};velocity={3+random()*3,(random()-.5f)*3,3+random()*2};
 float u=random(),v=random()*2*Pi,w=random()*2*Pi;rotation={std::sqrt(u)*std::cos(w),std::sqrt(1-u)*std::sin(v),std::sqrt(1-u)*std::cos(v),std::sqrt(u)*std::sin(w)};
 spin={(random()-.5f)*28,(random()-.5f)*28,(random()-.5f)*20};if(sides==2)spin={15+random()*12,(random()-.5f)*8,(random()-.5f)*7};
 trayYaw=elapsed=motionAge=quiet=accumulator=tiltX=tiltY=sleepTiltX=sleepTiltY=0;interactive=false;hits=result=0;lastHit=-1;settled=false;inertia=sides==2?Vec3{.25f,.25f,.5f}:Vec3{.27f,.27f,.27f};
}
Vec3 RollSimulation::inverseInertia(Vec3 v)const{auto local=rotate({rotation[0],-rotation[1],-rotation[2],-rotation[3]},v);return rotate(rotation,{local.x/inertia.x,local.y/inertia.y,local.z/inertia.z});}
void RollSimulation::impulse(Vec3 j,Vec3 r){velocity=add(velocity,j);spin=add(spin,inverseInertia(cross(r,j)));}
Vec3 RollSimulation::pointVelocity(Vec3 r)const{return add(velocity,cross(spin,r));}
float RollSimulation::effectiveMass(Vec3 r,Vec3 n)const{return 1+dot(n,cross(inverseInertia(cross(r,n)),r));}
int RollSimulation::faceUp()const{
 const auto& m=diceMesh(sides);float best=-100;int value=0;
 if(sides==4){for(size_t i=0;i<m.vertices.size();++i){float z=rotate(rotation,m.vertices[i]).z;if(z>best){best=z;value=i+1;}}}
 else for(const auto& f:m.faces)if(f.label){float z=rotate(rotation,normal(m,f)).z;if(z>best){best=z;value=f.label;}}
 return value;
}
void RollSimulation::step(float tx,float ty,float yawRate){
 if(settled)return;
 constexpr float dt=1.f/240;elapsed+=dt;motionAge+=dt;float limit=interactive?.85f:.16f;tiltX+=(std::clamp(tx,-limit,limit)-tiltX)*.08f;tiltY+=(std::clamp(ty,-limit,limit)-tiltY)*.08f;float previousYaw=trayYaw;trayYaw+=(yawRate/RollSimulationSpeed-trayYaw)*.08f;float alpha=(trayYaw-previousYaw)/dt;
 bool tilted=interactive&&(tiltX*tiltX+tiltY*tiltY>.0064f||std::abs(trayYaw)>.035f);
 Vec3 inertiaForce{2*trayYaw*velocity.y+alpha*position.y+trayYaw*trayYaw*position.x,-2*trayYaw*velocity.x-alpha*position.x+trayYaw*trayYaw*position.y,0};
 velocity=add(velocity,mul(inertiaForce,dt));spin.z-=alpha*dt;
 velocity=mul(add(velocity,mul({tiltX*28,tiltY*28,-28},dt)),std::exp(-.18f*dt));spin=mul(spin,std::exp(-.25f*dt));position=add(position,mul(velocity,dt));
 auto dq=product({0,spin.x,spin.y,spin.z},rotation);for(int i=0;i<4;++i)rotation[i]+=dq[i]*dt*.5f;rotation=normalized(rotation);
 struct Plane{Vec3 n;float d;};const Plane planes[]={{{0,0,1},0},{{.70710678f,0,.70710678f},4.94974747f},{{-.70710678f,0,.70710678f},4.94974747f},{{0,.70710678f,.70710678f},2.82842712f},{{0,-.70710678f,.70710678f},2.82842712f}};
 struct Contact{Vec3 r,n;float target,mu,j=0;Vec3 tangent{};};std::vector<Contact> contacts;contacts.reserve(40);bool floorContact=false;const auto& mesh=diceMesh(sides);
 for(auto vertex:mesh.vertices){auto r=rotate(rotation,vertex),world=add(position,r);for(auto plane:planes){float distance=dot(world,plane.n)+plane.d;if(distance>.007f)continue;float vn=dot(pointVelocity(r),plane.n);
   if(vn<-1.4f&&elapsed-lastHit>.045f){++hits;lastHit=elapsed;}
   float restitution=motionAge<1.3f?.32f:.08f,target=(vn<-1?-vn*restitution:0)+std::min(5.f,std::max(0.f,-distance-.002f)*.17f/dt);
   contacts.push_back({r,plane.n,target,plane.n.z==1?(tilted?.24f:motionAge>1.2f?.65f:.30f):.01f});if(plane.n.z==1)floorContact=true;
 }}
 for(int iteration=0;iteration<12;++iteration)for(auto& c:contacts){float vn=dot(pointVelocity(c.r),c.n),j=std::max(0.f,c.j+(c.target-vn)/effectiveMass(c.r,c.n)),dj=j-c.j;c.j=j;impulse(mul(c.n,dj),c.r);
   auto v=pointVelocity(c.r),tangent=sub(v,mul(c.n,dot(v,c.n)));float speed=std::sqrt(dot(tangent,tangent));if(speed<.00001f)continue;auto direction=mul(tangent,1/speed),delta=mul(direction,-speed/effectiveMass(c.r,direction)),candidate=add(c.tangent,delta);float maximum=c.j*c.mu,length=std::sqrt(dot(candidate,candidate));if(length>maximum)candidate=mul(candidate,maximum/length);impulse(sub(candidate,c.tangent),c.r);c.tangent=candidate;
 }
 if(floorContact){float late=tilted?0.f:std::clamp((motionAge-1.8f)/.8f,0.f,1.f),damping=tilted?1.2f:(motionAge>1.35f?2.6f:.75f)+late*9;spin=mul(spin,std::exp(-damping*dt));velocity=mul(velocity,std::exp(-((tilted?.8f:2.f)+late*36)*dt));}
 bool flat=false;for(const auto& f:mesh.faces)if((sides!=2||f.label)&&rotate(rotation,normal(mesh,f)).z<-.9985f){flat=true;break;}
 if(floorContact&&dot(velocity,velocity)<.04f&&dot(spin,spin)<.035f)quiet+=dt;else quiet=0;
 if(quiet>.10f&&motionAge>.65f&&std::abs(trayYaw)<.035f){settled=true;sleepTiltX=tx;sleepTiltY=ty;result=flat?faceUp():0;velocity=spin={};}
}
void RollSimulation::advance(float seconds,float tx,float ty,float yawRate){
 yawRate=std::isfinite(yawRate)?std::clamp(yawRate,-4.f,4.f):0.f;if(std::abs(yawRate)<.04f)yawRate=0;
 if(std::hypot(tx,ty)>.20f||std::abs(yawRate)>.08f)interactive=true;
 if(settled){if(std::hypot(tx-sleepTiltX,ty-sleepTiltY)<.045f&&std::abs(yawRate)<.08f)return;settled=false;interactive=true;motionAge=quiet=accumulator=0;result=0;}
 accumulator+=std::clamp(seconds,0.f,5.f)*RollSimulationSpeed;constexpr float dt=1.f/240;int steps=std::min(2040,static_cast<int>((accumulator+1e-7f)/dt));
 for(int i=0;i<steps&&!settled;++i)step(tx,ty,yawRate);
 accumulator-=steps*dt;
}
float diceLabelHeight(const Mesh& mesh,const Face& f,const std::string& text,float maximum){
    auto c=center(mesh,f),n=normal(mesh,f),u=unit(sub(mesh.vertices[f.ids[1]],mesh.vertices[f.ids[0]])),v=mul(cross(n,u),-1);
    float scale=maximum/14,width=pixelAdvance(text),bottom=text.find_first_of("HT")!=std::string::npos?10.f:text.find_first_of("69")!=std::string::npos?9.f:7.f;
    for(size_t i=0;i<f.ids.size();++i){auto a=mesh.vertices[f.ids[i]],b=mesh.vertices[f.ids[(i+1)%f.ids.size()]],inward=unit(cross(n,sub(b,a)));float room=dot(inward,sub(c,a))*.86f;
        for(float x:{-width/2,width/2})for(float y:{-7.f,bottom}){float direction=dot(inward,add(mul(u,x),mul(v,y)));if(direction<0)scale=std::min(scale,room/-direction);}
    }return std::max(0.f,scale*14);
}
void drawDiceTray(Renderer&target,Rect area,int sides,const RollPose* body){
 struct Clip:Renderer{Renderer&t;Rect area;Clip(Renderer&t,Rect a):t(t),area(a){}void screen(int)override{}void text(const std::string&,float,float,float,Color,Align,float)override{}void box(Rect r,Color c,float radius=0)override{float x=std::max(r.x,area.x),y=std::max(r.y,area.y),right=std::min(r.x+r.w,area.x+area.w),bottom=std::min(r.y+r.h,area.y+area.h);if(right>x&&bottom>y)t.box({x,y,right-x,bottom-y},c,radius);}} r(target,area);
 const auto&m=diceMesh(sides);
 auto projected=[&](Vec3 v){return project(v,area);};
 std::vector<Point> rim={projected({-8,-5,0}),projected({8,-5,0}),projected({8,5,0}),projected({-8,5,0})};
 polygon(r,rim,0x705839);
 std::vector<Point> floor={projected({-7,-4,0}),projected({7,-4,0}),projected({7,4,0}),projected({-7,4,0})};
 polygon(r,floor,0x183b34);for(size_t i=0;i<4;i++)edge(r,floor[i],floor[(i+1)%4],0x9d835c);
 if(!body)return;
 auto pose=*body;
 // A three-layer convex silhouette shadow: no blur shader or extra physics body.
 std::vector<Point> cast;for(auto vertex:m.vertices){auto world=add(rotate(pose.rotation,vertex),pose.position);float factor=12/std::max(3.f,12-world.z);cast.push_back(projected({-4+(world.x+4)*factor,-6+(world.y+6)*factor,0}));}
 std::sort(cast.begin(),cast.end(),[](Point a,Point b){return a.x<b.x||(a.x==b.x&&a.y<b.y);});
 auto turn=[](Point a,Point b,Point c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);};std::vector<Point> hull;
 for(auto p:cast){while(hull.size()>1&&turn(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}size_t lower=hull.size();
 for(int i=int(cast.size())-2;i>=0;--i){auto p=cast[i];while(hull.size()>lower&&turn(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}if(hull.size()>1)hull.pop_back();
 Point shadowCenter{};for(auto p:hull){shadowCenter.x+=p.x/hull.size();shadowCenter.y+=p.y/hull.size();}float height=std::max(0.f,pose.position.z);
 Point tl=projected({-7,4,0}),br=projected({7,-4,0});Clip shadowRenderer(r,{tl.x,tl.y,br.x-tl.x,br.y-tl.y});
 for(int layer=2;layer>=0;--layer){float spread=1+layer*(.035f+height*.035f),opacity=(layer==2?.16f:layer==1?.33f:.72f)/(1+height*.20f);Color color=0;for(int shift:{16,8,0})color|=static_cast<Color>(((0x183b34>>shift)&255)*(1-opacity)+((0x081416>>shift)&255)*opacity)<<shift;std::vector<Point> shape;for(auto p:hull)shape.push_back({shadowCenter.x+(p.x-shadowCenter.x)*spread,shadowCenter.y+(p.y-shadowCenter.y)*spread});polygon(shadowRenderer,shape,color);}
 struct Visible{const Face*f;float depth;};std::vector<Visible> faces;
 for(const auto&f:m.faces){auto n=rotate(pose.rotation,normal(m,f));auto c=rotate(pose.rotation,center(m,f));if(n.z>.015f){faces.push_back({&f,-c.z});}}
 std::sort(faces.begin(),faces.end(),[](Visible a,Visible b){return a.depth>b.depth;});
 for(auto item:faces){const auto&f=*item.f;auto n=rotate(pose.rotation,normal(m,f));std::vector<Point> points;for(int i:f.ids)points.push_back(projected(add(rotate(pose.rotation,m.vertices[i]),pose.position)));
 Color base=sides==2?0xd8ad60:0xd8c7a3;polygon(r,points,shade(base,.40f+.6f*std::max(0.f,dot(n,unit({-.4f,-.5f,1})))));for(size_t i=0;i<points.size();i++)edge(r,points[i],points[(i+1)%points.size()],0x79664b);
 if(f.label){auto c=add(rotate(pose.rotation,center(m,f)),pose.position),u=rotate(pose.rotation,unit(sub(m.vertices[f.ids[1]],m.vertices[f.ids[0]]))),v=mul(cross(n,u),-1);
 if(sides==4){for(int vertex:f.ids){Vec3 near=add(mul(m.vertices[vertex],.57f),mul(center(m,f),.43f));surfaceText(r,std::to_string(vertex+1),add(rotate(pose.rotation,near),pose.position),u,v,.31f,area);}}
 else {auto text=sides==2?(f.label==1?"H":"T"):std::to_string(f.label);surfaceText(r,text,c,u,v,diceLabelHeight(m,f,text,sides==2?.95f:sides>=12?.48f:.65f),area);}}
 }
}
}
