'use strict';
// Phase 11 attaches to existing Phase 10 / Phase 9 gameplay, preserving hero textures.
const fs=require('node:fs'),path=require('node:path');
function unique(s,a,b,label){
 const i=s.indexOf(a);
 if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Phase 11 integration hook mismatch: '+label);
 return s.slice(0,i)+b+s.slice(i+a.length);
}
const core=fs.readFileSync(path.join(__dirname,'phase11-regional-core.js'),'utf8');
const pure=core.replace(/module\.exports=\{[^}]+\};\s*$/,'');
if(pure===core)throw Error('Phase 11 export mismatch');
const runtime="\nconst mfP11=(()=>{\n__PHASE11_CORE__\nreturn {WINDUP,regionBoss,regionBossAction,regionalVolley,runeKey,championKey,canClaimRune,claimRegion,masteredRegions};\n})();\nlet mfP11BonusApplied=false,mfP11LastChampion='';\nfunction mfP11RunePoint(){\n return {x:W*.5,y:H*.52};\n}\nfunction mfP11HasRune(){\n return mfP11.regionBoss(mfWorld.region) &&\n  Boolean(mfWorld.flags[mfP11.runeKey(mfWorld.region,stage)]);\n}\nfunction mfP11GrantMastery(){\n if(mfP11BonusApplied)return;\n mfP11BonusApplied=true;\n const tiers=Math.min(3,mfP11.masteredRegions(mfWorld.flags).length);\n if(tiers>0)damageLevel+=tiers;\n}\nfunction mfP11AfterClear(){\n if(stage!==2||!mfWorld)return;\n if(!mfP11.claimRegion(mfWorld.flags,mfWorld.region))return;\n mfP11LastChampion=mfWorld.region;\n souls+=180;mfWorldSave();\n mfP11RefreshHud();\n}\nfunction mfP11RefreshHud(){\n const element=document.getElementById('mfP11Hud');\n if(!element||!mfWorld)return;\n const profile=mfP11.regionBoss(mfWorld.region);\n let msg='';\n if(mfP10ObjectiveRequired()){\n  if(mfP11HasRune())msg='🜁 Rune collected • reach the Sanctum';\n  else if(mfDungeonChamber===1)msg=enemies.some(e=>e&&e.hp>0)?\n    '🜁 Defeat Crossroads guardians':'🜁 Touch the glowing Rune';\n  else msg='🜁 Find the Crossroads Rune';\n }else msg=profile?'👑 '+profile.name:'👑 Boss';\n if(mfWorld.flags[mfP11.championKey(mfWorld.region)])msg+=' • REGION MASTERED';\n if(element.textContent!==msg)element.textContent=msg;\n}\nfunction mfP11RuneDraw(){\n if(!playing||!mfWorld||!mfP10ObjectiveRequired()||mfDungeonChamber!==1||mfP11HasRune())return;\n const point=mfP11RunePoint(),small=typeof mfReducedEffects!=='undefined'&&mfReducedEffects;\n ctx.save();\n try{\n  ctx.globalAlpha=small?.8:.72+.16*Math.sin(frame*.04);\n  ctx.strokeStyle='#a8f5ff';ctx.fillStyle='#a8f5ff';ctx.lineWidth=3;\n  ctx.beginPath();ctx.arc(point.x,point.y,19,0,Math.PI*2);ctx.stroke();\n  ctx.textAlign='center';ctx.font='bold 22px system-ui';ctx.fillText('✧',point.x,point.y+7);\n }finally{ctx.restore();}\n}\nfunction mfP11RuneStep(){\n if(!mfWorld)return;\n if(playing&&!paused&&!skillOpen&&!gearOpen&&player&&mfDungeonLayout&&mfP11.canClaimRune({\n  region:mfWorld.region,stage,chamber:mfDungeonChamber,player,point:mfP11RunePoint(),\n  enemies,flags:mfWorld.flags\n })){\n  mfWorld.flags[mfP11.runeKey(mfWorld.region,stage)]=true;\n  souls+=10;mfWorldSave();\n  if(typeof gainXP==='function')gainXP(15);\n  if(typeof burst==='function'&&!(typeof mfReducedEffects!=='undefined'&&mfReducedEffects))\n   burst(player.x,player.y,'#a8f5ff',10);\n }\n mfP11RefreshHud();\n}\nfunction mfP11BossThink(e){\n const action=mfP11.regionBossAction(e,mfWorld.region,frame,shots.length);\n if(!action||action.kind!=='fire')return;\n const aim=Math.atan2(player.y-e.y,player.x-e.x);\n for(const shot of mfP11.regionalVolley(action.profile,aim,frame*.018)){\n  if(shots.length>=240)break;\n  shots.push({x:e.x,y:e.y,\n   vx:Math.cos(shot.angle)*shot.speed,vy:Math.sin(shot.angle)*shot.speed,\n   r:7,damage:action.phase===2?15:11,friendly:false,color:action.profile.color});\n }\n}\nfunction mfP11WarningDraw(){\n if(!playing||!mfWorld)return;\n const profile=mfP11.regionBoss(mfWorld.region);\n if(!profile)return;\n const reduced=typeof mfReducedEffects!=='undefined'&&mfReducedEffects;\n for(const e of enemies){\n  if(!e||!e.boss||!e.mfP11Warning||frame>=e.mfP11Warning||e.hp<=0)continue;\n  const progress=1-(e.mfP11Warning-frame)/mfP11.WINDUP;\n  ctx.save();\n  try{\n   ctx.globalAlpha=.85;ctx.strokeStyle=profile.color;ctx.lineWidth=reduced?3:3+progress*2;\n   ctx.beginPath();ctx.arc(e.x,e.y,e.r+59,-Math.PI/2,-Math.PI/2+2*Math.PI*progress);\n   ctx.stroke();\n  }finally{ctx.restore();}\n }\n}\n";
function patchJs(s){
 s=unique(s,' const action=mfP10.bossAction(e,frame,stage,shots.length);',
  ' if(typeof mfP11BossThink===\"function\")mfP11BossThink(e);\n const action=mfP10.bossAction(e,frame,stage,shots.length);',
  'regional boss decision');
 s=unique(s,'function mfP10BossWarningDraw(){',
  'function mfP10BossWarningDraw(){mfP11WarningDraw();','regional boss warning');
 s=unique(s,'function mfP10ObjectiveDraw(){',
  'function mfP10ObjectiveDraw(){mfP11RuneDraw();','rune visual');
 s=unique(s,'function mfP10ObjectiveStep(){',
  'function mfP10ObjectiveStep(){mfP11RuneStep();','rune pickup');
 s=unique(s,'if(mfP10.canClaimSeal({',
  'if(mfP11HasRune()&&mfP10.canClaimSeal({','require crossroads rune before sanctum seal');
 s=unique(s,'mfWorldOnStageClear();','mfWorldOnStageClear();mfP11AfterClear();','region boss completion and rewards');
 s=unique(s,'function reset(){','function reset(){mfP11BonusApplied=false;mfP11LastChampion=\"\";','reset permanent rewards application');
 s=unique(s,'function beginStage(){','function beginStage(){mfP11GrantMastery();','apply region mastery at start of run');
 s=unique(s,'function loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
  runtime.replace('__PHASE11_CORE__',pure)+'\nfunction loop(){pollController();tick();draw();requestAnimationFrame(loop)}','regional runtime');
 return s;
}
function patchHtml(s){
 return unique(s,'<b id="mfP10Hud"',
  '<b id="mfP11Hud" aria-live="polite">🜁 Find the Crossroads Rune</b><b id="mfP10Hud"','rune HUD');
}
module.exports={patchJs,patchHtml};
