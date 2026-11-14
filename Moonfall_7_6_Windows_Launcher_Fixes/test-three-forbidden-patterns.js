'use strict';
const assert=require('node:assert/strict');
const {createEncounter,encounterTick}=require('./three-forbidden-encounter');
const {planAttack,planWarnings}=require('./three-forbidden-patterns');
const a={x:100,y:200,width:1000,height:700},p={x:450,y:520};
const boss=createEncounter(100);
const events=encounterTick(boss,24,0);
const warnings=events.filter(e=>e.type==='warning');
assert.deepEqual(warnings.map(e=>e.id),['echo-rupture','silencing-curse','shadow-wing-strike']);
const plans=planWarnings(warnings,a,p);
assert.equal(plans.length,4);
assert(plans.every(x=>x.damage===0),'telegraphs never deal damage');
assert.equal(plans.filter(x=>x.shape==='rect').length,2);
assert.equal(plans[1].cx,p.x);
assert.deepEqual(planAttack({type:'attack',id:'silencing-curse',owner:'speak'},a,p).map(x=>x.damage),[18]);
assert.equal(planAttack({type:'attack',id:'unknown'},a,p).length,0);
assert.throws(()=>planAttack({type:'attack',id:'echo-rupture'},{x:0,y:0,width:0,height:1},p),RangeError);
for(let frame=1;frame<=4000;frame++){
 const events=encounterTick(boss,24,frame);
 assert(events.length<=6);
 for(const e of events)if(e.type==='warning'||e.type==='attack'){
  const shapes=planAttack(e,a,p);
  assert(shapes.length>=1);
  for(const shape of shapes)assert(Number.isFinite(shape.damage));
 }
}
console.log('PASS: Three Forbidden attack geometry, warnings, and 4000 tick event stress');
