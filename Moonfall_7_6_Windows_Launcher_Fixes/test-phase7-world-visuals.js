'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm');
const visuals=require('./phase7-world-visuals.js');
const art='data:image/png;base64,AAABAA==';
const source="const heroArt='"+art+"';function draw(){paintBackground();mfDrawAtmosphere();}"+
 "function damageEnemy(e,d){if(!playing||!enemies.includes(e)||e.hp<=0)return;e.hp-=d;}"+
 "function tick(){} function reset(){}";
const built=visuals.patchJs(source);
new vm.Script(built);
assert(built.includes(art),'embedded original art must be byte-identical');
assert.match(built,/mfP5RecordImpact\(e\.x,e\.y\)/);
assert.match(built,/mfWorldExploreStep\(\)/);
assert.match(built,/mfWorldDecorDraw\(\)/);
assert.throws(()=>visuals.patchJs('function tick(){}'),/hook mismatch/);
const html=visuals.patchHtml('<b id="mfWorldHud" aria-live="polite">World</b>');
assert(html.includes('id="mfDiscoveryHud"'));
let draws=0,bursts=0,saves=0;
const ctx={
 save(){},restore(){},translate(){},moveTo(){},lineTo(){},beginPath(){},arc(){},
 closePath(){},stroke(){draws++},fill(){},fillRect(){},strokeRect(){draws++},fillText(){},
};
const store={};
const scope={ctx,frame:20,playing:true,paused:false,skillOpen:false,gearOpen:false,
 W:940,H:600,stage:1,player:{x:30,y:30},enemies:[],souls:0,
 mfWorld:{region:'witchwood',flags:{}},mfWorldSave(){saves++},
 mfReducedEffects:false,mfReducedMotion:{matches:false},
 paintBackground(){},mfDrawAtmosphere(){},burst(){bursts++},
 document:{getElementById(id){return store[id]||(store[id]={textContent:''})}}
};
vm.createContext(scope);vm.runInContext(built,scope);
const run=code=>vm.runInContext(code,scope);
let v=run('mfWorldLandmark()');
assert(v.decorations.length===9);
assert(v.hx>100&&v.hx<scope.W-100&&v.hy>100&&v.hy<scope.H-100);
scope.enemies.push({x:400,y:300,hp:100});
run('damageEnemy(enemies[0],10)');
assert.equal(scope.enemies[0].hp,90);
assert.equal(run('mfP5Impacts.length'),1);
run('draw()');
assert(draws>0,'world scenery and hit effects must draw');
scope.player.x=v.hx;scope.player.y=v.hy;run('tick()');
assert.equal(scope.souls,15);
assert.equal(saves,1);
assert.equal(bursts,1);
assert.equal(run("mfWorld.flags['relic:witchwood:stage-1']"),true);
run('tick()');assert.equal(scope.souls,15,'relic is one-time');
assert.match(store.mfDiscoveryHud.textContent,/RELIC FOUND/);
scope.stage=2;scope.mfWorld.region='foundry';v=run('mfWorldLandmark()');
assert.equal(v.region,'foundry');run('draw()');
scope.mfReducedEffects=true;
run('damageEnemy(enemies[0],10)');
assert.equal(run('mfP5Impacts.length'),1,'reduced effects must avoid queuing new pulses');
run('draw()');
assert.equal(run('mfP5Impacts.length'),0,'reduced effects clears pulses');
scope.playing=false;
run('tick()');assert.equal(scope.souls,15,'no discoveries outside gameplay');
run('reset()');assert.equal(run('mfWorldDecorCache'),null);
console.log('PASS: Phase 7 hit effects, region scenery, single-use discovery, reduced effects, original art');
