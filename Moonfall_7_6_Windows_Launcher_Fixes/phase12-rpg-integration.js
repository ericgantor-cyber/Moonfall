'use strict';
// Add persistent hero ranks to the existing connected world and stage-clear loop.
const fs=require('node:fs'),path=require('node:path');
function replaceOnce(s,a,b,label){
 const i=s.indexOf(a);
 if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('RPG hook missing or ambiguous: '+label);
 return s.slice(0,i)+b+s.slice(i+a.length);
}
const core=fs.readFileSync(path.join(__dirname,'phase12-rpg-core.js'),'utf8');
const body=core.replace(/module\.exports=\{[^}]+\};\s*$/,'');
if(body===core)throw Error('RPG core export mismatch');
const runtime=`
const mfRpg=(()=>{
__RPG_CORE__
return {restoreProfile,xpRequired,grantClear,damageBonus};
})();
let mfRpgBonusApplied=false;
function mfRpgProfile(){
 return mfRpg.restoreProfile(mfWorld.flags['rpg:hero']);
}
function mfRpgRefresh(){
 const label=document.getElementById('mfRpgHud');
 if(!label)return;
 const p=mfRpgProfile();
 label.textContent='⚔ RPG Rank '+p.level+' • '+p.xp+'/'+mfRpg.xpRequired(p.level)+' XP';
}
function mfRpgStageStart(){
 if(!mfRpgBonusApplied){
  damageLevel+=mfRpg.damageBonus(mfRpgProfile());
  mfRpgBonusApplied=true;
 }
 mfRpgRefresh();
}
function mfRpgStageClear(){
 // Only called after the existing stage-clear function, which already guards repeated clears.
 const key='rpg:reward:'+mfWorld.region+':stage-'+stage;
 if(!mfWorld.flags[key]){
  mfWorld.flags[key]=true;
  mfWorld.flags['rpg:hero']=mfRpg.grantClear(mfWorld.flags['rpg:hero'],stage);
  mfWorldSave();
 }
 mfRpgRefresh();
}
`;
function patchJs(s){
 s=replaceOnce(s,'function loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
  runtime.replace('__RPG_CORE__',body)+'\nfunction loop(){pollController();tick();draw();requestAnimationFrame(loop)}','runtime');
 s=replaceOnce(s,'function reset(){','function reset(){mfRpgBonusApplied=false;','run reset');
 s=replaceOnce(s,'function beginStage(){','function beginStage(){mfRpgStageStart();','stage begin');
 s=replaceOnce(s,'mfWorldOnStageClear();mfP11AfterClear();',
   'mfWorldOnStageClear();mfP11AfterClear();mfRpgStageClear();','stage completion');
 return s;
}
function patchHtml(s){
 return replaceOnce(s,'<b id="mfP11Hud"',
 '<b id="mfRpgHud" aria-live="polite">⚔ RPG Rank 1</b><b id="mfP11Hud"','rank HUD');
}
module.exports={patchJs,patchHtml};
