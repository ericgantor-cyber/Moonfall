'use strict';
// Bounded death-recovery rules: no inventory/world-flag rollback.
function deathRecovery(cp,region,stage,width,height,maxHp,used){
 if(used||!cp||cp.version!==1||cp.region!==region||cp.stage!==stage||
   !Number.isFinite(cp.x)||!Number.isFinite(cp.y)||
   cp.x<0||cp.x>10000||cp.y<0||cp.y>10000||
   !Number.isFinite(width)||!Number.isFinite(height)||width<60||height<60||
   !Number.isFinite(maxHp)||maxHp<=0)return null;
 const clamp=(x,lo,hi)=>Math.max(lo,Math.min(hi,x));
 return {x:clamp(cp.x,30,width-30),y:clamp(cp.y,30,height-30),
   hp:Math.max(1,Math.floor(Math.min(maxHp,100000)*.35))};
}
module.exports={deathRecovery};
