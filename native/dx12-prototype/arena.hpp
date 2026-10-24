// Moonfall isolated DirectX 12 ARENA EXPERIMENT, not the main Z/Nyra game.
// Deterministic fixed-capacity game logic: no per-frame heap allocations.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace moonfall {
constexpr float Width=1024.f, Height=576.f;
constexpr int MaxEnemies=48, MaxShots=96, MaxImpacts=32;
enum class Phase { Playing, Won, Lost };
enum class Hero { Z, Nyra };
enum class Weapon { Bolt, Nova };
enum class EnemyRole { Chaser, Runner, Brute, Hydra };
struct Input {
  bool up=false,down=false,left=false,right=false,attack=false,dash=false;
  bool restart=false,switchHero=false,special=false,switchWeapon=false;
};
struct Enemy {
  float x=0,y=0,hp=0; bool active=false;
  EnemyRole role=EnemyRole::Chaser;
  float maxHp=0,flash=0;
};
struct Shot {
  float x=0,y=0,vx=0,vy=0,life=0; bool active=false; float damage=1.f;
};
struct Impact { float x=0,y=0,radius=0,life=0; bool active=false; };

struct Arena {
  float x=Width/2,y=Height/2;
  float health=100,seconds=0,fireCooldown=0,dashCooldown=0,invulnerable=0,spawnCooldown=.4f;
  float heroSkillCooldown=0,dragonPower=0,nyraShield=0;
  float hydraTell=0,hydraAttackCooldown=2.4f,hydraMinionCooldown=1.5f;
  float novaPulse=0;
  Hero hero=Hero::Z;
  Weapon weapon=Weapon::Bolt;
  int kills=0,wave=1;
  Phase phase=Phase::Playing;
  bool bossSummoned=false,bossDefeated=false;
  std::uint32_t randomState=0xC0FFEEu;
  std::array<Enemy,MaxEnemies> enemies{};
  std::array<Shot,MaxShots> shots{};
  std::array<Impact,MaxImpacts> impacts{};

  static float clamp(float n,float lo,float hi){return std::max(lo,std::min(n,hi));}
  static float distanceSquared(float ax,float ay,float bx,float by){
    const float dx=ax-bx,dy=ay-by;return dx*dx+dy*dy;
  }
  std::uint32_t nextRandom(){
    randomState ^= randomState<<13;
    randomState ^= randomState>>17;
    randomState ^= randomState<<5;
    return randomState;
  }
  int enemyCount() const {int n=0;for(const auto& e:enemies)if(e.active)++n;return n;}
  int projectileCount() const {int n=0;for(const auto& s:shots)if(s.active)++n;return n;}
  const Enemy* hydra() const {
    for(const auto& e:enemies)if(e.active&&e.role==EnemyRole::Hydra)return &e;
    return nullptr;
  }
  bool hydraEnraged() const {
    const auto* h=hydra();return h&&h->hp<=h->maxHp*.5f;
  }
  void reset(){const Hero h=hero;const Weapon w=weapon;*this=Arena{};hero=h;weapon=w;}
  void impact(float ix,float iy,float radius,float life=.25f){
    for(auto& e:impacts)if(!e.active){e={ix,iy,radius,life,true};return;}
  }
  void spawnEnemy(bool summonedMinion=false){
    if(enemyCount()>=32||(!summonedMinion&&bossSummoned))return;
    for(auto& e:enemies)if(!e.active){
      const unsigned edge=nextRandom()%4;
      const float offset=30.f+float(nextRandom()%960);
      const unsigned roll=nextRandom()%5;
      EnemyRole role=EnemyRole::Chaser;
      if(wave>=2&&roll==0)role=EnemyRole::Brute;
      else if(roll<=1)role=EnemyRole::Runner;
      const float hp=role==EnemyRole::Brute?8.f:role==EnemyRole::Runner?2.f:
                     wave==1?2.f:wave==2?3.f:4.f;
      e={edge==0?20.f:edge==1?Width-20.f:clamp(offset,20.f,Width-20.f),
         edge==2?20.f:edge==3?Height-20.f:clamp(offset*.56f,20.f,Height-20.f),
         hp,true,role,hp,0};
      return;
    }
  }
  void spawnHydra(){
    if(bossSummoned)return;
    for(auto& e:enemies)if(!e.active){
      const float hp=32.f;
      e={Width/2,72.f,hp,true,EnemyRole::Hydra,hp,0};
      bossSummoned=true;
      hydraTell=0;
      hydraAttackCooldown=2.4f;
      hydraMinionCooldown=1.5f;
      impact(e.x,e.y,80.f,.45f);
      return;
    }
  }
  void damagePlayer(float damage){
    if(phase!=Phase::Playing||invulnerable>0||nyraShield>0)return;
    health=std::max(0.f,health-damage);
    invulnerable=.58f;
    impact(x,y,26.f,.30f);
    if(health<=0)phase=Phase::Lost;
  }
  void strike(Enemy& e,float damage){
    if(!e.active||phase!=Phase::Playing)return;
    e.hp-=damage;
    e.flash=.17f;
    impact(e.x,e.y,e.role==EnemyRole::Hydra?30.f:14.f,.18f);
    if(e.hp<=0){
      e.active=false;
      if(e.role==EnemyRole::Hydra){bossDefeated=true;phase=Phase::Won;impact(e.x,e.y,92.f,.6f);}
      else ++kills;
    }
  }
  void fireBolt(){
    for(auto& shot:shots)if(!shot.active){
      const Enemy* target=nullptr;float closest=1e15f;
      for(const auto& e:enemies)if(e.active){
        const float d=distanceSquared(x,y,e.x,e.y);
        if(d<closest){closest=d;target=&e;}
      }
      float dx=1,dy=0;
      if(target){dx=target->x-x;dy=target->y-y;}
      const float len=std::max(.0001f,std::sqrt(dx*dx+dy*dy));
      shot={x,y,dx/len*650.f,dy/len*650.f,1.15f,true,
            hero==Hero::Z&&dragonPower>0?2.f:1.f};
      return;
    }
  }
  void fire(){fireBolt();} // Keep classic arena test API.
  void fireNova(){
    novaPulse=.24f;
    impact(x,y,116.f,.28f);
    const float damage=hero==Hero::Z&&dragonPower>0?3.5f:2.f;
    for(auto& e:enemies)if(e.active){
      const float radius=e.role==EnemyRole::Hydra?143.f:124.f;
      if(distanceSquared(x,y,e.x,e.y)<=radius*radius)strike(e,damage);
    }
  }
  void update(float dt,const Input& input){
    if(input.restart){reset();return;}
    if(input.switchHero){
      hero=hero==Hero::Z?Hero::Nyra:Hero::Z;
      dragonPower=0;nyraShield=0;heroSkillCooldown=0;
    }
    if(input.switchWeapon)weapon=weapon==Weapon::Bolt?Weapon::Nova:Weapon::Bolt;
    if(phase!=Phase::Playing)return;
    dt=clamp(dt,0.f,.05f);
    seconds+=dt;
    fireCooldown=std::max(0.f,fireCooldown-dt);
    dashCooldown=std::max(0.f,dashCooldown-dt);
    invulnerable=std::max(0.f,invulnerable-dt);
    heroSkillCooldown=std::max(0.f,heroSkillCooldown-dt);
    dragonPower=std::max(0.f,dragonPower-dt);
    nyraShield=std::max(0.f,nyraShield-dt);
    novaPulse=std::max(0.f,novaPulse-dt);
    for(auto& fx:impacts)if(fx.active){fx.life-=dt;if(fx.life<=0)fx.active=false;}
    for(auto& e:enemies)if(e.active)e.flash=std::max(0.f,e.flash-dt);
    if(input.special&&heroSkillCooldown<=0){
      if(hero==Hero::Z){dragonPower=2.2f;heroSkillCooldown=7.f;}
      else{nyraShield=2.2f;heroSkillCooldown=6.f;}
    }
    float dx=float(input.right)-float(input.left),dy=float(input.down)-float(input.up);
    const float len=std::sqrt(dx*dx+dy*dy);
    if(len>0){dx/=len;dy/=len;}
    if(input.dash&&dashCooldown<=0&&len>0){
      x=clamp(x+dx*116.f,26.f,Width-26.f);
      y=clamp(y+dy*116.f,26.f,Height-26.f);
      dashCooldown=.9f;invulnerable=std::max(invulnerable,.16f);
    }
    x=clamp(x+dx*270.f*dt,26.f,Width-26.f);
    y=clamp(y+dy*270.f*dt,26.f,Height-26.f);

    spawnCooldown-=dt;
    if(spawnCooldown<=0&&!bossSummoned){
      spawnCooldown+=std::max(.30f,.90f-float(wave-1)*.20f);
      spawnEnemy();
    }
    if(input.attack&&fireCooldown<=0){
      if(weapon==Weapon::Nova){fireCooldown=.55f;fireNova();}
      else{fireCooldown=hero==Hero::Z&&dragonPower>0?.11f:.19f;fireBolt();}
    }
    for(auto& e:enemies)if(e.active&&phase==Phase::Playing){
      const float vx=x-e.x,vy=y-e.y;
      const float distance=std::max(.0001f,std::sqrt(vx*vx+vy*vy));
      const float speed=e.role==EnemyRole::Runner?110.f+wave*10.f:
                        e.role==EnemyRole::Brute?43.f:
                        e.role==EnemyRole::Hydra?(hydraEnraged()?50.f:28.f):
                        58.f+float(wave-1)*18.f;
      e.x=clamp(e.x+vx/distance*speed*dt,18.f,Width-18.f);
      e.y=clamp(e.y+vy/distance*speed*dt,18.f,Height-18.f);
      const float touch=e.role==EnemyRole::Hydra?52.f:
                        e.role==EnemyRole::Brute?38.f:31.f;
      if(distance<touch)damagePlayer(e.role==EnemyRole::Brute?20.f:
                                    e.role==EnemyRole::Hydra?17.f:12.f);
    }
    if(bossSummoned&&phase==Phase::Playing){
      auto* h=const_cast<Enemy*>(hydra());
      if(h){
        if(hydraTell>0){
          hydraTell-=dt;
          if(hydraTell<=0){
            impact(h->x,h->y,100.f,.35f);
            if(distanceSquared(x,y,h->x,h->y)<100.f*100.f)damagePlayer(18.f);
          }
        }else{
          hydraAttackCooldown-=dt;
          if(hydraAttackCooldown<=0){
            hydraTell=.85f;
            hydraAttackCooldown=hydraEnraged()?1.95f:2.8f;
          }
        }
        if(hydraEnraged()){
          hydraMinionCooldown-=dt;
          if(hydraMinionCooldown<=0){
            hydraMinionCooldown=1.8f;
            spawnEnemy(true);
          }
        }
      }
    }
    for(auto& s:shots)if(s.active&&phase==Phase::Playing){
      s.x+=s.vx*dt;s.y+=s.vy*dt;s.life-=dt;
      if(s.life<=0||s.x<0||s.x>Width||s.y<0||s.y>Height){s.active=false;continue;}
      for(auto& e:enemies)if(e.active){
        const float radius=e.role==EnemyRole::Hydra?42.f:
                           e.role==EnemyRole::Brute?31.f:24.f;
        if(distanceSquared(s.x,s.y,e.x,e.y)<radius*radius){
          s.active=false;strike(e,s.damage);break;
        }
      }
    }
    wave=kills<8?1:kills<20?2:3;
    if(kills>=36&&!bossSummoned&&phase==Phase::Playing)spawnHydra();
  }
};
} // namespace moonfall
