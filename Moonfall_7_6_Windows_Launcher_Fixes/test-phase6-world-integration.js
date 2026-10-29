'use strict';
const assert=require('node:assert/strict');
const vm=require('node:vm');
const phase6=require('./phase6-world-integration.js');
const art='data:image/png;base64,AAABAA==';
const fixture="const heroArt='"+art+"';let mfRoom=null,bossWarnings=[];"+
 "function mfSetRoom(){mfRoom={name:'Shadow Labyrinth',color:'#fff',speed:1,cadence:1};}"+
 "function reset(){} function beginStage(){mfSetRoom();bossWarnings=[];} function next(){} "+
 "function loop(){pollController();tick();draw();requestAnimationFrame(loop)}";
const script=phase6.patchJs(fixture);
new vm.Script(script);
assert(script.includes(art),'Z/Nyra art values may not be rewritten');
assert.match(script,/mfWorldBeginStage\(\)/);
assert.match(script,/mfWorldOnStageClear\(\)/);
assert.match(script,/mfWorldDecorateRoom\(\)/);
assert.throws(()=>phase6.patchJs('function reset(){}'),/hook missing/);
const html=phase6.patchHtml('<style></style><b id="mfRoomHud">ROOM</b><div id="mfBoonChoices"></div>');
assert.match(html,/id="mfWorldTravel"/);
assert.match(html,/id="mfWorldHud"/);
assert.match(script,/data-mf-region/);

function environment(seed){
 const store=seed||new Map();
 const nodes={};
 function node(id){
  if(!nodes[id])nodes[id]={textContent:'',innerHTML:'',listeners:{},hidden:false,
   classList:{toggle(name,hide){if(name==='hidden')nodes[id].hidden=hide;}},
   addEventListener(event,handler){this.listeners[event]=handler}};
  return nodes[id];
 }
 const env={stage:1,stages:[null,{count:10},{count:12}],document:{getElementById:node},
  localStorage:{getItem(key){return store.get(key)||null;},setItem(key,value){store.set(key,value)}},
  pollController(){},tick(){},draw(){},requestAnimationFrame(){}};
 vm.createContext(env);
 vm.runInContext(script,env);
 return {env,store,node,run:s=>vm.runInContext(s,env)};
}
const {env,store,node,run}=environment();
assert.equal(run('mfWorld.region'),'witchwood');
assert.equal(run("mfWorldChooseGate('foundry')"),false,'cannot travel in combat');
run('reset();beginStage();next()');
assert.match(node('mfWorldTravel').innerHTML,/Mech Foundry/);
assert.match(node('mfWorldTravel').innerHTML,/Cursed Caverns/);
assert.equal(node('mfWorldTravel').hidden,false);
const gate=node('mfWorldTravel').listeners.click;
gate({target:{closest(){return {dataset:{mfRegion:'city'}}}}});
assert.equal(run('mfWorld.region'),'witchwood','must not skip adjacent locked regions');
gate({target:{closest(){return {dataset:{mfRegion:'foundry'}}}}});
assert.equal(run('mfWorld.region'),'foundry');
assert.equal(run("mfWorldChooseGate('city')"),false,'only one gate per clear');
assert.match(node('mfWorldHud').textContent,/Mech Foundry/);
assert.equal(JSON.parse(store.get('moonfall-living-world-v1')).region,'foundry');
env.stage=2;
run('beginStage()');
assert.match(run('mfRoom.name'),/Mech Foundry/);
assert(run('mfRoom.speed')>1,'biome should influence arena challenge');
assert.equal(node('mfWorldTravel').hidden,true);
run('next()');
assert.equal(run("mfWorld.unlocked.includes('city')"),true);
assert.equal(run("mfWorldRoomCache.activeRooms().length"),2);
for(let i=3;i<14;i++){env.stage=i;run('mfWorldBeginStage()');}
assert.equal(run('mfWorldRoomCache.activeRooms().length'),5,'room descriptors must be bounded');
assert(run('mfWorld.flags["cleared:witchwood:stage-1"]'));
const reload=environment(store);
assert.equal(reload.run('mfWorld.region'),'foundry','saved region must restore');
reload.run('reset()');
assert.equal(reload.run('mfWorld.region'),'witchwood','new run begins in Witchwood');
assert.equal(reload.run("mfWorld.unlocked.includes('city')"),true,'progress remains unlocked');
const damaged=environment(new Map([['moonfall-living-world-v1','{bad json']]));
assert.equal(damaged.run('mfWorld.region'),'witchwood','corrupt saves recover');
console.log('PASS: Phase 6 stage integration, gates, biome effects, persistence, room limits and art preservation');
