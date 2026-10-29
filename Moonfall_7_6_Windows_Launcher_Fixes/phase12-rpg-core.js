'use strict';
// Small, versioned persistent RPG progression. No artwork or per-frame allocation.
const MAX_LEVEL=100, MAX_XP=100000000;
function cleanRank(value){return Number.isInteger(value)?Math.max(1,Math.min(MAX_LEVEL,value)):1;}
function cleanXP(value){return Number.isInteger(value)?Math.max(0,Math.min(MAX_XP,value)):0;}
function xpRequired(level){return 80+Math.max(0,level-1)*35;}
function restoreProfile(raw){
 const o=raw&&typeof raw==='object'&&!Array.isArray(raw)?raw:{};
 return {version:1,level:cleanRank(o.level),xp:cleanXP(o.xp),
   totalClears:Number.isInteger(o.totalClears)?Math.max(0,Math.min(1000000,o.totalClears)):0};
}
function grantClear(raw,stage){
 const p=restoreProfile(raw);
 if(!Number.isInteger(stage)||stage<1||stage>1000)return p;
 p.totalClears=Math.min(1000000,p.totalClears+1);
 p.xp=Math.min(MAX_XP,p.xp+40+stage*10);
 while(p.level<MAX_LEVEL&&p.xp>=xpRequired(p.level)){
  p.xp-=xpRequired(p.level);p.level++;
 }
 if(p.level===MAX_LEVEL)p.xp=0;
 return p;
}
function damageBonus(profile){return Math.min(12,Math.floor((restoreProfile(profile).level-1)/4));}
module.exports={MAX_LEVEL,restoreProfile,xpRequired,grantClear,damageBonus};
