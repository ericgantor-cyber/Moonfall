'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm');
const nav=require('./phase9-dungeon-ai-core.js');
const p8=require('./phase8-dungeon-integration.js'),p9=require('./phase9-dungeon-integration.js');
const wall={x:160,y:80,w:40,h:80};
assert(nav.segmentRectHit({x:50,y:110},{x:270,y:110},wall));
assert(nav.segmentRectHit({x:180,y:0},{x:180,y:270},wall));
assert(!nav.segmentRectHit({x:50,y:10},{x:270,y:10},wall));
assert(!nav.segmentRectHit({x:0,y:0},{x:270,y:270},{x:120,y:0,w:30,h:30}));
assert(nav.shotBlocked({x:20,y:110},{x:350,y:110},4,[wall]));
assert(!nav.shotBlocked({x:20,y:10},{x:350,y:10},4,[wall]));
assert.equal(nav.obstacleBetween({x:20,y:110},{x:350,y:110},[wall]),wall);
assert.equal(nav.obstacleBetween({x:20,y:10},{x:350,y:10},[wall]),null);
const waypoint=nav.chooseWaypoint({x:30,y:110},{x:360,y:110},[wall],8);
assert.notDeepEqual(waypoint,{x:360,y:110},'obstruction requires a waypoint');
assert.equal(nav.encounterCount(1,0),0);
assert.equal(nav.encounterCount(1,1),2);
assert.equal(nav.encounterCount(1,2),3);
assert.equal(nav.encounterCount(2,2),0,'boss stages must not add mobs');
const art='data:image/png;base64,AAABAA==';
const fixture="const heroImage='"+art+"';"+
 "let frame=1,W=960,H=540,stage=1,playing=true,paused=false,skillOpen=false,gearOpen=false,mfReducedEffects=false;"+
 "let mfWorld={region:'witchwood',flags:{}},player={x:480,y:270,r:16},enemies=[],shots=[];"+
 "function mfWorldSave(){saveCalls++}function mfCreateMob(e){return {...e,mobVariant:{role:'flanker'}}}"+
 "function receiveDamage(d){damageCalls+=d}"+
 "function mfRoleMove(e,a,s){e.x+=Math.cos(a)*s;e.y+=Math.sin(a)*s;}"+
 "function mfEvade(){const oldX=player.x,oldY=player.y,dy=0,len=1;player.y=Math.max(70,Math.min(H-30,player.y+dy/len*115));}"+
 "function reset(){}function beginStage(){}"+
 "function tick(){frame++;for(const e of enemies)if(e.id==='walker')mfRoleMove(e,0,190);for(const s of shots){s.x+=s.vx;s.y+=s.vy;}}"+
 "function draw(){paintBackground();}"+
 "function loop(){pollController();tick();draw();requestAnimationFrame(loop)}";
const built=p9.patchJs(p8.patchJs(fixture));
new vm.Script(built);
assert(built.includes(art),'all hero image bytes must remain untouched');
assert.match(built,/mfD9MoveEnemy\(e,a,s\)/);
assert.match(built,/mfD9Enter\(\)/);
assert.throws(()=>p9.patchJs('function tick(){}'),/hook mismatch/);
const ui={textContent:''};
const scope={saveCalls:0,damageCalls:0,ctx:{save(){},restore(){},fillRect(){},strokeRect(){},fillText(){}},
 paintBackground(){},document:{getElementById(){return ui}}};
vm.createContext(scope);vm.runInContext(built,scope);
const run=s=>vm.runInContext(s,scope);
run('beginStage()');
assert.equal(run('mfDungeonChamber'),0);
run('enemies.push({id:"entrance",x:400,y:300,r:16,hp:100})');
const wallP=run('mfDungeonLayout.pillars[0]');
run('shots.push({x:mfDungeonLayout.pillars[0].x-75,y:mfDungeonLayout.pillars[0].y+mfDungeonLayout.pillars[0].h/2,vx:160,vy:0,r:4})');
run('tick()');assert.equal(run('shots.length'),0,'fast bullets must be occluded before tick');
run('enemies.push({id:"walker",x:mfDungeonLayout.pillars[0].x-60,y:mfDungeonLayout.pillars[0].y+mfDungeonLayout.pillars[0].h/2,r:12,hp:90})');
run('player.x=mfDungeonLayout.pillars[0].x+170;player.y=mfDungeonLayout.pillars[0].y+mfDungeonLayout.pillars[0].h/2');
run('tick()');
assert.equal(run('mfD8.hitCircleRect(enemies.find(e=>e.id==="walker").x,enemies.find(e=>e.id==="walker").y,12,mfDungeonLayout.pillars[0])'),false,
 'AI cannot tunnel through walls');
run('player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonChamber'),1);
assert.equal(run('enemies.length'),2,'first crossroads visit spawns bounded encounter');
run('enemies.splice(0,1)');
run('mfDungeonDoorCooldown=0;player.x=43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonChamber'),0);
assert.equal(run('enemies.some(e=>e.id==="entrance")'),true,'entrance enemies must restore');
run('mfDungeonDoorCooldown=0;player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonChamber'),1);
assert.equal(run('enemies.length'),1,'cleared encounter should not respawn on reentry');
run('mfDungeonDoorCooldown=0;player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonChamber'),2);
assert.equal(run('enemies.length'),3,'sanctum has separate encounter');
assert(run('mfD9Rooms.size<=3'),'per-stage enemy snapshots must remain bounded');
run('stage=2;beginStage();enemies.push({boss:true,hp:500,x:400,y:300,r:32});player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonChamber'),0,'active boss must prevent exit');
assert.equal(run('enemies.some(e=>e.boss&&e.hp>0)'),true);
console.log('PASS: Phase 9 LOS waypoints, enemy room persistence, boss locks, shot occlusion, geometry and hero art');
