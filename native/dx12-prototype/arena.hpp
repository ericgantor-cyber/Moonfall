// Moonfall isolated DirectX 12 ARENA EXPERIMENT, not the existing Z/Nyra game.
// Portable, deterministic game logic without Win32, GPU, SDKs or character art.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace moonfall {
constexpr float Width = 1024.f, Height = 576.f;
constexpr int MaxEnemies = 48, MaxShots = 96;
enum class Phase { Playing, Won, Lost };
struct Input { bool up=false, down=false, left=false, right=false, attack=false, dash=false, restart=false; };
struct Enemy { float x=0, y=0, hp=0; bool active=false; };
struct Shot { float x=0, y=0, vx=0, vy=0, life=0; bool active=false; };

struct Arena {
  float x=Width/2, y=Height/2;
  float health=100, seconds=0, fireCooldown=0, dashCooldown=0, invulnerable=0, spawnCooldown=0.4f;
  int kills=0, wave=1;
  Phase phase=Phase::Playing;
  uint32_t randomState=0xC0FFEEu;
  std::array<Enemy,MaxEnemies> enemies{};
  std::array<Shot,MaxShots> shots{};

  static float clamp(float n,float min,float max){return std::max(min,std::min(n,max));}
  static float distanceSquared(float ax,float ay,float bx,float by){
    const float dx=ax-bx,dy=ay-by;return dx*dx+dy*dy;
  }
  uint32_t nextRandom(){
    randomState ^= randomState<<13;
    randomState ^= randomState>>17;
    randomState ^= randomState<<5;
    return randomState;
  }
  int enemyCount() const {int n=0;for(const auto& e:enemies)if(e.active)++n;return n;}
  int projectileCount() const {int n=0;for(const auto& s:shots)if(s.active)++n;return n;}
  void reset(){*this=Arena{};}
  void spawnEnemy(){
    if(enemyCount()>=32)return;
    for(auto& e:enemies)if(!e.active){
      const unsigned edge=nextRandom()%4;
      const float offset=30.f+float(nextRandom()%960);
      e={edge==0?20.f:edge==1?Width-20.f:clamp(offset,20.f,Width-20.f),
         edge==2?20.f:edge==3?Height-20.f:clamp(offset*0.56f,20.f,Height-20.f),
         wave==1?2.f:wave==2?3.f:4.f,true};
      return;
    }
  }
  void fire(){
    for(auto& shot:shots)if(!shot.active){
      const Enemy* target=nullptr;float closest=1e15f;
      for(const auto& e:enemies)if(e.active){
        const float d=distanceSquared(x,y,e.x,e.y);
        if(d<closest){closest=d;target=&e;}
      }
      float dx=1,dy=0;
      if(target){dx=target->x-x;dy=target->y-y;}
      const float len=std::max(0.0001f,std::sqrt(dx*dx+dy*dy));
      shot={x,y,dx/len*650.f,dy/len*650.f,1.15f,true};
      return;
    }
  }
  void update(float dt,const Input& input){
    if(input.restart){reset();return;}
    if(phase!=Phase::Playing)return;
    dt=clamp(dt,0.f,0.05f);
    seconds+=dt;
    fireCooldown=std::max(0.f,fireCooldown-dt);
    dashCooldown=std::max(0.f,dashCooldown-dt);
    invulnerable=std::max(0.f,invulnerable-dt);
    float dx=float(input.right)-float(input.left),dy=float(input.down)-float(input.up);
    const float len=std::sqrt(dx*dx+dy*dy);
    if(len>0){dx/=len;dy/=len;}
    if(input.dash&&dashCooldown<=0&&len>0){
      x=clamp(x+dx*116.f,26.f,Width-26.f);
      y=clamp(y+dy*116.f,26.f,Height-26.f);
      dashCooldown=0.9f;invulnerable=std::max(invulnerable,0.16f);
    }
    x=clamp(x+dx*270.f*dt,26.f,Width-26.f);
    y=clamp(y+dy*270.f*dt,26.f,Height-26.f);
    spawnCooldown-=dt;
    if(spawnCooldown<=0){
      spawnCooldown+=std::max(0.30f,0.90f-float(wave-1)*0.20f);
      spawnEnemy();
    }
    if(input.attack&&fireCooldown<=0){fireCooldown=0.19f;fire();}
    for(auto& e:enemies)if(e.active){
      float vx=x-e.x,vy=y-e.y;
      const float dist=std::max(0.0001f,std::sqrt(vx*vx+vy*vy));
      const float speed=58.f+float(wave-1)*18.f;
      e.x+=vx/dist*speed*dt;e.y+=vy/dist*speed*dt;
      if(dist<31.f&&invulnerable<=0){
        health=std::max(0.f,health-12.f);
        invulnerable=0.58f;
        if(health<=0)phase=Phase::Lost;
      }
    }
    for(auto& s:shots)if(s.active){
      s.x+=s.vx*dt;s.y+=s.vy*dt;s.life-=dt;
      if(s.life<=0||s.x<0||s.x>Width||s.y<0||s.y>Height){s.active=false;continue;}
      for(auto& e:enemies)if(e.active&&distanceSquared(s.x,s.y,e.x,e.y)<24.f*24.f){
        s.active=false;
        e.hp-=1.f;
        if(e.hp<=0){e.active=false;++kills;}
        break;
      }
    }
    wave=kills<8?1:kills<20?2:3;
    if(kills>=36)phase=Phase::Won;
  }
};
} // namespace moonfall
