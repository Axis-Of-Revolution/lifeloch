#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <string>
namespace deck {
class Renderer;struct Rect;
constexpr float RollSimulationSpeed=1.7f;
constexpr uint64_t RollDurationMs=3000; // Tuning target, not a forced outcome deadline.
struct Vec3{float x=0,y=0,z=0;};
struct Face{std::vector<int> ids;int label=0;};
struct Mesh{std::vector<Vec3> vertices;std::vector<Face> faces;};
struct RollPose{Vec3 position;std::array<float,4> rotation{{1,0,0,0}};int hits=0;bool settled=false;int result=0;};
const Mesh& diceMesh(int sides);
class RollSimulation {
public:
    void start(int sides,uint32_t seed);
    void advance(float seconds,float tiltX=0,float tiltY=0,float yawRate=0);
    RollPose pose()const{return {position,rotation,hits,settled,result};}
    float seconds()const{return elapsed/RollSimulationSpeed;}
    Vec3 linearVelocity()const{return velocity;}
    Vec3 angularVelocity()const{return spin;}
    int faceUp()const;
private:
    int sides=20,hits=0,result=0;
    Vec3 position{},velocity{},spin{},inertia{.27f,.27f,.27f};
    std::array<float,4> rotation{{1,0,0,0}};
    float elapsed=0,quiet=0,accumulator=0,tiltX=0,tiltY=0,lastHit=-1;
    bool settled=false,interactive=false;
    float trayYaw=0;
    float motionAge=0,sleepTiltX=0,sleepTiltY=0;
    void step(float tx,float ty,float yawRate);
    Vec3 inverseInertia(Vec3 value)const;
    Vec3 pointVelocity(Vec3 point)const;
    float effectiveMass(Vec3 point,Vec3 normal)const;
    void impulse(Vec3 force,Vec3 point);
};
float diceLabelHeight(const Mesh& mesh,const Face& face,const std::string& text,float maximum);
void drawDiceTray(Renderer& renderer,Rect area,int sides,const RollPose* pose);
}
