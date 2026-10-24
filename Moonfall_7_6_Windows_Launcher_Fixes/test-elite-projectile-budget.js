'use strict';
const assert=require('node:assert/strict');
const vm=require('node:vm');
const {patchJs}=require('./roguelike-rooms');
 // Test the actual embedded attack function in isolation without rewriting gameplay.
const transform=require('node:fs').readFileSync(require('node:path').join(__dirname,'roguelike-rooms.js'),'utf8');
const match=transform.match(/function mfEliteAttack\(e\)\{[\s\S]*?\n\}/);
assert(match,'elite attack function exists');
const scope={Math,frame:85,player:{x:50,y:50},shots:Array.from({length:238},()=>({})),mfRoom:{color:'#fff'},burst(){}};
vm.runInNewContext('('+match[0]+')(e)',{...scope,e});
assert.equal(scope.shots.length,240,'elite volley must respect shot limit');
const full={...scope,shots:Array.from({length:240},()=>({}))};
vm.runInNewContext('('+match[0]+')(e)',{...full,e});
assert.equal(full.shots.length,240,'full shot buffer must not grow');
console.log('PASS: elite shots remain within shared projectile budget');
