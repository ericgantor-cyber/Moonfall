'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm');
const transform=require('./boss-combat-stage.js');
const fixture='let frame=0,stage=2,playing=true,shots=[],enemies=[],player={x:300,y:250},bursts=0;'+
'function burst(){bursts++}function mfRoleAttack(e){}function mfEliteAttack(e){}'+
'function tickBoss(e){mfRoleAttack(e);mfEliteAttack(e);}'+
'function draw(){paintBackground();}';
const source=transform.patchJs(fixture);
new vm.Script(source);
assert.throws(()=>transform.patchJs('function draw(){}'),/hook mismatch/);
const ctx={save(){},restore(){},beginPath(){},arc(){},stroke(){}};
const env={ctx,paintBackground(){}};vm.createContext(env);vm.runInContext(source,env);
const run=s=>vm.runInContext(s,env);
run('var boss={boss:true,hp:100,max:100,x:100,y:100,r:30};enemies.push(boss)');
run('tickBoss(boss)');
assert.equal(run('boss.mfNextBossAttack'),190,'each boss gets a delayed first attack');
run('frame=155;tickBoss(boss)');
assert.equal(run('boss.mfTellUntil'),190,'tell appears 35 frames before attack');
run('frame=190;tickBoss(boss)');
assert.equal(run('shots.length'),5,'phase one fires five projectiles');
assert.equal(run('boss.mfTellUntil'),0,'telegraph clears on fire');
run('boss.hp=40;frame=240;tickBoss(boss)');
assert.equal(run('boss.mfBossPhase'),2);
assert.equal(run('bursts'),1,'phase two announces once');
run('frame=380;tickBoss(boss)');
assert.equal(run('shots.length'),14,'phase two adds nine shots');
run('boss.hp=20;frame=390;tickBoss(boss)');
assert.equal(run('boss.mfBossPhase'),3);
assert.equal(run('bursts'),2,'final phase announces once');
run('frame=525;tickBoss(boss)');
assert.equal(run('shots.length'),25,'final phase adds eleven shots');
run('boss.hp=100;frame=530;tickBoss(boss)');
assert.equal(run('boss.mfBossPhase'),3,'boss phase does not regress after healing');
run('var late={boss:true,hp:100,max:100,x:120,y:90,r:24};enemies.push(late);tickBoss(late)');
assert.equal(run('late.mfNextBossAttack'),720,'new boss has independent attack timer');
run('shots.length=210;frame=630;tickBoss(boss)');
assert.equal(run('shots.length'),210,'projectile budget prevents barrage');
assert.equal(run('boss.mfNextBossAttack'),735,'attack countdown continues when budget is full');
run('boss.hp=0;frame=735;tickBoss(boss)');
assert.equal(run('shots.length'),210,'dead boss cannot attack');

// Overlapping bosses cannot bypass the shared projectile limit by launching a full volley at 209 shots.
run('shots.length=209;boss.hp=100;boss.mfNextBossAttack=frame;frame=800;tickBoss(boss)');
assert.equal(run('shots.length'),210,'boss volley honors the strict shared soft cap');
run('shots.length=0;boss.hp=100;boss.mfTellUntil=frame+20;');
let tellStrokes=0;
env.ctx.stroke=()=>{tellStrokes++};
run('draw()');
assert.equal(tellStrokes,1,'living boss tell still visible');
run('boss.hp=0;draw()');
assert.equal(tellStrokes,1,'defeated boss never leaves an orphan warning');
// Three overlapping bosses over a long deterministic session, including changing HP phases.
// This is a logic stress test, not a real render or measured frame-rate benchmark.
run('enemies.length=0;shots.length=0;frame=0;');
run('for(let i=0;i<3;i++)enemies.push({boss:true,hp:100,max:100,x:80+i*140,y:100,r:24})');
const startStress=Date.now();
for(let frame=0;frame<6000;frame++){
 run('frame='+frame+';if(frame===1600)for(const e of enemies)e.hp=40;if(frame===3200)for(const e of enemies)e.hp=20;for(const e of enemies)tickBoss(e);');
 assert(run('shots.length')<=210,'multiple bosses never exceed shared projectile soft cap');
 if(frame%2===0)run('shots.splice(0,Math.min(3,shots.length))');
}
assert.equal(run('enemies.every(e=>e.mfBossPhase===3)'),true,'all bosses reach final phase');
console.log('Boss combat stress: 3 bosses over 6000 ticks in '+(Date.now()-startStress)+'ms; diagnostic only.');

console.log('PASS: independent boss cadence, telegraphs, three HP phases, budget and defeat');
