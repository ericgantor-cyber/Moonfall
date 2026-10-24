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
enum class EnemyRole { Chaser, Runner, Brute, Hydra, Titan, Elite };
enum class Room { Bloodmoon, Shadow, Cinder };
struct RoomRules {float speed=1.f,cadence=1.f;};
constexpr std::array<RoomRules,3> OriginalRoomRules{{{1.17f,1.2f},{1.05f,1.f},{1.f,1.4f}}};
constexpr const char* RoomNames[]={"Bloodmoon Arena","Shadow Labyrinth","Cinder Gauntlet"};
struct Input {
  bool up=false,down=false,left=false,right=false,attack=false,dash=false;
  bool restart=false,switchHero=false,special=false,switchWeapon=false;
  bool boonDamage=false,boonVitality=false,roomCinder=false;
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
struct HostileShot {float x=0,y=0,vx=0,vy=0,life=0,damage=0;bool active=false;};

struct Arena {
  float x=Width/2,y=Height/2;
  float health=100,seconds=0,fireCooldown=0,dashCooldown=0,invulnerable=0,spawnCooldown=.4f;
  float heroSkillCooldown=0,dragonPower=0,nyraShield=0;
  float hydraTell=0,hydraAttackCooldown=2.4f,hydraMinionCooldown=1.5f;
  float novaPulse=0;
  float bossVolleyTimer=190.f/60.f,bossVolleyTell=0;
  float damageMultiplier=1.f,maxHealth=100.f;
  int stage=1,volleyCount=0;
  Room room=Room::Bloodmoon;
  bool eliteSpawned=false;
  float eliteVolleyTimer=85.f/60.f;
  int eliteVolleys=0;
  Hero hero=Hero::Z;
  Weapon weapon=Weapon::Bolt;
  int kills=0,wave=1;
  Phase phase=Phase::Playing;
  bool bossSummoned=false,bossDefeated=false;
  std::uint32_t randomState=0xC0FFEEu;
  std::array<Enemy,MaxEnemies> enemies{};
  std::array<Shot,MaxShots> shots{};
  std::array<Impact,MaxImpacts> impacts{};
  std::array<HostileShot,96> hostileShots{};

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
  int hostileProjectileCount() const {int n=0;for(const auto& s:hostileShots)if(s.active)++n;return n;}
  const Enemy* hydra() const {
    for(const auto& e:enemies)if(e.active&&e.role==EnemyRole::Hydra)return &e;
    return nullptr;
  }
  Enemy* hydra() {
    for(auto& e:enemies)if(e.active&&e.role==EnemyRole::Hydra)return &e;
    return nullptr;
  }
  bool hydraEnraged() const {
    const auto* h=hydra();return h&&h->hp<=h->maxHp*.5f;
  }
  const Enemy* activeBoss() const {
    for(const auto& e:enemies)if(e.active&&(e.role==EnemyRole::Hydra||e.role==EnemyRole::Titan))return &e;
    return nullptr;
  }
  bool bossInSecondPhase() const {
    const auto* boss=activeBoss();return boss&&boss->hp<=boss->maxHp*.5f;
  }
  void reset(){const Hero h=hero;const Weapon w=weapon;*this=Arena{};hero=h;weapon=w;}
  RoomRules roomRules() const {return OriginalRoomRules[static_cast<int>(room)];}
  bool activeElite() const {for(const auto& e:enemies)if(e.active&&e.role==EnemyRole::Elite)return true;return false;}
  void impact(float ix,float iy,float radius,float life=.25f){
    for(auto& e:impacts)if(!e.active){e={ix,iy,radius,life,true};return;}
  }
  void spawnElite(){
    if(stage!=3||eliteSpawned||kills<10||phase!=Phase::Playing)return;
    for(auto& e:enemies)if(!e.active){
      // Prototype-scaled health, original elite location / source-defined attack.
      e={Width*.5f,125.f,24.f,true,EnemyRole::Elite,24.f,0.f};
      eliteSpawned=true;eliteVolleyTimer=85.f/60.f;
      impact(e.x,e.y,52.f,.45f);
      return;
    }
  }
  void launchEliteVolley(const Enemy& elite){
    // Source: roguelike-rooms.js mfEliteAttack, five aimed projectiles,
    // offsets +/-2*0.21, speed 4.2 px/frame, damage 13, every 85 frames.
    const float aim=std::atan2(y-elite.y,x-elite.x);
    for(int i=-2;i<=2;++i){
      const float angle=aim+float(i)*.21f;
      for(auto& bullet:hostileShots)if(!bullet.active){
        bullet={elite.x,elite.y,std::cos(angle)*4.2f*60.f,
                std::sin(angle)*4.2f*60.f,3.5f,13.f,true};
        break;
      }
    }
    ++eliteVolleys;
    impact(elite.x,elite.y,28.f,.24f);
  }
  void startRoomStage(Room choice){
    stage=3;room=choice;phase=Phase::Playing;
    bossSummoned=false;bossDefeated=false;eliteSpawned=false;
    kills=0;wave=3;spawnCooldown=.4f;eliteVolleyTimer=85.f/60.f;eliteVolleys=0;
    invulnerable=1.f;
    fireCooldown=0;bossVolleyTell=0;hydraTell=0;
    for(auto& e:enemies)e.active=false;
    for(auto& p:shots)p.active=false;
    for(auto& p:hostileShots)p.active=false;
    for(auto& fx:impacts)fx.active=false;
    health=std::min(maxHealth,health+35.f);
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
      bossVolleyTimer=190.f/60.f;bossVolleyTell=0;
      impact(e.x,e.y,80.f,.45f);
      return;
    }
  }
  void spawnTitan(){
    for(auto& e:enemies)if(!e.active){
      e={Width/2,100.f,42.f,true,EnemyRole::Titan,42.f,0};
      bossSummoned=true;bossDefeated=false;
      bossVolleyTimer=190.f/60.f;bossVolleyTell=0;volleyCount=0;
      impact(e.x,e.y,100.f,.6f);
      return;
    }
  }
  void startTitanStage(bool chooseVitality){
    stage=2;phase=Phase::Playing;bossSummoned=false;bossDefeated=false;
    kills=0;wave=3;spawnCooldown=1000.f;invulnerable=1.f;
    hydraTell=0;hydraAttackCooldown=2.4f;
    for(auto& e:enemies)e.active=false;
    for(auto& shot:shots)shot.active=false;
    for(auto& shot:hostileShots)shot.active=false;
    for(auto& fx:impacts)fx.active=false;
    if(chooseVitality){
      maxHealth+=30.f;health=std::min(maxHealth,health+50.f);
    }else{
      damageMultiplier*=1.25f;
      health=std::min(maxHealth,health+25.f);
    }
    spawnTitan();
  }
  void launchBossVolley(const Enemy& boss){
    // Faithful numeric port of mfBossExtra in boss-combat-stage.js:
    // 5 shots phase one, 9 phase two; stage%3 picks radial/aimed fan;
    // 4/3.1 px per 60Hz frame, 10/15 damage.
    constexpr float Pi=3.14159265358979323846f;
    const bool second=boss.hp<=boss.maxHp*.5f;
    const int n=second?9:5,pattern=stage%3;
    const float dir=std::atan2(y-boss.y,x-boss.x);
    const float speed=(pattern==1?4.f:3.1f)*60.f;
    for(int i=0;i<n;++i){
      const float angle=pattern==0?float(i)*2.f*Pi/float(n):
        dir+(float(i)-float(n-1)*.5f)*(pattern==1?.18f:.32f);
      for(auto& bullet:hostileShots)if(!bullet.active){
        bullet={boss.x,boss.y,std::cos(angle)*speed,std::sin(angle)*speed,
                3.5f,second?15.f:10.f,true};
        break;
      }
    }
    ++volleyCount;
    impact(boss.x,boss.y,52.f,.25f);
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
      if(e.role==EnemyRole::Hydra||e.role==EnemyRole::Titan){bossDefeated=true;phase=Phase::Won;impact(e.x,e.y,92.f,.6f);}
      else if(e.role!=EnemyRole::Elite)++kills;
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
            (hero==Hero::Z&&dragonPower>0?2.f:1.f)*damageMultiplier};
      return;
    }
  }
  void fire(){fireBolt();} // Keep classic arena test API.
  void fireNova(){
    novaPulse=.24f;
    impact(x,y,116.f,.28f);
    const float damage=(hero==Hero::Z&&dragonPower>0?3.5f:2.f)*damageMultiplier;
    for(auto& e:enemies)if(e.active){
      const float radius=e.role==EnemyRole::Hydra?143.f:124.f;
      if(distanceSquared(x,y,e.x,e.y)<=radius*radius)strike(e,damage);
    }
  }
  void update(float dt,const Input& input){
    if(input.restart){reset();return;}
    if(phase==Phase::Won&&stage==1&&(input.boonDamage||input.boonVitality)){
      startTitanStage(input.boonVitality);return;
    }
    if(phase==Phase::Won&&stage==2&&(input.boonDamage||input.boonVitality||input.roomCinder)){
      startRoomStage(input.roomCinder?Room::Cinder:input.boonVitality?Room::Shadow:Room::Bloodmoon);
      return;
    }
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
      spawnCooldown+=std::max(.30f,.90f-float(wave-1)*.20f)/(stage==3?roomRules().cadence:1.f);
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
                        e.role==EnemyRole::Titan?(bossInSecondPhase()?42.f:24.f):
                        e.role==EnemyRole::Elite?38.f:
                        58.f+float(wave-1)*18.f;
      const float adjustedSpeed=speed*(stage==3?roomRules().speed:1.f);
      e.x=clamp(e.x+vx/distance*adjustedSpeed*dt,18.f,Width-18.f);
      e.y=clamp(e.y+vy/distance*adjustedSpeed*dt,18.f,Height-18.f);
      const float touch=e.role==EnemyRole::Hydra||e.role==EnemyRole::Titan?52.f:
                        e.role==EnemyRole::Brute||e.role==EnemyRole::Elite?38.f:31.f;
      if(distance<touch)damagePlayer(e.role==EnemyRole::Brute||e.role==EnemyRole::Elite?20.f:
                                    e.role==EnemyRole::Hydra||e.role==EnemyRole::Titan?17.f:12.f);
    }
    if(bossSummoned&&phase==Phase::Playing){
      auto* h=hydra();
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
    // Original game boss projectile patterns, translated from 60fps ticks to seconds.
    if(phase==Phase::Playing){
      if(const auto* boss=activeBoss()){
        const bool second=bossInSecondPhase();
        if(bossVolleyTell>0){
          bossVolleyTell-=dt;
          if(bossVolleyTell<=0){
            launchBossVolley(*boss);
            bossVolleyTimer=(second?145.f:190.f)/60.f;
          }
        }else{
          bossVolleyTimer-=dt;
          if(bossVolleyTimer<=0)bossVolleyTell=35.f/60.f;
        }
      }
    }
    if(stage==3&&phase==Phase::Playing){
      // Elite spawns after half of the 20 required normal kills.
      spawnElite();
      if(activeElite()){
        eliteVolleyTimer-=dt;
        if(eliteVolleyTimer<=0){
          for(const auto& enemy:enemies)if(enemy.active&&enemy.role==EnemyRole::Elite){
            launchEliteVolley(enemy);break;
          }
          eliteVolleyTimer+=85.f/60.f;
        }
      }
    }
    for(auto& bullet:hostileShots)if(bullet.active&&phase==Phase::Playing){
      bullet.x+=bullet.vx*dt;bullet.y+=bullet.vy*dt;bullet.life-=dt;
      if(bullet.life<=0||bullet.x<0||bullet.x>Width||bullet.y<0||bullet.y>Height)
        {bullet.active=false;continue;}
      if(distanceSquared(bullet.x,bullet.y,x,y)<20.f*20.f){
        bullet.active=false;damagePlayer(bullet.damage);
      }
    }
    for(auto& s:shots)if(s.active&&phase==Phase::Playing){
      s.x+=s.vx*dt;s.y+=s.vy*dt;s.life-=dt;
      if(s.life<=0||s.x<0||s.x>Width||s.y<0||s.y>Height){s.active=false;continue;}
      for(auto& e:enemies)if(e.active){
        const float radius=e.role==EnemyRole::Hydra||e.role==EnemyRole::Titan?42.f:
                           e.role==EnemyRole::Brute||e.role==EnemyRole::Elite?31.f:24.f;
        if(distanceSquared(s.x,s.y,e.x,e.y)<radius*radius){
          s.active=false;strike(e,s.damage);break;
        }
      }
    }
    wave=kills<8?1:kills<20?2:3;
    if(stage==1&&kills>=36&&!bossSummoned&&phase==Phase::Playing)spawnHydra();
    if(stage==3&&kills>=20&&!activeElite()&&phase==Phase::Playing)phase=Phase::Won;
  }
};
} // namespace moonfall
