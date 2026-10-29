'use strict';
const assert=require('node:assert/strict');
const vm=require('node:vm');
const c=require('./phase11-regional-core.js');
const p8=require('./phase8-dungeon-integration.js');
const p9=require('./phase9-dungeon-integration.js');
const p10=require('./phase10-tactics-integration.js');
const p11=require('./phase11-regional-integration.js');
assert.equal(Object.keys(c.REGIONAL_BOSSES).length,5);
const attacks=new Set();
for(const region of Object.keys(c.REGIONAL_BOSSES)){
 const profile=c.regionBoss(region);
 attacks.add(profile.attack);
 const bullets=c.regionalVolley(profile,.25,1.5);
 assert.equal(bullets.length,profile.count);
 assert(bullets.every(x=>Number.isFinite(x.angle)&&Number.isFinite(x.speed)));
}
assert.equal(attacks.size,5,'each biome needs a distinct boss attack');
const b={boss:true,hp:900,max:1000};
assert.equal(c.regionBossAction(b,'foundry',100,0),null);
assert.equal(c.regionBossAction(b,'foundry',259,0),null);
assert.equal(c.regionBossAction(b,'foundry',260,0).kind,'warn');
assert.equal(c.regionBossAction(b,'foundry',314,0),null);
const fired=c.regionBossAction(b,'foundry',315,0);
assert.equal(fired.kind,'fire');
assert.equal(fired.profile.attack,'crossfire');
assert.equal(c.regionBossAction(b,'foundry',316,0),null);
assert.equal(c.regionBossAction(b,'foundry',600,230).kind,'warn');
assert.equal(c.regionBossAction(b,'foundry',655,230),null,'full projectile budget skips attack');
assert.equal(c.regionBossAction(b,'witchwood',656,0),null,'changing regions resets boss schedule');
assert.equal(c.regionBossAction({boss:false,hp:50,max:50},'city',500,0),null);
assert.equal(c.canClaimRune({region:'city',stage:3,chamber:1,player:{x:50,y:50},
 point:{x:50,y:50},enemies:[],flags:{}}),true);
assert.equal(c.canClaimRune({region:'city',stage:3,chamber:1,player:{x:50,y:50},
 point:{x:50,y:50},enemies:[{hp:1}],flags:{}}),false);
assert.equal(c.canClaimRune({region:'city',stage:2,chamber:1,player:{x:50,y:50},
 point:{x:50,y:50},enemies:[],flags:{}}),false);
const flags={'seal:witchwood:stage-1':true,'cleared:witchwood:stage-2':true};
assert(c.regionComplete(flags,'witchwood'));
assert(c.claimRegion(flags,'witchwood'));
assert(!c.claimRegion(flags,'witchwood'),'champion reward cannot repeat');
assert.equal(c.masteredRegions(flags).length,1);
const image='data:image/png;base64,AAABAA==';
const fixture="const originalHeroArtwork='"+image+"';"+
 "let W=960,H=540,frame=1,stage=1,playing=true,paused=false,skillOpen=false,gearOpen=false,mfReducedEffects=false,damageLevel=0,"+
 "mfWorld={region:'witchwood',flags:{}},player={x:480,y:270,r:16},enemies=[],shots=[],souls=0,killCount=0,stages=[null,{count:3},{count:3}];"+
 "function mfWorldSave(){saved++}function mfCreateMob(e){return e}function gainXP(n){xp+=n}"+
 "function burst(){}function receiveDamage(){}"+
 "function mfWorldOnStageClear(){mfWorld.flags['cleared:'+mfWorld.region+':stage-'+stage]=true;}"+
 "function mfRoleMove(e,a,s){e.x+=Math.cos(a)*s;e.y+=Math.sin(a)*s;}"+
 "function mfBossExtra(e){ if(!e.boss||e.hp<=0||!playing)return;}"+
 "function mfBossWarningsDraw(){}"+
 "function mfEvade(){const oldX=player.x,oldY=player.y,dy=0,len=1;player.y=Math.max(70,Math.min(H-30,player.y+dy/len*115));}"+
 "function reset(){damageLevel=0}function beginStage(){}"+
 "function next(){mfWorldOnStageClear();advanced++;}function tick(){frame++;}"+
 "function draw(){paintBackground();mfBossWarningsDraw();}"+
 "function loop(){pollController();tick();draw();requestAnimationFrame(loop)}";
const built=p11.patchJs(p10.patchJs(p9.patchJs(p8.patchJs(fixture))));
new vm.Script(built);
assert(built.includes(image),'character artwork must remain unchanged');
assert.throws(()=>p11.patchJs('function tick(){}'),/hook mismatch/);
assert.match(p11.patchHtml('<b id="mfP10Hud">Objectives</b>'),/mfP11Hud/);
const nodes=new Map();
const scope={saved:0,xp:0,advanced:0,paintBackground(){},
 document:{getElementById(id){if(!nodes.has(id))nodes.set(id,{textContent:''});return nodes.get(id)}},
 ctx:{save(){},restore(){},beginPath(){},arc(){},stroke(){},fillText(){},fillRect(){},strokeRect(){}}
};
vm.createContext(scope);vm.runInContext(built,scope);
const run=s=>vm.runInContext(s,scope);
run('beginStage()');
run('player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonChamber'),1);
assert.equal(run('enemies.length'),2);
run('enemies.length=0;player.x=W*.5;player.y=H*.52;tick()');
assert.equal(run("mfWorld.flags['rune:witchwood:stage-1']"),true);
assert.equal(run('souls'),10);
assert.equal(scope.xp,15);
run('mfDungeonDoorCooldown=0;player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonChamber'),2);
assert.equal(run('enemies.length'),3);
run('enemies.length=0;player.x=W*.5;player.y=H*.52;killCount=3;tick()');
assert.equal(run("mfWorld.flags['seal:witchwood:stage-1']"),true);
assert.equal(run('souls'),45);
assert.equal(scope.xp,45);
assert.equal(scope.advanced,1,'complete rune and sanctum seal allow stage progression');
run('tick()');
assert.equal(run('souls'),45,'rune and seal rewards cannot repeat');
run('stage=2;beginStage();next()');
assert.equal(run("mfWorld.flags['champion:witchwood']"),true);
assert.equal(run('souls'),225,'region victory pays 180 additional souls');
run('next()');assert.equal(run('souls'),225,'region reward paid once');
run('reset();beginStage()');
assert.equal(run('damageLevel'),1,'one permanent champion grants one damage level next run');
run('beginStage()');assert.equal(run('damageLevel'),1,'mastery bonus applied once per run');
scope.bossRef={boss:true,hp:900,max:1000,x:410,y:230,r:30};
run("stage=2;mfWorld.region='foundry';frame=100;enemies=[bossRef];mfP10BossThink(enemies[0])");
run('frame=260;mfP10BossThink(enemies[0])');
assert.equal(run('enemies[0].mfP11Warning'),315);
run('frame=315;mfP10BossThink(enemies[0])');
assert.equal(run('shots.length'),8,'Foundry boss uses eight-shot crossfire');
assert.match(nodes.get('mfP11Hud').textContent,/witchwood|Boss|Rune|REGION|region/i);
console.log('PASS: five boss archetypes, windups, rune gates, seal rewards, one-time champion mastery and original hero art');
