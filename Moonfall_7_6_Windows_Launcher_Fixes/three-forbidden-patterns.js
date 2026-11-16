'use strict';
// Pure attack geometry: consumes scheduler events. No damage, canvas or engine side effects.
// Positions are arena-relative; the caller translates these into gameplay collision/FX.
const DEFAULT_ARENA=Object.freeze({x:0,y:0,width:1000,height:700});
function finite(v){return typeof v==='number'&&Number.isFinite(v);}
function validateArena(arena){
 if(!arena||!['x','y','width','height'].every(k=>finite(arena[k]))||arena.width<=0||arena.height<=0)throw new RangeError('Invalid arena');
 return arena;
}
function clamp(x,a,b){return Math.max(a,Math.min(b,x));}
function planAttack(event,arena=DEFAULT_ARENA,player={x:500,y:350},options={}){
 validateArena(arena);
 if(!event||!['warning','attack'].includes(event.type))return [];
 if(!player||!finite(player.x)||!finite(player.y))throw new TypeError('Invalid player position');
 const {x,y,width:w,height:h}=arena,cx=x+w/2,cy=y+h/2;
 const px=clamp(player.x,x,x+w),py=clamp(player.y,y,y+h);
 const telegraph=event.type==='warning',common={source:event.id,eventType:event.type,owner:event.owner,fireAt:event.fireAt??null};
 switch(event.id){
  case 'echo-rupture':
   // Expanding ring with a 70 degree safe opening toward arena center.
   return [{...common,shape:'ring-sector',cx,cy,innerRadius:w*.17,outerRadius:w*.34,safeAngle:Math.atan2(py-cy,px-cx)+Math.PI,safeArc:Math.PI*70/180,damage:telegraph?0:14,visual:'silver-wave'}];
  case 'silencing-curse':
   // Cursed disk locks player location at warning time in the live adapter.
   return [{...common,shape:'circle',cx:px,cy:py,radius:Math.min(w,h)*.15,damage:telegraph?0:18,visual:'crimson-sigil'}];
  case 'shadow-wing-strike':
   // Two narrow crossing lanes; corners remain outside both hitboxes.
   return [
    {...common,shape:'rect',x:cx-w*.065,y, width:w*.13,height:h,damage:telegraph?0:20,visual:'wing-lane'},
    {...common,shape:'rect',x,y:cy-h*.07,width:w,height:h*.14,damage:telegraph?0:20,visual:'wing-lane'}
   ];
  default:return [];
 }
}
function planWarnings(events,arena,player){
 return events.flatMap(e=>planAttack(e,arena,player));
}
module.exports={DEFAULT_ARENA,validateArena,planAttack,planWarnings};
