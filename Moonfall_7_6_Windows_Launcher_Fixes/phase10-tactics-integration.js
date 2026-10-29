'use strict';
// Integrate Phase 10 into Phase 8/9. No original hero art is touched.
const fs=require('node:fs'),path=require('node:path');
function unique(s,a,b,label){
 const i=s.indexOf(a);
 if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Phase 10 hook mismatch: '+label);
 return s.slice(0,i)+b+s.slice(i+a.length);
}
const core=fs.readFileSync(path.join(__dirname,'phase10-tactics-core.js'),'utf8');
const stripped=core.replace(/module\.exports=\{[^}]+\};\s*$/,'');
if(stripped===core)throw Error('Phase 10 core export changed');
const runtime="\nconst mfP10=(()=>{\n__PHASE10_CORE__\nreturn {bossStage,formationPosition,bossAction,objectiveKey,objectiveRequired,canClaimSeal};\n})();\nfunction mfP10FormationPosition(room,chamber,index,count,radius){\n return mfP10.formationPosition(room,chamber,index,count,radius);\n}\nfunction mfP10BossThink(e){\n if(!e||!e.boss||!playing||e.hp<=0||!player)return;\n const action=mfP10.bossAction(e,frame,stage,shots.length);\n if(!action||action.kind!=='fire')return;\n const aim=Math.atan2(player.y-e.y,player.x-e.x),count=action.count;\n for(let i=0;i<count&&shots.length<240;i++){\n  const angle=action.pattern==='ring'?frame*.005+i*2*Math.PI/count:\n    aim+(i-(count-1)/2)*.22;\n  const speed=action.phase===3?3.7:3.15;\n  shots.push({x:e.x,y:e.y,vx:Math.cos(angle)*speed,vy:Math.sin(angle)*speed,\n   r:7,damage:action.phase===3?14:10,friendly:false,color:'#fba7d7'});\n }\n}\nfunction mfP10BossWarningDraw(){\n if(!playing)return;\n const reduced=typeof mfReducedEffects!=='undefined'&&mfReducedEffects;\n for(const e of enemies){\n  if(!e||!e.boss||!e.mfP10TellUntil||e.mfP10TellUntil<=frame||e.hp<=0)continue;\n  const remaining=e.mfP10TellUntil-frame,progress=1-remaining/45;\n  ctx.save();\n  try{\n   ctx.strokeStyle='#fff0b8';ctx.globalAlpha=.9;\n   ctx.lineWidth=reduced?3:3+progress*2;\n   ctx.beginPath();ctx.arc(e.x,e.y,e.r+42,Math.PI*-.5,Math.PI*(-.5+2*progress));\n   ctx.stroke();\n  }finally{ctx.restore();}\n }\n}\nfunction mfP10SealPosition(){\n if(!mfDungeonLayout)return null;\n return {x:mfDungeonLayout.width*.5,y:mfDungeonLayout.height*.52};\n}\nfunction mfP10ObjectiveRequired(){return mfP10.objectiveRequired(stage)}\nfunction mfP10ObjectiveComplete(){\n return !mfP10ObjectiveRequired()||Boolean(mfWorld.flags[mfP10.objectiveKey(mfWorld.region,stage)]);\n}\nfunction mfP10ObjectiveHud(){\n const hud=document.getElementById('mfP10Hud');\n if(!hud)return;\n let label='⚔ Boss chamber';\n if(mfP10ObjectiveRequired()){\n  if(mfP10ObjectiveComplete())label='🗝 Sanctum seal activated';\n  else if(mfDungeonChamber!==2)label='🗝 Find the Sanctum seal';\n  else if(enemies.some(e=>e&&e.hp>0))label='🗝 Defeat guardians to activate seal';\n  else label='🗝 Approach the Sanctum seal';\n }\n if(hud.textContent!==label)hud.textContent=label;\n}\nfunction mfP10ObjectiveDraw(){\n if(!playing||!mfP10ObjectiveRequired()||mfDungeonChamber!==2||mfP10ObjectiveComplete())return;\n const point=mfP10SealPosition();\n if(!point)return;\n const reduced=typeof mfReducedEffects!=='undefined'&&mfReducedEffects;\n ctx.save();\n try{\n  ctx.strokeStyle='#ffdf93';ctx.fillStyle='#ffdf93';ctx.lineWidth=3;\n  ctx.globalAlpha=reduced?.85:(.7+.17*Math.sin(frame*.06));\n  ctx.beginPath();ctx.arc(point.x,point.y,19,0,Math.PI*2);ctx.stroke();\n  ctx.font='bold 24px system-ui';ctx.textAlign='center';ctx.fillText('✦',point.x,point.y+8);\n }finally{ctx.restore();}\n}\nfunction mfP10ObjectiveStep(){\n if(!playing||paused||skillOpen||gearOpen||!player||!mfDungeonLayout)return;\n if(mfP10.canClaimSeal({\n  stage,chamber:mfDungeonChamber,player,seal:mfP10SealPosition(),\n  enemies,flags:mfWorld.flags,region:mfWorld.region\n })){\n  const key=mfP10.objectiveKey(mfWorld.region,stage);\n  mfWorld.flags[key]=true;\n  souls+=35;\n  mfWorldSave();\n  if(typeof gainXP==='function')gainXP(30);\n  if(typeof burst==='function'&&!(typeof mfReducedEffects!=='undefined'&&mfReducedEffects))\n   burst(player.x,player.y,'#ffdf93',12);\n  if(killCount>=stages[stage].count&&!enemies.some(e=>e&&e.hp>0))next();\n }\n mfP10ObjectiveHud();\n}\nconst mfP10OriginalTick=tick;\ntick=function(){mfP10OriginalTick();mfP10ObjectiveStep();};\n";
function patchJs(s){
 // Modify existing injected Phase 9 spawns and Phase 8 combat before inserting the runtime.
 s=unique(s,'const e={x:W*(.43+.13*(i%2)),y:H*(.36+.15*Math.floor(i/2)),',
  'const position=mfP10FormationPosition(mfDungeonLayout,chamber,i,count,21);\n  const e={x:position.x,y:position.y,',
  'formation positions');
 s=unique(s,' if(!e.boss||e.hp<=0||!playing)return;',
  ' if(!e.boss||e.hp<=0||!playing)return;mfP10BossThink(e);',
  'boss decision');
 s=unique(s,'mfBossWarningsDraw();',
  'mfBossWarningsDraw();mfP10BossWarningDraw();','boss windup drawing');
 s=unique(s,'function next(){','function next(){if(!mfP10ObjectiveComplete())return;','objective stage-clear lock');
 s=unique(s,'function beginStage(){','function beginStage(){mfP10ObjectiveHud();','objective refresh');
 s=unique(s,'function draw(){paintBackground();','function draw(){paintBackground();mfP10ObjectiveDraw();','objective rendering');
 s=unique(s,'function loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
  runtime.replace('__PHASE10_CORE__',stripped)+'\nfunction loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
  'boss and objective runtime');
 return s;
}
function patchHtml(s){
 return unique(s,'<b id="mfDungeonHud"',
  '<b id="mfP10Hud" aria-live="polite">🗝 Find the Sanctum seal</b><b id="mfDungeonHud"','accessible objective HUD');
}
module.exports={patchJs,patchHtml};
