// Stage-one gameplay build integration. Original hero art is never modified.
'use strict';
function one(s,a,b,n){let i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)>=0)throw Error('Stage 1 hook mismatch: '+n);return s.slice(0,i)+b+s.slice(i+a.length)}
const extra=[
'let mfRouteChoice=null,mfHeroFlash=0;',
'function mfRouteButtons(){',
' if(!upgradePending)return "";if([2,4,5,6,7,8].includes(stage))return "<p>⚔ Boss chamber ahead</p>";',
' return "<div class=\\"mf-route-options\\"><b>Choose the next arena</b>"+mfRoomTypes.map((room,i)=>"<button type=\\"button\\" data-mf-route=\\""+i+"\\""+(mfRouteChoice===i?" class=\\"selected\\"":"")+">"+room.name+"</button>").join("")+"</div>";',
'}',
'function mfRoleMove(e,a,s){',
' const v=e.mobVariant,role=v?.role,d=Math.hypot(player.x-e.x,player.y-e.y);',
' if(role==="flanker")a+=Math.sin(frame*.023+v.id)*.95;',
' if(role==="ranged"){',
'  if(d<170)a+=Math.PI;',
'  else if(d<260)a+=Math.PI/2;',
'  else if(d<320)s*=.65;',
' }',
' if(role==="support"){',
'  if(d<185)a+=Math.PI;',
'  else if(d<280)a+=Math.PI/2;',
'  else s*=.7;',
' }',
' if(role==="tank")s*=.8;',
' if(role==="ambusher")s*=(frame+v.id)%140<25?1.8:.8;',
' e.x+=Math.cos(a)*s;e.y+=Math.sin(a)*s;',
'}',
'function mfRoleAttack(e){',
' const v=e.mobVariant;if(!v||e.boss||e.mfElite||e.hp<=0)return;',
' const n=Math.max(65,Math.round(135/(v.stats.attackRate||1)));',
' if(v.role==="ranged"&&(frame+v.id)%n===0){',
'  const a=Math.atan2(player.y-e.y,player.x-e.x);',
'  if(shots.length<240)shots.push({x:e.x,y:e.y,vx:Math.cos(a)*3.4,vy:Math.sin(a)*3.4,r:6,damage:mfMobDamage(e,7),friendly:false,color:"#dd90ff"});',
' }else if(v.role==="support"&&(frame+v.id)%190===0){',
'  let n=0;for(const f of enemies){if(f===e||f.boss||f.hp>=f.max)continue;',
'    const dx=e.x-f.x,dy=e.y-f.y;if(dx*dx+dy*dy<16000){f.hp=Math.min(f.max,f.hp+10);if(++n===3)break;}}',
' }',
'}'
].join('\n');
function patchJs(s){
 s=one(s,'function mfSetRoom(){',extra+'\nfunction mfSetRoom(){','state');
 s=one(s,'mfRoom=![2,4,5,6,7,8].includes(stage)?mfRoomTypes[(stage+mfMastery)%mfRoomTypes.length]:null;',
 'mfRoom=![2,4,5,6,7,8].includes(stage)?mfRoomTypes[mfRouteChoice===null?(stage+mfMastery)%mfRoomTypes.length:mfRouteChoice]:null;mfRouteChoice=null;','room path');
 s=one(s,'function mfRollBoons(){','function mfRollBoons(){mfRouteChoice=[2,4,5,6,7,8].includes(stage)?0:null;','route reset');
 s=one(s,"root.innerHTML='<p>Choose one blessing before the next battle.</p>",
 "root.innerHTML=mfRouteButtons()+'<p>Choose one blessing before the next battle.</p>",'route buttons');
 s=one(s,'if(!upgradePending||!Number.isInteger(slot)||slot<0||slot>=mfBoonChoices.length)return;',
 'if(!upgradePending||mfRouteChoice===null||!Number.isInteger(slot)||slot<0||slot>=mfBoonChoices.length)return;','choose route');
 s=one(s,"const card=event.target.closest('[data-boon]');",
 "const route=event.target.closest('[data-mf-route]');if(route){mfRouteChoice=Number(route.dataset.mfRoute);mfDrawBoons();return;}const card=event.target.closest('[data-boon]');",'route click');
 s=one(s,'function reset(){','function reset(){mfRouteChoice=null;mfHeroFlash=0;','reset');
 s=one(s,'mfEliteAttack(e);e.x+=Math.cos(a)*speed;',
 'mfRoleAttack(e);mfEliteAttack(e);mfRoleMove(e,a,speed);','role x');
 s=one(s,'e.y+=Math.sin(a)*speed;','','role y');
 s=one(s,'mfCombo=0;mfUpdateCombo()};','mfCombo=0;mfHeroFlash=12;mfUpdateCombo()};','damage pulse');
 s=one(s,'function draw(){paintBackground();',
 'function draw(){paintBackground();if(mfHeroFlash>0){ctx.save();ctx.strokeStyle="rgba(255,55,110,"+(mfHeroFlash/12)+")";ctx.lineWidth=5;ctx.beginPath();ctx.arc(player.x,player.y,player.r+17,0,Math.PI*2);ctx.stroke();ctx.restore();mfHeroFlash--;}','pulse drawing');
 return s;
}
function patchHtml(s){
 const css='.mf-route-options{padding:9px;border:1px solid #a77bc9;border-radius:9px;margin-bottom:12px;display:flex;flex-wrap:wrap;justify-content:center;gap:7px}.mf-route-options b{width:100%}.mf-route-options button{background:#2e1640;color:#fff;border:2px solid #8654ac;border-radius:8px;padding:9px;cursor:pointer}.mf-route-options button.selected,.mf-route-options button:focus-visible{border-color:#ffdf9b;background:#5b3071}';
 return one(s,'</style>',css+'</style>','style');
}
module.exports={patchJs,patchHtml};
