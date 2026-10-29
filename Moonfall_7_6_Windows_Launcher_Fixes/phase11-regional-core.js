'use strict';
// Deterministic regional mechanics. No DOM, textures, sprites, or per-frame allocations.
const REGIONAL_BOSSES=Object.freeze({
 witchwood:Object.freeze({name:'Briar Sovereign',attack:'thorns',color:'#93e7ae',count:7}),
 foundry:Object.freeze({name:'Furnace Warden',attack:'crossfire',color:'#ffb17a',count:8}),
 city:Object.freeze({name:'Grave Regent',attack:'spiral',color:'#cda4ff',count:7}),
 caverns:Object.freeze({name:'Echo Maw',attack:'echo',color:'#88dafa',count:6}),
 bosslands:Object.freeze({name:'Eclipse Tyrant',attack:'eclipse',color:'#ff719e',count:9})
});
const WINDUP=55,MAX_PROJECTILES=210;
function regionBoss(region){return Object.hasOwn(REGIONAL_BOSSES,region)?REGIONAL_BOSSES[region]:null}
function regionBossAction(boss,region,frame,shotCount){
 const profile=regionBoss(region);
 if(!profile||!boss||!boss.boss||boss.hp<=0||!Number.isFinite(frame))return null;
 if(boss.mfP11Region!==region){
  boss.mfP11Region=region;boss.mfP11Next=frame+160;boss.mfP11Warning=0;
 }
 if(boss.mfP11Warning&&frame>=boss.mfP11Warning){
  boss.mfP11Warning=0;
  const enraged=boss.max>0&&boss.hp/boss.max<=.35;
  boss.mfP11Next=frame+(enraged?210:285);
  if(shotCount>=MAX_PROJECTILES)return null;
  return {kind:'fire',profile,phase:enraged?2:1};
 }
 if(!boss.mfP11Warning&&frame>=boss.mfP11Next){
  boss.mfP11Warning=frame+WINDUP;
  return {kind:'warn',profile,until:boss.mfP11Warning};
 }
 return null;
}
function regionalVolley(profile,aim,spin=0){
 if(!profile)return [];
 const out=[];
 for(let i=0;i<profile.count;i++){
  let angle;
  if(profile.attack==='thorns')angle=aim+(i-(profile.count-1)/2)*.19;
  else if(profile.attack==='crossfire')angle=Math.floor(i/2)*Math.PI/2+(i%2?-.11:.11);
  else if(profile.attack==='spiral')angle=spin+i*2*Math.PI/profile.count;
  else if(profile.attack==='echo')angle=aim+(i%2?-.34:.34)+(Math.floor(i/2)-1)*.085;
  else angle=aim+i*2*Math.PI/profile.count;
  out.push({angle,speed:profile.attack==='thorns'?4.1:profile.attack==='echo'?3.4:3.7});
 }
 return out;
}
function runeKey(region,stage){return 'rune:'+region+':stage-'+stage}
function championKey(region){return 'champion:'+region}
function canClaimRune({region,stage,chamber,player,point,enemies,flags}){
 if(!regionBoss(region)||!Number.isInteger(stage)||[2,4,5,6,7,8].includes(stage)||stage<1||
 chamber!==1||!player||!point||!flags||flags[runeKey(region,stage)])return false;
 if((enemies||[]).some(e=>e&&e.hp>0))return false;
 return Math.hypot(player.x-point.x,player.y-point.y)<=42;
}
function regionComplete(flags,region){
 return Boolean(regionBoss(region)&&flags&&flags['seal:'+region+':stage-1']&&flags['cleared:'+region+':stage-2']);
}
function claimRegion(flags,region){
 if(!regionComplete(flags,region)||flags[championKey(region)])return false;
 flags[championKey(region)]=true;return true;
}
function masteredRegions(flags){
 return Object.keys(REGIONAL_BOSSES).filter(region=>flags&&flags[championKey(region)]);
}
module.exports={REGIONAL_BOSSES,WINDUP,MAX_PROJECTILES,regionBoss,regionBossAction,regionalVolley,runeKey,championKey,canClaimRune,regionComplete,claimRegion,masteredRegions};
