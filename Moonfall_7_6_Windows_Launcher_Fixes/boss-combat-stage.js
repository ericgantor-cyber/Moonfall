// Original supplementary boss combat phases and telegraphed attacks.
'use strict';
function once(s,a,b,n){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)>=0)throw Error('Boss stage hook mismatch: '+n);return s.slice(0,i)+b+s.slice(i+a.length)}
const logic=[
'function mfBossExtra(e){',
' if(!e.boss||e.hp<=0||!playing||!Number.isFinite(e.max)||e.max<=0)return;',
' const ratio=Math.max(0,e.hp/e.max);',
' const phase=ratio<=.25?3:ratio<=.5?2:1;',
' if(!Number.isFinite(e.mfBossPhase))e.mfBossPhase=1;',
' if(phase>e.mfBossPhase){e.mfBossPhase=phase;burst(e.x,e.y,phase===3?"#ff669c":"#e59aff",24);}',
' const interval=phase===3?105:phase===2?145:190;',
' const tell=35;',
' if(!Number.isFinite(e.mfNextBossAttack))e.mfNextBossAttack=frame+interval;',
' if(e.mfNextBossAttack-frame>interval)e.mfNextBossAttack=frame+interval;',
' if(e.mfNextBossAttack-frame>tell)return;',
' if(e.mfNextBossAttack>frame){e.mfTellUntil=e.mfNextBossAttack;return;}',
' // Keep a telegraph and a bounded next attack even when the projectile budget is full.',
' e.mfNextBossAttack=frame+interval;e.mfTellUntil=0;',
' if(shots.length>=210)return;',
' const a=Math.atan2(player.y-e.y,player.x-e.x);',
' const pattern=(stage+phase)%3,n=phase===3?11:phase===2?9:5;',
' const damage=phase===3?18:phase===2?15:10;',
' for(let i=0;i<n&&shots.length<240;i++){',
'  const angle=pattern===0?i*Math.PI*2/n:a+(i-(n-1)/2)*(pattern===1?.18:.32);',
'  const speed=pattern===1?4:3.1;',
'  shots.push({x:e.x,y:e.y,vx:Math.cos(angle)*speed,vy:Math.sin(angle)*speed,r:7,damage,friendly:false,color:phase===3?"#ff568e":pattern===0?"#dc94ff":"#ff91c0"});',
' }',
'}',
'function mfBossWarningsDraw(){',
' for(const e of enemies){',
'  if(!e.boss||!(e.mfTellUntil>frame))continue;',
'  const remaining=e.mfTellUntil-frame;',
'  ctx.save();ctx.strokeStyle=remaining<13?"#ff638b":"#ffe294";',
'  ctx.lineWidth=3+Math.sin(frame*.5)*1.5;',
'  ctx.beginPath();ctx.arc(e.x,e.y,e.r+15+(35-remaining)*.5,0,Math.PI*2);ctx.stroke();ctx.restore();',
' }',
'}'
].join('\n');
function patchJs(s){
 s=once(s,'function mfEliteAttack(e){',logic+'\nfunction mfEliteAttack(e){','boss code');
 s=once(s,'mfRoleAttack(e);mfEliteAttack(e);','mfRoleAttack(e);mfBossExtra(e);mfEliteAttack(e);','boss update');
 s=once(s,'function draw(){paintBackground();','function draw(){paintBackground();mfBossWarningsDraw();','telegraph renderer');
 return s;
}
module.exports={patchJs};
