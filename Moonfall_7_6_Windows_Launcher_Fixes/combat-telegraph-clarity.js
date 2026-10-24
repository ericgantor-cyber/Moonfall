// Combat warning readability; does not change attack timing or character art.
'use strict';
function replaceOne(s,a,b,label){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Telegraph integration hook missing: '+label);return s.slice(0,i)+b+s.slice(i+a.length)}
const logic=String.raw`
function mfClearWarningsDraw(){
 if(!playing)return;
 const reduced=typeof mfReducedEffects!=='undefined'&&mfReducedEffects;
 for(const e of enemies){
  if(e.hp<=0)continue;
  // Elite volleys are triggered on frame % 85 === 0 in mfEliteAttack.
  if(e.mfElite){
   const until=(85-frame%85)%85;
   if(until>24||until===0)continue;
   const progress=1-until/24;
   ctx.save();
   ctx.strokeStyle=until<=9?'#ff5278':'#ffe6a1';
   ctx.lineWidth=reduced?3:2+progress*3;
   ctx.globalAlpha=.72+progress*.28;
   ctx.beginPath();ctx.arc(e.x,e.y,e.r+13+progress*11,0,Math.PI*2);ctx.stroke();
   if(!reduced){
    ctx.beginPath();ctx.arc(e.x,e.y,e.r+26,0,Math.PI*2*progress);ctx.stroke();
   }
   ctx.restore();
  }
  if(e.boss&&e.mfTellUntil>frame){
   const remaining=e.mfTellUntil-frame;
   const progress=Math.max(0,Math.min(1,1-remaining/35));
   ctx.save();
   ctx.strokeStyle=remaining<=12?'#ff557f':'#fff0ad';
   ctx.lineWidth=3;
   ctx.globalAlpha=.78;
   // Secondary progress ring makes the firing countdown readable without flashing.
   ctx.beginPath();
   ctx.arc(e.x,e.y,e.r+36,-Math.PI/2,-Math.PI/2+Math.PI*2*progress);
   ctx.stroke();
   ctx.restore();
  }
 }
}
`;
function patchJs(s){
 s=replaceOne(s,'function draw(){paintBackground();',logic+'\nfunction draw(){paintBackground();','insert warning draw function');
 return replaceOne(s,'mfBossWarningsDraw();','mfBossWarningsDraw();mfClearWarningsDraw();','render combat warnings');
}
module.exports={patchJs};
