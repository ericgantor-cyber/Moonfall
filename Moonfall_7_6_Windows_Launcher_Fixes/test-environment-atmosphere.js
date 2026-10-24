'use strict';
const assert=require('node:assert/strict');
const vm=require('node:vm');
const {patchJs}=require('./environment-atmosphere');
const source='function draw(){paintBackground();ctx.fillRect(0,0,10,10)}';
const result=patchJs(source);
assert(result.includes('mfDrawAtmosphere();'));
assert(result.includes("roomName.includes('Bloodmoon')"));
assert(result.includes("roomName.includes('Shadow')"));
assert(result.includes("roomName.includes('Cinder')"));
assert(result.includes('ctx.restore()'));
new vm.Script(result);
assert.throws(()=>patchJs('function unrelated(){}'),/drawing hook/);

let gradients=0;
const ctx={
 save(){},restore(){},beginPath(){},stroke(){},ellipse(){},fillRect(){},
 createRadialGradient(){gradients++;return {addColorStop(){}}}
};
const scope={ctx,W:960,H:540,frame:1,mfRoom:{name:'Bloodmoon Arena'},playing:true,paintBackground(){}};
vm.createContext(scope);
vm.runInContext(result,scope);
scope.draw();scope.frame++;scope.draw();
assert.equal(gradients,1,'unchanged room must reuse its atmospheric gradient');
scope.mfRoom={name:'Cinder Gauntlet'};scope.draw();
assert.equal(gradients,2,'new room palette should rebuild gradient once');
scope.W=1024;scope.draw();
assert.equal(gradients,3,'resized arena should rebuild gradient once');

console.log('PASS: room atmosphere integrates and parses without altering source art');
