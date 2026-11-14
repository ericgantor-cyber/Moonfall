'use strict';
// Event-only encounter scheduler; rendering, hitboxes and damage require a game adapter.
const POWERS=Object.freeze([
 {id:'echo-rupture',owner:'hear',cooldown:150,warning:36},
 {id:'silencing-curse',owner:'speak',cooldown:180,warning:42},
 {id:'shadow-wing-strike',owner:'see',cooldown:120,warning:30}
]);
const PHASE_ATTACKS=[['echo-rupture'],['silencing-curse'],['shadow-wing-strike'],POWERS.map(p=>p.id)];
function createEncounter(maxHP,frame=0){
 if(!Number.isFinite(maxHP)||maxHP<=0||!Number.isSafeInteger(frame)||frame<0)throw new RangeError('Invalid maxHP or frame');
 return {maxHP,hp:maxHP,phase:0,lastFrame:frame,active:new Map(),defeated:false};
}
function encounterTick(state,hp,frame,options={}){
 if(!state||!(state.active instanceof Map)||!Number.isFinite(state.maxHP)||state.maxHP<=0)throw new TypeError('Invalid encounter state');
 if(!Number.isFinite(hp)||!Number.isSafeInteger(frame)||frame<state.lastFrame)throw new RangeError('Invalid HP or non-monotonic frame');
 const events=[];state.lastFrame=frame;
 if(state.defeated)return events;
 state.hp=Math.max(0,Math.min(state.maxHP,hp));
 if(state.hp===0){state.defeated=true;state.active.clear();return [{type:'defeat',frame}];}
 const ratio=state.hp/state.maxHP;
 const phase=ratio<=.25?3:ratio<=.5?2:ratio<=.75?1:0;
 if(phase>state.phase){state.phase=phase;state.active.clear();events.push({type:'phase-change',phase:phase+1,frame});}
 const enabled=PHASE_ATTACKS[state.phase];
 for(const power of POWERS){
  if(!enabled.includes(power.id))continue;
  if(!state.active.has(power.id))state.active.set(power.id,{nextWarning:frame,fireAt:null});
  const timer=state.active.get(power.id);
  if(timer.fireAt!==null&&frame>=timer.fireAt){
   if(options.canAttack!==false)events.push({type:'attack',id:power.id,owner:power.owner,frame});
   timer.fireAt=null;timer.nextWarning=frame+power.cooldown;
  }
  if(timer.fireAt===null&&frame>=timer.nextWarning){
   timer.fireAt=frame+power.warning;
   events.push({type:'warning',id:power.id,owner:power.owner,frame,fireAt:timer.fireAt});
  }
 }
 return events;
}
module.exports={POWERS,PHASE_ATTACKS,createEncounter,encounterTick};
