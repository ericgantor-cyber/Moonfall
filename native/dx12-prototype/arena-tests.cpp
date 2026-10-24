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
  assert(victory.kills==36&&victory.phase==Phase::Playing&&victory.bossSummoned);
  assert(victory.hydra()!=nullptr&&victory.hydra()->hp==32.f);
  for(auto& enemy:victory.enemies)if(enemy.active&&enemy.role==moonfall::EnemyRole::Hydra)
    victory.strike(enemy,999.f);
  assert(victory.bossDefeated&&victory.phase==Phase::Won);
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
  // Role AI: Runner advances faster than Brute at equal distance.
  Arena roles;roles.spawnCooldown=1000.f;
  roles.enemies[0]={roles.x+150.f,roles.y,2.f,true,moonfall::EnemyRole::Runner,2.f};
  roles.enemies[1]={roles.x+150.f,roles.y+40.f,8.f,true,moonfall::EnemyRole::Brute,8.f};
  const float runnerStart=roles.enemies[0].x,bruteStart=roles.enemies[1].x;
  roles.update(.05f,Input{});
  assert(runnerStart-roles.enemies[0].x > bruteStart-roles.enemies[1].x);
  assert(roles.enemies[1].hp==8.f);
  // A second selectable weapon hits nearby enemies without creating bullets.
  Arena nova;nova.spawnCooldown=1000.f;
  nova.enemies[0]={nova.x+70.f,nova.y,2.f,true,moonfall::EnemyRole::Chaser,2.f};
  Input blast{};blast.switchWeapon=true;blast.attack=true;
  nova.update(.016f,blast);
  assert(nova.weapon==moonfall::Weapon::Nova);
  assert(nova.kills==1&&nova.projectileCount()==0&&nova.novaPulse>0);
  assert(nova.fireCooldown>.5f);
  nova.enemies[0]={nova.x+70.f,nova.y,2.f,true};
  nova.update(.016f,Input{false,false,false,false,true});
  assert(nova.kills==1); // Nova cannot fire again during cooldown.
  nova.update(.016f,Input{false,false,false,false,false,false,false,false,false,true});
  assert(nova.weapon==moonfall::Weapon::Bolt);
  // The Hydra is an actual boss gate, with a visible telegraph and enrage.
  Arena boss;boss.spawnCooldown=1000.f;
  boss.spawnHydra();
  assert(boss.bossSummoned&&boss.hydra()!=nullptr&&boss.hydra()->maxHp==32.f);
  boss.hydraAttackCooldown=0;
  boss.update(.05f,Input{});
  assert(boss.hydraTell>0.f);
  for(auto& enemy:boss.enemies)if(enemy.active&&enemy.role==moonfall::EnemyRole::Hydra)
    boss.strike(enemy,17.f);
  assert(boss.hydraEnraged());
  boss.hydraMinionCooldown=0;
  boss.update(.05f,Input{});
  assert(boss.enemyCount()>=2);
  boss.update(.05f,Input{false,false,false,false,false,false,true});
  assert(!boss.bossSummoned&&boss.phase==Phase::Playing);
  // Shield stops boss telegraph damage while held.
  Arena shieldBoss;shieldBoss.spawnCooldown=1000.f;
  shieldBoss.x=512;shieldBoss.y=325;
  shieldBoss.spawnHydra();
  for(auto& enemy:shieldBoss.enemies)if(enemy.role==moonfall::EnemyRole::Hydra)enemy.y=250.f;
  shieldBoss.hero=moonfall::Hero::Nyra;
  shieldBoss.hydraAttackCooldown=0;
  Input guard{};guard.special=true;
  shieldBoss.update(.05f,guard);
  assert(shieldBoss.nyraShield>0&&shieldBoss.hydraTell>0);
  for(int i=0;i<18;i++)shieldBoss.update(.05f,Input{});
  assert(shieldBoss.health==100.f);
  // Deterministic roles and finite projectile/effect allocation across runs.
  Arena ra,rb;for(int i=0;i<9;i++){ra.spawnEnemy();rb.spawnEnemy();}
  for(int i=0;i<moonfall::MaxEnemies;i++)
    assert(ra.enemies[i].role==rb.enemies[i].role);
  for(int i=0;i<50;i++)ra.impact(100.f,100.f,20.f);
  int fxCount=0;for(const auto& fx:ra.impacts)if(fx.active)fxCount++;
  assert(fxCount<=moonfall::MaxImpacts);
  std::cout<<"PASS: DX12 arena input, fixed-step bounds, dash, seeded waves, bullets, AI, health, restart, Hydra boss victory\n";
}
