'use strict';
const assert=require('node:assert/strict');
const vm=require('node:vm');
const transform=require('./render-scale.js');
const fixture='function loop(){pollController();tick();draw();requestAnimationFrame(loop)}';
const generated=transform.patchJs(fixture);
new vm.Script(generated);
assert.match(generated,/resolution-scale-v1/);
assert.throws(()=>transform.patchJs(fixture+fixture),/mismatch/);
const html=transform.patchHtml('<button id="mfSpeechFocusBtn" type="button" aria-pressed="false">💬 All dialogue</button>');
assert.match(html,/id="mfScaleBtn"/);
assert.match(html,/not NVIDIA DLSS/);

const callbacks={};
const button={textContent:'',title:'',attrs:{},
 addEventListener(name,fn){callbacks[name]=fn;},
 setAttribute(name,v){this.attrs[name]=v;}
};
const stored=new Map();
const canvas={width:1200,height:800,style:{},
 getBoundingClientRect(){return {width:960,height:640};}};
let transforms=[],draws=0;
const ctx={canvas,setTransform(...args){transforms.push(args);}};
const env={
 ctx,draw(){draws++;},
 document:{getElementById(id){return id==='mfScaleBtn'?button:null;}},
 localStorage:{getItem(key){return stored.get(key)||null;},setItem(key,v){stored.set(key,v);}}
};
const context=vm.createContext(env);
const start=generated.indexOf("const mfScaleModes=");
const stop=generated.indexOf('function loop(){',start);
assert(start>=0&&stop>start);
vm.runInContext(generated.slice(start,stop),context);
const render=()=>vm.runInContext('draw()',context);
const current=()=>vm.runInContext('mfScaleModes[mfScaleIndex].id',context);
assert.equal(current(),'native');
render();
assert.equal(canvas.width,1200);
assert.equal(canvas.height,800);
assert.equal(draws,1);
assert.equal(canvas.style.width,'960px');
assert.equal(canvas.style.height,'640px');
assert.deepEqual([...transforms.at(-1)],[1,0,0,1,0,0]);
callbacks.click(); // Quality 85%
assert.equal(current(),'quality');
render();
assert.equal(canvas.width,1020);
assert.equal(canvas.height,680);
assert.equal(canvas.style.width,'960px','displayed size must stay constant');
assert.equal(canvas.style.height,'640px');
assert.equal(transforms.at(-1)[0],.85);
assert.equal(stored.get('moonfall-resolution-scale-v1'),'quality');
assert.match(button.textContent,/85%/);
callbacks.click(); // Balanced 70%
render();
assert.equal(canvas.width,840);
assert.equal(canvas.height,560);
callbacks.click(); // Performance 55%
render();
assert.equal(canvas.width,660);
assert.equal(canvas.height,440);
callbacks.click(); // Native 100%
render();
assert.equal(canvas.width,1200);
assert.equal(canvas.height,800);
assert.match(button.title,/not NVIDIA DLSS/i);
assert.equal(draws,5);
assert.equal(ctx.imageSmoothingQuality,'high');

// Reload retains the selected mode.
stored.set('moonfall-resolution-scale-v1','performance');
const rerun={...env,draw(){},ctx:{canvas:{width:1200,height:800,style:{},
 getBoundingClientRect(){return {width:960,height:640};}},
 setTransform(){}}};
const other=vm.createContext(rerun);
vm.runInContext(generated.slice(start,stop),other);
assert.equal(vm.runInContext('mfScaleModes[mfScaleIndex].id',other),'performance');
console.log('PASS: render scale presets, canvas resolution, display size, persistence and generated syntax');
