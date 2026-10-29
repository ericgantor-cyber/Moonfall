'use strict';
// Optional Phase 5 effects. Does not alter artwork, damage, collision, or hitboxes.
const clamp=(x,min,max)=>Math.max(min,Math.min(max,x));
function blendPose(previous,next,elapsed,blendMs=120){
  const t=clamp(blendMs>0?elapsed/blendMs:1,0,1);
  const smooth=t*t*(3-2*t);
  return {x:previous.x+(next.x-previous.x)*smooth,
    y:previous.y+(next.y-previous.y)*smooth,
    angle:previous.angle+(next.angle-previous.angle)*smooth};
}
function combatPulse(ctx,{x,y,age=0,duration=180,radius=18,color='#f4c7fa',reducedMotion=false}){
  if(!ctx||duration<=0||age<0||age>=duration||reducedMotion)return;
  const t=age/duration;
  ctx.save();
  try {
    ctx.globalAlpha=(1-t)*.6;
    ctx.strokeStyle=color;
    ctx.lineWidth=Math.max(.5,2*(1-t));
    ctx.beginPath();
    ctx.arc(x,y,radius*(.45+.65*t),0,Math.PI*2);
    ctx.stroke();
  } finally {ctx.restore();}
}
function witchwoodSway({timeMs=0,seed=0,wind=.6,reducedMotion=false}={}){
  if(reducedMotion)return 0;
  return Math.sin(timeMs*.0016+seed*2.4)*clamp(wind,0,1)*.045;
}
function witchwoodReaction({distance=0,radius=90,strength=1,reducedMotion=false}={}){
  if(reducedMotion||radius<=0)return 0;
  return clamp(1-distance/radius,0,1)*clamp(strength,0,1);
}
module.exports={blendPose,combatPulse,witchwoodSway,witchwoodReaction};
