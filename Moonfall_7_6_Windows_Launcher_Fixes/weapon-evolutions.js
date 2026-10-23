// Moonfall elemental weapon evolution boons: fire, ice, and storm.
'use strict';
function once(s,a,b,n){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)>=0)throw Error('Elemental upgrade hook mismatch: '+n);return s.slice(0,i)+b+s.slice(i+a.length)}
const runtime=[
'let mfElements={fire:false,ice:false,storm:false},mfPulseFrame=-1000;',
'function mfRefreshElements(){',
' const el=document.getElementById("mfElementsHud");if(!el)return;',
' const a=Object.entries(mfElements).filter(([,on])=>on).map(([id])=>id.toUpperCase());',
' el.textContent=a.length?"✨ ELEMENTS: "+a.join(" + "):"✨ NO ELEMENTAL BOON";',
'}',
'function mfElementAttack(){',
' if(!playing||paused||skillOpen||gearOpen||frame-mfPulseFrame<25)return;',
' if(!mfElements.fire&&!mfElements.ice&&!mfElements.storm)return;',
' mfPulseFrame=frame;',
' let target=null,best=230*230;',
' for(const e of enemies){if(e.hp<=0)continue;const dx=e.x-player.x,dy=e.y-player.y,d=dx*dx+dy*dy;if(d<best){best=d;target=e;}}',
' if(!target)return;',
' if(mfElements.fire){target.hp-=18;burst(target.x,target.y,"#ff8e43",5);}',
' if(mfElements.ice){target.mfChillUntil=frame+85;burst(target.x,target.y,"#91d8ff",4);}',
' if(mfElements.storm){let struck=0;',
'  for(const e of enemies){if(e.hp<=0||e===target)continue;',
'   const dx=e.x-target.x,dy=e.y-target.y;if(dx*dx+dy*dy>145*145)continue;',
'   e.hp-=12;burst(e.x,e.y,"#b0a5ff",3);if(++struck===3)break;',
'  }',
' }',
'}',
'function mfElementReset(){mfElements={fire:false,ice:false,storm:false};mfPulseFrame=-1000;mfRefreshElements();}'
].join('\n');
function patchJs(s){
 s=once(s,'const mfBoonPool=[',
 'const mfBoonPool=['+
 '{name:"🔥 Ember Weapon",description:"Attacks ignite a nearby enemy",apply(){mfElements.fire=true;mfRefreshElements()}},'+
 '{name:"❄️ Frost Weapon",description:"Attacks chill a nearby enemy",apply(){mfElements.ice=true;mfRefreshElements()}},'+
 '{name:"⚡ Storm Weapon",description:"Attacks arc to nearby foes",apply(){mfElements.storm=true;mfRefreshElements()}},',
 'elemental boons');
 s=once(s,'function mfRollBoons(){',runtime+'\nfunction mfRollBoons(){','elemental runtime');
 s=once(s,'const pool=mfBoonPool.map((_,i)=>i);','const pool=mfBoonPool.map((_,i)=>i).filter(i=>i===0?!mfElements.fire:i===1?!mfElements.ice:i===2?!mfElements.storm:true);','hide already unlocked elemental boons');
 s=once(s,'function shoot(){','function shoot(){mfElementAttack();','attack hook');
 s=once(s,'function reset(){','function reset(){mfElementReset();','reset');
 s=once(s,'mfRoleMove(e,a,speed);','mfRoleMove(e,a,speed*(e.mfChillUntil>frame?.65:1));','ice slowdown');
 return s;
}
function patchHtml(s){
 return once(s,'<b id="mfComboHud"','<b id="mfElementsHud" style="color:#a6deff">✨ NO ELEMENTAL BOON</b><b id="mfComboHud"','element HUD');
}
module.exports={patchJs,patchHtml};
