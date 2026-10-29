'use strict';
// Safe same-stage checkpoint restoration; never rewinds rewards, enemies or quest flags.
function restorePosition(cp,region,stage,width,height,currentMaxHp){
 if(!cp||cp.version!==1||cp.region!==region||cp.stage!==stage||
  !Number.isFinite(cp.x)||!Number.isFinite(cp.y)||
  cp.x<0||cp.y<0||cp.x>10000||cp.y>10000||
  !Number.isFinite(width)||!Number.isFinite(height)||width<60||height<60)return null;
 const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
 const result={x:clamp(cp.x,30,width-30),y:clamp(cp.y,30,height-30)};
 if(Number.isFinite(cp.hp)&&cp.hp>0&&Number.isFinite(currentMaxHp)&&currentMaxHp>0)
  result.hp=clamp(cp.hp,1,currentMaxHp);
 return result;
}
module.exports={restorePosition};
