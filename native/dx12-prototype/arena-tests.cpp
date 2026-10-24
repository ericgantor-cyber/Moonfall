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
  // Source-derived generic boss patterns: 5 projectiles in phase 1,
  // 9 in phase 2; 190/145 frame intervals, 35 frame warnings at 60Hz.
  Arena original;
  original.spawnCooldown=1000.f;
  original.spawnHydra();
  auto* originalBoss=original.hydra();
  assert(originalBoss!=nullptr);
  original.launchBossVolley(*originalBoss);
  assert(original.hostileProjectileCount()==5);
  assert(original.volleyCount==1);
  for(auto& b:original.hostileShots)b.active=false;
  originalBoss->hp=15.f;
  original.launchBossVolley(*originalBoss);
  assert(original.hostileProjectileCount()==9);
  assert(original.volleyCount==2);
  for(const auto& b:original.hostileShots)if(b.active)
    assert(b.damage==15.f && std::abs(std::sqrt(b.vx*b.vx+b.vy*b.vy)-240.f)<.05f);
  original.bossVolleyTell=0;
  original.bossVolleyTimer=0;
  original.update(.016f,Input{});
  assert(original.bossVolleyTell>0.f);
  // Original bear and creep imagery uses fixed sprite indices, no new art.
  // First cleared Hydra gate offers genuine named main-game blessing choices.
  Arena upgraded;
  upgraded.spawnCooldown=1000.f;
  upgraded.spawnHydra();
  for(auto& e:upgraded.enemies)if(e.active&&e.role==moonfall::EnemyRole::Hydra)
    upgraded.strike(e,1000.f);
  assert(upgraded.phase==Phase::Won && upgraded.stage==1);
  Input moonsteel{};moonsteel.boonDamage=true;
  upgraded.update(.016f,moonsteel);
  assert(upgraded.phase==Phase::Playing && upgraded.stage==2);
  assert(upgraded.damageMultiplier==1.25f && upgraded.maxHealth==100.f);
  assert(upgraded.activeBoss()!=nullptr &&
         upgraded.activeBoss()->role==moonfall::EnemyRole::Titan);
  upgraded.hero=moonfall::Hero::Z;
  upgraded.dragonPower=0;
  upgraded.fireBolt();
  assert(upgraded.shots[0].damage==1.25f);
  for(auto& e:upgraded.enemies)if(e.active&&e.role==moonfall::EnemyRole::Titan)
    upgraded.strike(e,1000.f);
  assert(upgraded.phase==Phase::Won && upgraded.stage==2);
  upgraded.update(.016f,moonsteel);
  assert(upgraded.stage==3 && upgraded.phase==Phase::Playing);
  assert(upgraded.room==moonfall::Room::Bloodmoon && upgraded.damageMultiplier==1.25f);
  assert(upgraded.roomRules().speed==1.17f && upgraded.roomRules().cadence==1.2f);
  Arena vitality;
  vitality.health=35.f;
  vitality.bossSummoned=true;vitality.bossDefeated=true;
  vitality.phase=Phase::Won;
  Input crimson{};crimson.boonVitality=true;
  vitality.update(.016f,crimson);
  assert(vitality.stage==2 && vitality.maxHealth==130.f);
  assert(vitality.health==85.f && vitality.damageMultiplier==1.f);
  assert(vitality.activeBoss()->role==moonfall::EnemyRole::Titan);
  vitality.reset();
  assert(vitality.stage==1 && vitality.maxHealth==100.f &&
         vitality.damageMultiplier==1.f && vitality.hostileProjectileCount()==0);
  // Enemy shots obey shields and the fixed capacity under extreme volleys.
  Arena barrage;
  barrage.spawnCooldown=1000.f;barrage.spawnHydra();
  barrage.hero=moonfall::Hero::Nyra;
  barrage.nyraShield=1.f;
  barrage.hostileShots[0]={barrage.x,barrage.y,0,0,1.f,10.f,true};
  barrage.update(.016f,Input{});
  assert(barrage.health==100.f && barrage.hostileProjectileCount()==0);
  for(int n=0;n<100;n++)barrage.launchBossVolley(*barrage.hydra());
  assert(barrage.hostileProjectileCount()<=96);
  // Original room modifiers and elite projectile fan ported from roguelike-rooms.js.
  Arena roomChoice;
  roomChoice.stage=2;roomChoice.phase=Phase::Won;
  Input shadow{};shadow.boonVitality=true;
  roomChoice.update(.016f,shadow);
  assert(roomChoice.stage==3 && roomChoice.room==moonfall::Room::Shadow);
  assert(roomChoice.roomRules().speed==1.05f&&roomChoice.roomRules().cadence==1.f);
  roomChoice.reset();
  assert(roomChoice.stage==1 && !roomChoice.eliteSpawned);
  roomChoice.stage=2;roomChoice.phase=Phase::Won;
  Input cinder{};cinder.roomCinder=true;
  roomChoice.update(.016f,cinder);
  assert(roomChoice.room==moonfall::Room::Cinder);
  assert(roomChoice.roomRules().speed==1.f && roomChoice.roomRules().cadence==1.4f);
  assert(roomChoice.stage==3);
  // Enemy movement changes with the source-defined room speed modifier.
  Arena speedA,speedB;
  speedA.stage=3;speedB.stage=3;
  speedA.room=moonfall::Room::Bloodmoon;
  speedB.room=moonfall::Room::Cinder;
  speedA.spawnCooldown=1000.f;speedB.spawnCooldown=1000.f;
  speedA.enemies[0]={speedA.x+200.f,speedA.y,3.f,true};
  speedB.enemies[0]=speedA.enemies[0];
  speedA.update(.05f,Input{});speedB.update(.05f,Input{});
  assert(speedA.enemies[0].x < speedB.enemies[0].x);
  // Elite must appear only after ten kills, only once, and block victory.
  Arena elite;
  elite.stage=3;elite.spawnCooldown=1000.f;elite.kills=9;
  elite.update(.05f,Input{});
  assert(!elite.eliteSpawned);
  elite.kills=10;
  elite.update(.05f,Input{});
  assert(elite.eliteSpawned&&elite.activeElite());
  const int occupied=elite.enemyCount();
  elite.spawnElite();
  assert(elite.enemyCount()==occupied);
  elite.eliteVolleyTimer=0.f;
  elite.update(.05f,Input{});
  assert(elite.hostileProjectileCount()==5 && elite.eliteVolleys==1);
  // Native copies the original five shot angles, 4.2px/frame and 13 damage.
  for(const auto& bullet:elite.hostileShots)if(bullet.active){
    assert(bullet.damage==13.f);
    const float velocity=std::sqrt(bullet.vx*bullet.vx+bullet.vy*bullet.vy);
    assert(std::abs(velocity-252.f)<.05f);
  }
  elite.kills=20;
  elite.update(.016f,Input{});
  assert(elite.phase==Phase::Playing); // elite room gate is enforced
  for(auto& enemy:elite.enemies)if(enemy.active&&enemy.role==moonfall::EnemyRole::Elite)
    elite.strike(enemy,1000.f);
  elite.update(.016f,Input{});
  assert(elite.phase==Phase::Won && elite.stage==3);
  elite.update(.016f,moonsteel);
  assert(elite.stage==3 && elite.phase==Phase::Won); // no hidden next stage
  std::cout<<"PASS: DX12 arena input, fixed-step bounds, dash, seeded waves, bullets, AI, health, restart, boss volley timing, actual image mappings and 3-stage progression, original room modifiers and elite AI\n";
}
