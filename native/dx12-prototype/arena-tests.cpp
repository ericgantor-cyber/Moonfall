#include "arena.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using moonfall::Arena;
using moonfall::Input;
using moonfall::Phase;
int main(){
  Arena a,b;
  assert(a.phase==Phase::Playing&&a.health==100&&a.kills==0);
  assert(a.x==moonfall::Width/2&&a.y==moonfall::Height/2);
  a.update(0.5f,Input{false,false,false,true});
  assert(std::abs(a.x-(512.f+270.f*0.05f))<0.001f); // clamp runaway delta
  assert(std::abs(a.seconds-0.05f)<0.001f);
  Input dash;
  dash.right=true;dash.dash=true;
  const float before=a.x;
  a.update(0.016f,dash);
  assert(a.x>before+100.f&&a.dashCooldown>0.f&&a.invulnerable>0.f);
  const float after=a.x;
  a.update(0.016f,dash);
  assert(a.x-after<10.f); // cannot spam dash
  for(int i=0;i<300;i++)a.update(0.05f,Input{false,false,false,true});
  assert(a.x<=moonfall::Width-26.f);
  for(int i=0;i<300;i++)a.update(0.05f,Input{true,false,false,false});
  assert(a.y>=26.f);
  Arena seeded1,seeded2;
  for(int i=0;i<10;i++){seeded1.spawnEnemy();seeded2.spawnEnemy();}
  for(int i=0;i<moonfall::MaxEnemies;i++){
    assert(seeded1.enemies[i].x==seeded2.enemies[i].x);
    assert(seeded1.enemies[i].y==seeded2.enemies[i].y);
  }
  assert(seeded1.enemyCount()==10);
  for(int i=0;i<100;i++)seeded1.spawnEnemy();
  assert(seeded1.enemyCount()<=32);
  Arena combat;
  combat.spawnCooldown=1000.f;
  combat.enemies[0]={combat.x+45.f,combat.y,1.f,true};
  combat.update(.016f,Input{false,false,false,false,true});
  assert(combat.projectileCount()==1);
  for(int i=0;i<15;i++)combat.update(.016f,Input{});
  assert(combat.kills==1&&!combat.enemies[0].active);
  Arena collision;
  collision.spawnCooldown=1000.f;
  collision.enemies[0]={collision.x,collision.y,2.f,true};
  collision.update(.016f,Input{});
  assert(collision.health==88.f);
  collision.update(.016f,Input{});
  assert(collision.health==88.f); // temporary invulnerability works
  for(int i=0;i<200;i++){
    collision.update(.016f,Input{});
    if(collision.phase==Phase::Lost)break;
  }
  assert(collision.phase==Phase::Lost&&collision.health==0.f);
  const auto lostX=collision.x;
  collision.update(.016f,Input{false,false,false,true});
  assert(collision.x==lostX);
  collision.update(.016f,Input{false,false,false,false,false,false,true});
  assert(collision.phase==Phase::Playing&&collision.health==100.f);
  Arena victory;
  victory.spawnCooldown=1000.f;
  victory.kills=35;victory.wave=3;
  victory.enemies[0]={victory.x,victory.y+50.f,1.f,true};
  victory.shots[0]={victory.x,victory.y+50.f,0,0,1.f,true};
  victory.update(.016f,Input{});
  assert(victory.kills==36&&victory.phase==Phase::Won);
  victory.update(.016f,Input{false,false,false,false,false,false,true});
  assert(victory.kills==0&&victory.phase==Phase::Playing);
  Arena limited;
  for(auto& shot:limited.shots)shot={500,200,0,0,1.f,true};
  limited.fire();
  assert(limited.projectileCount()==moonfall::MaxShots);
  // Early, deliberately isolated hero skill prototypes, not parity with the full game.
  Arena heroes;
  assert(heroes.hero==moonfall::Hero::Z);
  Input toggle{};toggle.switchHero=true;
  heroes.update(.016f,toggle);
  assert(heroes.hero==moonfall::Hero::Nyra);
  heroes.spawnCooldown=1000.f;
  heroes.enemies[0]={heroes.x,heroes.y,2.f,true};
  Input skill{};skill.special=true;
  heroes.update(.016f,skill);
  assert(heroes.nyraShield>2.f&&heroes.health==100.f);
  assert(heroes.heroSkillCooldown>0.f);
  for(int i=0;i<40;i++)heroes.update(.016f,Input{});
  assert(heroes.health==100.f);
  heroes.update(.016f,toggle);
  assert(heroes.hero==moonfall::Hero::Z);
  assert(heroes.nyraShield==0.f);
  heroes.enemies[0].active=false;
  heroes.update(.016f,skill);
  assert(heroes.dragonPower>2.f);
  assert(heroes.heroSkillCooldown>0.f);
  heroes.enemies[0]={heroes.x+45.f,heroes.y,2.f,true};
  Input attack{};attack.attack=true;
  heroes.update(.016f,attack);
  assert(heroes.projectileCount()==1);
  assert(heroes.shots[0].damage==2.f);
  for(int i=0;i<15;i++)heroes.update(.016f,Input{});
  assert(heroes.kills==1);
  heroes.update(.016f,Input{false,false,false,false,false,false,true});
  assert(heroes.hero==moonfall::Hero::Z && heroes.dragonPower==0 && heroes.heroSkillCooldown==0);
  std::cout<<"PASS: DX12 arena input, fixed-step bounds, dash, seeded waves, bullets, AI, health, restart, victory\n";
}
