'use strict';
const assert=require('node:assert/strict');
const vm=require('node:vm');
const perf=require('./performance-overlay.js');
const loop='function loop(){pollController();tick();draw();requestAnimationFrame(loop)}';
const compiled=perf.patchJs(loop);
new vm.Script(compiled);
assert.throws(()=>perf.patchJs(loop+loop),/mismatch/);
const html=perf.patchHtml('<button id="mfScaleBtn" type="button" title="Resolution scaling, not NVIDIA DLSS">🖥 Render: Native 100%</button>');
assert.match(html,/id="mfPerfBtn"/);
assert.match(html,/aria-live="off"/);
const elements={};
for(const id of ['mfPerfBtn','mfPerfHud']){
 elements[id]={textContent:'',style:{},attributes:{},
  addEventListener(type,cb){this[type]=cb;},
  setAttribute(key,value){this.attributes[key]=value}};
}
const bindings={},store=new Map(),state={now:0};
const env={
 document:{getElementById(id){return elements[id];}},
 window:{addEventListener(type,cb){bindings[type]=cb;}},
 performance:{now(){return state.now;}},
 localStorage:{getItem(key){return store.get(key)||null;},setItem(key,val){store.set(key,val);}},
 playing:true,paused:false,pollController(){},tick(){},draw(){},requestAnimationFrame(){}
};
vm.createContext(env);
const start=compiled.indexOf("const mfPerfKey=");
const end=compiled.indexOf('function loop(){mfPerfFrame()',start);
assert(start>0&&end>start);
vm.runInContext(compiled.slice(start,end),env);
const run=s=>vm.runInContext(s,env);
assert.equal(elements.mfPerfHud.style.display,'none');
assert.equal(elements.mfPerfBtn.attributes['aria-pressed'],'false');
elements.mfPerfBtn.click();
assert.equal(store.get('moonfall-performance-overlay-v1'),'true');
assert.equal(elements.mfPerfHud.style.display,'block');
state.now=100;run('mfPerfFrame()');
for(let t=116;t<=1132;t+=16){state.now=t;run('mfPerfFrame()')}
assert.match(elements.mfPerfHud.textContent,/FPS \d+ \| worst \d+ ms \| >50 ms \d+/);
env.paused=true;state.now=1150;run('mfPerfFrame()');
assert.equal(elements.mfPerfHud.textContent,'FPS: paused');
env.paused=false;env.playing=false;run('mfPerfFrame()');
assert.equal(elements.mfPerfHud.textContent,'FPS: idle');
const evt={code:'F6',repeat:false,ctrlKey:false,altKey:false,metaKey:false,preventDefault(){this.handled=true}};
bindings.keydown(evt);
assert(evt.handled,'F6 must prevent browser default');
assert.equal(elements.mfPerfHud.style.display,'none');
assert.equal(store.get('moonfall-performance-overlay-v1'),'false');
console.log('PASS: FPS overlay, frame pacing, accessibility, persistence, pause/idle and F6 shortcut');
