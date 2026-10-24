// Lightweight canvas dialogue bubbles. Never touches original character artwork.
'use strict';
function replaceOnce(s, before, after, label) {
 const i=s.indexOf(before);
 if(i<0 || s.indexOf(before,i+before.length)>=0)throw Error('Speech bubble hook mismatch: '+label);
 return s.slice(0,i)+after+s.slice(i+before.length);
}
function patchJs(s){
 const logic=`
let mfSpeech=null,mfLowHealthSpoken=false,mfBossSpoken=new WeakSet();
const mfHeroLines={z:['The shadows won't stop me.','Bring it on.'],nyra:['The eclipse is mine.','Stay close to the moonlight.']};
const mfBossLines=['You cannot escape the moon!','This realm belongs to me!','Witness my true power!'];
function mfSay(target,text,life=145){
 if(!target||!text||!playing)return;
 mfSpeech={target,text,expires:frame+life};
}
function mfSpeechStep(){
 if(!playing||paused)return;
 if(mfSpeech&&frame>=mfSpeech.expires)mfSpeech=null;
 if(!player||!player.max)return;
 if(player.hp<=player.max*.27&&!mfLowHealthSpoken){
   mfLowHealthSpoken=true;mfSay(player,'I need to be careful!',165);
 }else if(player.hp>player.max*.45)mfLowHealthSpoken=false;
 if(frame%45!==0)return;
 for(const e of enemies){
   if(e.boss&&e.hp>0&&!mfBossSpoken.has(e)){
     mfBossSpoken.add(e);mfSay(e,mfBossLines[stage%mfBossLines.length],165);return;
   }
 }
 if(frame%450===0&&!mfSpeech&&enemies.length){
   const e=enemies.find(e=>e.hp>0&&!e.boss);
   if(e)mfSay(e,'You cannot hide!',100);
 }
}
function mfSpeechDraw(){
 if(!mfSpeech||!playing||frame>=mfSpeech.expires)return;
 const e=mfSpeech.target;
 if(!e||!Number.isFinite(e.x)||!Number.isFinite(e.y)||e.hp<=0&&e!==player)return;
 const message=mfSpeech.text;
 ctx.save();
 ctx.font='bold 15px system-ui';
 const width=Math.min(295,Math.max(115,ctx.measureText(message).width+25));
 const x=Math.max(width/2+8,Math.min(W-width/2-8,e.x));
 const y=Math.max(36,Math.min(H-55,e.y-(e.r||20)-37));
 ctx.fillStyle='rgba(17,9,34,.93)';
 ctx.strokeStyle=e===player?'#dda4ff':'#ffc78c';
 ctx.lineWidth=2;
 ctx.beginPath();ctx.roundRect(x-width/2,y-26,width,39,11);ctx.fill();ctx.stroke();
 ctx.textAlign='center';ctx.textBaseline='middle';ctx.fillStyle='#fff8ff';
 ctx.fillText(message,x,y-7,width-14);
 ctx.restore();
}
`;
 s=replaceOnce(s,'function loop(){pollController();tick();draw();requestAnimationFrame(loop)}',logic+'\nfunction loop(){pollController();tick();draw();requestAnimationFrame(loop)}','runtime declaration');
 s=replaceOnce(s,'function reset(){','function reset(){mfSpeech=null;mfLowHealthSpoken=false;mfBossSpoken=new WeakSet();','reset');
 s=replaceOnce(s,'function tick(){','function tick(){mfSpeechStep();','tick');
 s=replaceOnce(s,'function beginStage(){','function beginStage(){mfSpeech=null;mfBossSpoken=new WeakSet();mfSay(player,mfHeroLines[selectedHero]?.[stage%2]||"The fight begins!",130);','stage speech');
 s=replaceOnce(s,' if(playing&&combatCombo>=2&&frame-comboLastHit<=165){',' mfSpeechDraw();\n if(playing&&combatCombo>=2&&frame-comboLastHit<=165){','speech draw');
 return s;
}
module.exports={patchJs};
