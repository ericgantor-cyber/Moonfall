'use strict';
// Opt-in same-stage Continue action, preserving existing game-over and new-run behavior.
const fs=require('node:fs'),path=require('node:path');
function once(s,a,b,label){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Phase14 hook missing or ambiguous: '+label);return s.slice(0,i)+b+s.slice(i+a.length)}
const src=fs.readFileSync(path.join(__dirname,'phase14-resume-core.js'),'utf8');
const body=src.replace(/module\.exports=\{[^}]+\};\s*$/,'');
if(body===src)throw Error('Phase14 export changed');
const runtime=`
const mfResume=(()=>{
__CORE__
return {restorePosition};
})();
let mfResumeUsed=false;
function mfResumeCheckpoint(){
 if(mfResumeUsed||!playing||!player||!mfWorld||!mfWorld.flags)return false;
 const cp=mfWorld.flags['rpg:checkpoint'];
 const state=mfResume.restorePosition(cp,mfWorld.region,stage,W,H,player.maxHp);
 if(!state)return false;
 // Once per run prevents loading the same HP repeatedly during combat.
 mfResumeUsed=true;
 player.x=state.x;player.y=state.y;
 if(Number.isFinite(state.hp))player.hp=state.hp;
 const label=document.getElementById('mfResumeStatus');
 if(label)label.textContent='Saved character position restored';
 return true;
}
document.getElementById('mfResumeBtn').addEventListener('click',()=>{
 const label=document.getElementById('mfResumeStatus');
 if(!mfResumeCheckpoint()&&label)label.textContent='No matching checkpoint in this stage, or already resumed';
});
`;
function patchJs(s){
 s=once(s,'function loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}',
  runtime.replace('__CORE__',body)+'\nfunction loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}','resume runtime');
 s=once(s,'function reset(){','function reset(){mfResumeUsed=false;','new run reset');
 return s;
}
function patchHtml(s){
 return once(s,'<b id="mfRpgHud"',
 '<button type="button" id="mfResumeBtn" aria-label="Restore saved position in current stage">Resume saved position</button><span id="mfResumeStatus" role="status" aria-live="polite"></span><b id="mfRpgHud"','resume button');
}
module.exports={patchJs,patchHtml};
