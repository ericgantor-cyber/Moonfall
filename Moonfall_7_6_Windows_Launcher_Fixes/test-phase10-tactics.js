'use strict';
const assert=require('node:assert/strict');
const vm=require('node:vm');
const C=require('./phase10-tactics-core.js');
const p8=require('./phase8-dungeon-integration.js');
const p9=require('./phase9-dungeon-integration.js');
const p10=require('./phase10-tactics-integration.js');
assert.equal(C.bossPhase({boss:true,hp:100,max:100}),1);
assert.equal(C.bossPhase({boss:true,hp:70,max:100}),2);
assert.equal(C.bossPhase({boss:true,hp:35,max:100}),3);
assert.equal(C.bossPhase({boss:false,hp:30,max:100}),0);
let b={boss:true,hp:60,max:100};
assert.equal(C.bossAction(b,1,3,0),null);
assert.equal(C.bossAction(b,90,3,0),null);
assert.equal(C.bossAction(b,91,3,0).kind,'warn');
assert.equal(C.bossAction(b,135,3,0),null);
const fired=C.bossAction(b,136,3,0);
assert.equal(fired.kind,'fire');
assert.equal(fired.count,5);
assert.equal(C.bossAction(b,137,3,0),null,'boss should honor cooldown');
b.hp=30;
assert.equal(C.bossAction(b,138,3,0),null,'phase change resets tell');
assert.equal(C.bossAction(b,228,3,0).kind,'warn');
assert.equal(C.bossAction(b,273,3,220),null,'projectile budget cancels volley');
const room={width:960,height:540,pillars:[{x:220,y:120,w:70,h:60}],hazards:[{x:430,y:330,w:80,h:45}]};
for(let ch=1;ch<3;ch++)for(let i=0;i<ch+1;i++){
 const pos=C.formationPosition(room,ch,i,ch+1,21);
 assert(pos.x>0&&pos.x<960&&pos.y>0&&pos.y<540);
 for(const rect of [...room.pillars,...room.hazards])assert(!C.circleHitsRect(pos.x,pos.y,21,rect),'formations must not spawn in hazards or walls');
}
assert.throws(()=>C.formationPosition(room,2,4,3));
const key=C.objectiveKey('witchwood',1);
const seal={x:500,y:280};
const conditions={stage:1,chamber:2,player:{x:500,y:280},seal,enemies:[],flags:{},region:'witchwood'};
assert(C.canClaimSeal(conditions));
assert(!C.canClaimSeal({...conditions,chamber:1}));
assert(!C.canClaimSeal({...conditions,enemies:[{hp:1}]}));
assert(!C.canClaimSeal({...conditions,flags:{[key]:true}}));
assert(!C.canClaimSeal({...conditions,stage:2}));
const image='data:image/png;base64,AAABAA==';
const fixture="const heroArt='"+image+"';"+
 "let W=960,H=540,frame=1,stage=1,playing=true,paused=false,skillOpen=false,gearOpen=false,mfReducedEffects=false,"+
 "mfWorld={region:'witchwood',flags:{}},player={x:480,y:270,r:16},enemies=[],shots=[],souls=0,killCount=0,stages=[null,{count:3},{count:3}];"+
 "function mfWorldSave(){saved++;}function mfCreateMob(e){return e;}function gainXP(x){xp+=x}function burst(){}function receiveDamage(){}"+
 "function mfRoleMove(e,a,s){e.x+=Math.cos(a)*s;e.y+=Math.sin(a)*s;}"+
 "function mfBossExtra(e){ if(!e.boss||e.hp<=0||!playing)return;}"+
 "function mfBossWarningsDraw(){}"+
 "function mfEvade(){const oldX=player.x,oldY=player.y,dy=0,len=1;player.y=Math.max(70,Math.min(H-30,player.y+dy/len*115));}"+
 "function reset(){}function beginStage(){}function next(){advanced++;}function tick(){frame++;}"+
 "function draw(){paintBackground();mfBossWarningsDraw();}"+
 "function loop(){pollController();tick();draw();requestAnimationFrame(loop)}";
const built=p10.patchJs(p9.patchJs(p8.patchJs(fixture)));
new vm.Script(built);
assert(built.includes(image),'hero source art must remain identical');
assert.match(built,/mfP10BossThink/);
assert.match(built,/mfP10FormationPosition/);
assert.match(p10.patchHtml('<b id="mfDungeonHud">Dungeon</b>'),/id="mfP10Hud"/);
const huds=new Map();
let saved=0,xp=0,advanced=0;
const ctx={save(){},restore(){},beginPath(){},arc(){},stroke(){},fillText(){},fillRect(){},strokeRect(){}};
const scope={ctx,paintBackground(){},get saved(){return saved},get xp(){return xp},get advanced(){return advanced},
 document:{getElementById(id){if(!huds.has(id))huds.set(id,{textContent:''});return huds.get(id)}}};
vm.createContext(scope);
vm.runInContext(built,scope);
const run=s=>vm.runInContext(s,scope);
run('beginStage()');
run('next()');
assert.equal(advanced,0,'stage must not clear while seal incomplete');
run('player.x=W-43;player.y=H*.5;tick()');
run('mfDungeonDoorCooldown=0;player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonChamber'),2);
assert.equal(run('enemies.length'),3,'sanctum formation must spawn three guardians');
const loc=run('({x:mfDungeonLayout.width*.5,y:mfDungeonLayout.height*.52})');
run('enemies.length=0');
run('player.x=W*.5;player.y=H*.52;killCount=3;tick()');
assert.equal(saved>=1,true);
assert.equal(run("mfWorld.flags['seal:witchwood:stage-1']"),true);
assert.equal(run('souls'),35,'reward should be granted once');
assert.equal(xp,30,'reward should grant XP');
assert.equal(advanced,1,'stage continues after completed objective and kills');
run('tick()');
assert.equal(run('souls'),35);
assert.equal(advanced,1);
assert.match(huds.get('mfP10Hud').textContent,/activated/);
const boss={boss:true,hp:300,max:1000,x:250,y:210,r:30};
scope.bossRef=boss;run('stage=2;beginStage()');
run('enemies.push(bossRef);frame=50;mfBossExtra(enemies[0])');
assert.equal(run('enemies[0].mfP10Phase'),3);
run('frame=140;mfBossExtra(enemies[0])');
assert(run('enemies[0].mfP10TellUntil')>140,'boss warning must start');
run('frame=185;mfBossExtra(enemies[0])');
assert.equal(run('shots.length'),9,'boss volley fires after windup');
run('draw()');
console.log('PASS: boss telegraph pacing, formations, seal gating, one-time rewards and original art');
