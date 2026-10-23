// Add hit-chain feedback to the modular Windows build without altering character art.
'use strict';
function once(s, needle, value, label) {
  const pos = s.indexOf(needle);
  if (pos < 0 || s.indexOf(needle, pos + needle.length) >= 0) throw Error('Hades combo hook mismatch: ' + label);
  return s.slice(0, pos) + value + s.slice(pos + needle.length);
}
function patchJs(s) {
  s = once(s, 'function damageEnemy(e,d){', `let combatCombo=0,comboLastHit=-9999,combatShake=0;
function damageEnemy(e,d){`, 'damage function');
  s = once(s, 'function damageEnemy(e,d){if(!playing||!enemies.includes(e)||e.hp<=0)return;', `function damageEnemy(e,d){if(!playing||!enemies.includes(e)||e.hp<=0)return;
 if(frame-comboLastHit>165)combatCombo=0;
 combatCombo=Math.min(99,combatCombo+1);comboLastHit=frame;
 combatShake=Math.min(7,2+combatCombo*.07);
 if(combatCombo>=5)d*=1+Math.min(.15,Math.floor(combatCombo/5)*.025);`, 'valid hit');
  s = once(s, 'function reset(){', 'function reset(){combatCombo=0;comboLastHit=-9999;combatShake=0;', 'new run reset');
  s = once(s, 'function loop(){pollController();tick();draw();requestAnimationFrame(loop)}', `const mfBaseDraw=draw;
draw=function(){
 ctx.save();
 try{
   if(combatShake>0.1 && !(window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches)){
     ctx.translate((Math.random()-.5)*combatShake,(Math.random()-.5)*combatShake);
     combatShake*=.78;
   } else combatShake=0;
   mfBaseDraw();
 }finally{ctx.restore();}
 if(playing&&combatCombo>=2&&frame-comboLastHit<=165){
   ctx.save();ctx.textAlign='center';ctx.font='bold 22px system-ui';
   ctx.shadowColor='#d04cff';ctx.shadowBlur=12;ctx.fillStyle='#fff0ff';
   ctx.fillText(combatCombo+' HIT COMBO',W/2,46);
   if(combatCombo>=5){ctx.font='bold 13px system-ui';ctx.fillText('CHAIN BONUS',W/2,65);}
   ctx.restore();
 }
};
function loop(){pollController();tick();draw();requestAnimationFrame(loop)}`, 'draw wrapper');
  return s;
}
module.exports={patchJs};
