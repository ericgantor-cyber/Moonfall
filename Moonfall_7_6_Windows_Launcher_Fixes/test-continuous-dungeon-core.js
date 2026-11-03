'use strict';
const assert=require('node:assert/strict');
const {DUNGEONS_PER_REGION,CHAMBERS_PER_DUNGEON,REGIONS,roomAt,neighbor,blockedSegment,resolveMotion,circleHitsRect}=require('./continuous-dungeon-core.js');
assert.equal(DUNGEONS_PER_REGION,5);
assert.equal(CHAMBERS_PER_DUNGEON,14);
for(const region of REGIONS){
 const seen=new Set();
 for(let dungeon=0;dungeon<5;dungeon++)for(let chamber=0;chamber<14;chamber++){
  const room=roomAt({region,dungeon,chamber});
  assert(!seen.has(room.id));seen.add(room.id);
  assert.equal(room.blockers.length,4);
  assert(!room.blockers.some(b=>circleHitsRect(480,270,16,b)),'central lane blocked');
  const next=neighbor(region,dungeon,chamber,'right');
  if(next)assert.deepEqual(neighbor(next.region,next.dungeon,next.chamber,'left'),{region,dungeon,chamber});
 }
 assert.equal(seen.size,70);
 assert.equal(neighbor(region,0,0,'left'),null);
 assert.equal(neighbor(region,4,13,'right'),null);
 assert.deepEqual(neighbor(region,0,13,'right'),{region,dungeon:1,chamber:0});
}
const first=roomAt({region:'foundry',dungeon:2,chamber:7});
assert.deepEqual(first,roomAt({region:'foundry',dungeon:2,chamber:7}),'deterministic layout');
assert.throws(()=>roomAt({region:'invalid'}));
assert.throws(()=>roomAt({dungeon:5}));
assert.throws(()=>roomAt({chamber:14}));
const wall=first.blockers[0],y=wall.y+wall.h/2;
assert(blockedSegment({x:wall.x-80,y},{x:wall.x+wall.w+80,y},8,[wall]),'fast shot must hit wall');
const step=resolveMotion({x:wall.x-80,y},{x:wall.x+wall.w+80,y},12,[wall]);
assert.equal(step.x,wall.x-12,'swept dash must not pass through pillar');
assert(!blockedSegment({x:480,y:270},{x:490,y:270},4,first.blockers));
console.log('PASS: continuous 5x14 dungeon topology per region, deterministic geometry, no wave code, swept collision and projectile occlusion');
