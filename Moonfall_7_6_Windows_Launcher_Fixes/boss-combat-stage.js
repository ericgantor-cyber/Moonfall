// Original supplementary boss combat phases and telegraphed attacks.
'use strict';
function once(s,a,b,n){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)>=0)throw Error('Boss stage hook mismatch: '+n);return s.slice(0,i)+b+s.slice(i+a.length)}
const logic=[
'function mfBossExtra(e){',
' if(!e.boss||e.hp<=0||!playing)return;',
' const second=e.hp<=e.max*.5;',
' if(second&&!e.mfPhaseTwo){e.mfPhaseTwo=true;burst(e.x,e.y,"#e59aff",24);}',
' const interval=second?145:190;',
' const time=frame%interval;',
' if(time===interval-35)e.mfTellUntil=frame+35;',
' if(time!==0||shots.length>=210)return;',
' const a=Math.atan2(player.y-e.y,player.x-e.x);',
' const pattern=stage%3,n=second?9:5;',
' if(pattern===0){',
'  for(let i=0;i<n&&shots.length<240;i++){const angle=i*Math.PI*2/n;',
'   shots.push({x:e.x,y:e.y,vx:Math.cos(angle)*3.1,vy:Math.sin(angle)*3.1,r:7,damage:second?15:10,friendly:false,color:"#dc94ff"});',
'  }',
' }else{',
'  for(let i=0;i<n&&shots.length<240;i++){const angle=a+(i-(n-1)/2)*(pattern===1?.18:.32);',
'   shots.push({x:e.x,y:e.y,vx:Math.cos(angle)*(pattern===1?4:3.1),vy:Math.sin(angle)*(pattern===1?4:3.1),r:7,damage:second?15:10,friendly:false,color:"#ff91c0"});',
'  }',
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
