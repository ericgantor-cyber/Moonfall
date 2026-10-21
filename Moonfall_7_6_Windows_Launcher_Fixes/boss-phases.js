'use strict';
// Deterministic boss-phase controller. No artwork or game state is modified here.
// Integrate with Moonfall's live combat loop after mapping boss IDs and events.
const BOSS_PROFILES=Object.freeze({
 dragon:{thresholds:[.7,.35],attacks:['claw','flame-burst','meteor-storm'],rewards:['dragon-scale','ember-crown']},
 phoenix:{thresholds:[.65,.3],attacks:['firebolt','wing-inferno','rebirth-flare'],rewards:['phoenix-feather','solar-heart']},
 screamer:{thresholds:[.6,.25],attacks:['shriek','echo-pulse','silence-storm'],rewards:['echo-shard','void-voice']},
 default:{thresholds:[.66,.33],attacks:['strike','charged-strike','final-fury'],rewards:['boss-essence']}
});
function createBossPhaseState(kind,maxHP){
 if(!Number.isFinite(maxHP)||maxHP<=0)throw new RangeError('maxHP must be positive');
 return {kind, maxHP, hp:maxHP, phase:0, defeated:false, rewardGranted:false};
}
function updateBossPhase(state,hp){
 if(!state||!Number.isFinite(state.maxHP)||state.maxHP<=0)throw new TypeError('Invalid boss state');
 if(!Number.isFinite(hp))throw new TypeError('hp must be finite');
 const profile=BOSS_PROFILES[state.kind]||BOSS_PROFILES.default;
 const previousPhase=state.phase;
 const nextHP=Math.max(0,Math.min(state.maxHP,hp));
 const nextPhase=profile.thresholds.reduce((n,threshold)=>n+(nextHP/state.maxHP<=threshold?1:0),0);
 // Phase never regresses if a boss heals.
 state.phase=Math.max(previousPhase,nextPhase);
 state.hp=nextHP;
 const justDefeated=nextHP===0&&!state.defeated;
 if(justDefeated)state.defeated=true;
 return {phase:state.phase,phaseChanged:state.phase!==previousPhase,attack:profile.attacks[state.phase],justDefeated,rewards:justDefeated&&!state.rewardGranted?profile.rewards.slice():[]};
}
function acknowledgeBossRewards(state){if(!state.defeated)throw new Error('Boss is not defeated');state.rewardGranted=true;}
module.exports={BOSS_PROFILES,createBossPhaseState,updateBossPhase,acknowledgeBossRewards};
