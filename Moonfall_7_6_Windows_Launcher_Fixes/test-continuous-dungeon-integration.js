'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm');
const integration=require('./continuous-dungeon-integration.js');
const artwork='data:image/png;base64,AAABAA==';
const fixture='const heroImage="'+artwork+'";'+
 'let frame=1,W=960,H=540,stage=1,playing=true,paused=false,skillOpen=false,gearOpen=false;'+
 'let player={x:480,y:270,r:16},enemies=[{id:"initial",hp:100}],shots=[],drops=[{id:"coin"}];'+
 'let mfWorld={region:"witchwood",flags:{}};function mfWorldSave(){saved++}'+
 'function tick(){frame++;} function reset(){} function beginStage(){} '+
 'function draw(){paintBackground();}function loop(){pollController();tick();draw();requestAnimationFrame(loop)}';
const built=integration.patchJs(fixture);
new vm.Script(built);
assert(built.includes(artwork),'original character art preserved');
assert.throws(()=>integration.patchJs('function tick(){}'),/hook missing/);
assert.match(integration.patchHtml('<b id="mfWorldHud">World</b>'),/mfDungeonHud/);
const hud={textContent:''};
const ctx={save(){},restore(){},fillRect(){},strokeRect(){},fillText(){}};
const scope={saved:0,ctx,document:{getElementById:id=>id==='mfDungeonHud'?hud:null},paintBackground(){}};
vm.createContext(scope);vm.runInContext(built,scope);
const run=s=>vm.runInContext(s,scope);
run('beginStage()');
assert.equal(run('mfDungeonRoom.chamber'),0);
assert.match(hud.textContent,/Chamber 1\/14/);
run('enemies[0].hp=61;drops[0].id="coin-untaken"');
run('player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('enemies.length'),0,'new chamber does not create waves');
assert.equal(run('drops.length'),0,'new chamber starts without duplicated loot');
run('enemies.push({id:"resident",hp:73});drops.push({id:"treasure"});');

assert.equal(run('mfDungeonRoom.chamber'),1,'advance to next chamber');
assert.equal(run('player.x'),90,'arrive from left');
run('mfDungeonDoorCooldown=0;player.x=43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonRoom.chamber'),0,'return to previous chamber');
assert.equal(run('enemies[0].hp'),61,'enemy state preserved when returning');
assert.equal(run('drops[0].id'),'coin-untaken','loot state preserved when returning');
run('mfDungeonDoorCooldown=0;player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('enemies[0].id'),'resident','second chamber enemy restored without respawn');
assert.equal(run('drops[0].id'),'treasure','second chamber loot restored');

run('mfDungeonEnter(0,13);mfDungeonDoorCooldown=0;player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonRoom.dungeon'),1,'cross to next dungeon');
assert.equal(run('mfDungeonRoom.chamber'),0);
assert.equal(run('mfWorld.flags["dungeon:visited:witchwood:1:0"]'),true);
run('mfWorld.region="foundry";player.x=480;player.y=270;tick()');
assert.equal(run('mfDungeonRoom.region'),'foundry');
run('mfWorld.region="witchwood";tick()');
assert.equal(run('mfDungeonRoom.dungeon'),1,'resume visited dungeon for region');
run('reset()');
assert.equal(run('mfDungeonRoom'),null);
assert.equal(run('mfDungeonSnapshots.size'),0,'new run drops old chamber snapshots');
assert.equal(run('Object.keys(mfWorld.flags).filter(k=>k.startsWith("dungeon:")).length'),0);
assert(scope.saved>0);
console.log('PASS: 5x14 traversal, chamber-local enemies and loot, backwards travel, region recall and reset');
