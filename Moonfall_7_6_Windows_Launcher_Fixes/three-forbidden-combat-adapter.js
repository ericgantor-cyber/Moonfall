'use strict';
// Renderer-neutral combat integration for The Three Forbidden.
// Feed encounterTick events through consumeEvents; use hazards() for warnings;
// resolveHit() for damage decisions. Host game remains responsible for HP,
// parry animation, stamina, drawing and actual entity spawning.
const {planAttack}=require('./three-forbidden-patterns');
function createCombatAdapter(arena){
 return {arena, pending:new Map(), lastFrame:-1, defeated:false};
}
function copyShapes(shapes){
 return shapes.map(s=>({...s}));
}
function consumeEvents(state,events,player,frame){
 if(!state||!(state.pending instanceof Map))throw new TypeError('Invalid combat adapter');
 if(!Number.isSafeInteger(frame)||frame<state.lastFrame)throw new RangeError('Frames must be monotonic');
 state.lastFrame=frame;
 const result=[];
 for(const event of events){
  if(event.type==='defeat'){state.pending.clear();state.defeated=true;result.push({type:'defeat'});continue;}
  if(state.defeated)continue;
  if(event.type==='phase-change'){state.pending.clear();result.push({...event});continue;}
  if(event.type==='warning'){
   const shapes=planAttack(event,state.arena,player);
   state.pending.set(event.id,{fireAt:event.fireAt,shapes});
   result.push({type:'warning',id:event.id,shapes:copyShapes(shapes)});
  }
  if(event.type==='attack'){
   const queued=state.pending.get(event.id);
   if(!queued||frame<queued.fireAt)continue;
   state.pending.delete(event.id);
   result.push({type:'attack',id:event.id,shapes:queued.shapes.map(s=>({...s,eventType:'attack',damage:planDamage(s.source)}))});
  }
 }
 return result;
}
function planDamage(source){return source==='echo-rupture'?14:source==='silencing-curse'?18:source==='shadow-wing-strike'?20:0;}
function pointInShape(s,px,py){
 if(s.shape==='circle')return Math.hypot(px-s.cx,py-s.cy)<=s.radius;
 if(s.shape==='rect')return px>=s.x&&px<=s.x+s.width&&py>=s.y&&py<=s.y+s.height;
 if(s.shape==='ring-sector'){
  const distance=Math.hypot(px-s.cx,py-s.cy);
  if(distance<s.innerRadius||distance>s.outerRadius)return false;
  const angle=Math.atan2(py-s.cy,px-s.cx);
  const diff=Math.atan2(Math.sin(angle-s.safeAngle),Math.cos(angle-s.safeAngle));
  return Math.abs(diff)>s.safeArc/2;
 }
 return false;
}
function resolveHit(attack,player,defense={}){
 if(!attack||attack.type!=='attack'||!Array.isArray(attack.shapes))return {hit:false,damage:0};
 if(!player||!Number.isFinite(player.x)||!Number.isFinite(player.y))throw new TypeError('Invalid player');
 const hit=attack.shapes.some(s=>pointInShape(s,player.x,player.y));
 if(!hit)return {hit:false,damage:0};
 if(defense.invulnerable===true||defense.dodging===true)return {hit:false,damage:0,evaded:true};
 if(defense.parrying===true&&attack.id==='shadow-wing-strike')return {hit:false,damage:0,parried:true};
 return {hit:true,damage:Math.max(...attack.shapes.map(s=>s.damage),0)};
}
module.exports={createCombatAdapter,consumeEvents,pointInShape,resolveHit};
