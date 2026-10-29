'use strict';
// Pure deterministic formation and boss-state logic. No textures or DOM dependencies.
const BOSS_WINDUP=45;
const BOSS_BULLET_BUDGET=210;
const BOSS_STAGES=Object.freeze([2,4,5,6,7,8]);
function bossStage(stage){return BOSS_STAGES.includes(stage)}
function circleHitsRect(x,y,r,b){const px=Math.max(b.x,Math.min(x,b.x+b.w)),py=Math.max(b.y,Math.min(y,b.y+b.h));return (x-px)**2+(y-py)**2<r*r}
function formationPosition(layout,chamber,index,count,radius=21){
 if(!layout||!Number.isInteger(index)||index<0||!Number.isInteger(count)||count<1||index>=count)throw Error('Invalid formation');
 const W=layout.width,H=layout.height,pillars=layout.pillars||[],hazards=layout.hazards||[];
 const slots=[[.42,.38],[.58,.38],[.5,.61],[.37,.58],[.63,.58],[.5,.3]];
 const start=(chamber*2)%slots.length;
 for(let offset=0;offset<slots.length;offset++){
  const slot=slots[(start+index+offset)%slots.length],x=W*slot[0],y=H*slot[1];
  if([...pillars,...hazards].every(b=>!circleHitsRect(x,y,radius+7,b)))return {x,y};
 }
 // Keep encounter creation bounded and safely away from the central door lane.
 return {x:W*.5,y:H*.5};
}
function bossPhase(boss){
 if(!boss||!boss.boss||!Number.isFinite(boss.hp)||!Number.isFinite(boss.max)||boss.max<=0||boss.hp<=0)return 0;
 const hp=boss.hp/boss.max;
 return hp<=.35?3:hp<=.70?2:1;
}
function bossAction(boss,frame,stage,shotsCount){
 const phase=bossPhase(boss);
 if(phase<2||!Number.isFinite(frame))return null;
 if(boss.mfP10Phase!==phase){
  boss.mfP10Phase=phase;boss.mfP10ReadyAt=frame+90;boss.mfP10TellUntil=0;
 }
 if(boss.mfP10TellUntil>0&&frame>=boss.mfP10TellUntil){
  boss.mfP10TellUntil=0;boss.mfP10ReadyAt=frame+(phase===3?235:290);
  if(shotsCount>=BOSS_BULLET_BUDGET)return null;
  return {kind:'fire',phase,pattern:(stage+phase)%2===0?'ring':'fan',count:phase===3?9:5};
 }
 if(!boss.mfP10TellUntil&&frame>=boss.mfP10ReadyAt){
  boss.mfP10TellUntil=frame+BOSS_WINDUP;
  return {kind:'warn',phase,until:boss.mfP10TellUntil};
 }
 return null;
}
function objectiveKey(region,stage){return 'seal:'+region+':stage-'+stage}
function objectiveRequired(stage){return Number.isInteger(stage)&&stage>=1&&!bossStage(stage)}
function canClaimSeal({stage,chamber,player,seal,enemies,flags,region}){
 if(!objectiveRequired(stage)||chamber!==2||!player||!seal||!flags||flags[objectiveKey(region,stage)])return false;
 if((enemies||[]).some(e=>e&&e.hp>0))return false;
 return Math.hypot(player.x-seal.x,player.y-seal.y)<=43;
}
module.exports={BOSS_WINDUP,BOSS_BULLET_BUDGET,BOSS_STAGES,bossStage,circleHitsRect,formationPosition,bossPhase,bossAction,objectiveKey,objectiveRequired,canClaimSeal};
