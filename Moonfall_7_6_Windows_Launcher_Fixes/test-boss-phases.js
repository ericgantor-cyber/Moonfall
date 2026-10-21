'use strict';
const assert=require('node:assert/strict');
const {createBossPhaseState,updateBossPhase,acknowledgeBossRewards}=require('./boss-phases');
for(const kind of ['dragon','phoenix','screamer','unknown']){
 const boss=createBossPhaseState(kind,100);
 assert.equal(updateBossPhase(boss,90).phase,0);
 const second=updateBossPhase(boss,20);
 assert.equal(second.phase,2);
 assert.equal(second.phaseChanged,true);
 assert.equal(updateBossPhase(boss,80).phase,2,'Healing cannot reverse a phase');
 const win=updateBossPhase(boss,0);
 assert.equal(win.justDefeated,true);
 assert(win.rewards.length>0);
 acknowledgeBossRewards(boss);
 assert.deepEqual(updateBossPhase(boss,0).rewards,[]);
 assert.equal(updateBossPhase(boss,0).justDefeated,false);
}
assert.throws(()=>createBossPhaseState('dragon',0),RangeError);
console.log('PASS: boss phase thresholds, attacks, healing, defeat and rewards');
