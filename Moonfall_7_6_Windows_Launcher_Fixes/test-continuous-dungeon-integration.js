'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm');
const integration=require('./continuous-dungeon-integration.js');
const artwork='data:image/png;base64,AAABAA==';
const fixture='const heroImage="'+artwork+'";'+
 'let frame=1,W=960,H=540,stage=1,playing=true,paused=false,skillOpen=false,gearOpen=false;'+
 'let player={x:480,y:270,r:16};'+
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
run('player.x=W-43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonRoom.chamber'),1,'advance to next chamber');
assert.equal(run('player.x'),90,'arrive from left');
run('mfDungeonDoorCooldown=0;player.x=43;player.y=H*.5;tick()');
assert.equal(run('mfDungeonRoom.chamber'),0,'return to previous chamber');
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
assert.equal(run('Object.keys(mfWorld.flags).filter(k=>k.startsWith("dungeon:")).length'),0);
assert(scope.saved>0);
console.log('PASS: 5x14 traversal, backwards travel, region recall, HUD, persistence and reset');
