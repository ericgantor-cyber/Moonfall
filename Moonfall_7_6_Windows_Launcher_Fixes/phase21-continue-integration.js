'use strict';
const fs=require('node:fs'),path=require('node:path');
function once(s,a,b,label){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Phase21 hook mismatch: '+label);return s.slice(0,i)+b+s.slice(i+a.length)}
const raw=fs.readFileSync(path.join(__dirname,'phase21-continue-core.js'),'utf8');
const body=raw.replace(/module\.exports=\{[^}]+\};\s*$/,'');
if(body===raw)throw Error('Phase21 export mismatch');
const runtime=`
const mfContinueGuard=(()=>{
__BODY__
return {canContinue};
})();
function mfContinueFromSavedAdventure(){
 const notice=document.getElementById('mfContinueStatus');
 if(!playing||!player||!mfWorld||!mfWorld.flags||mfResumeUsed){
  if(notice)notice.textContent='Start the saved hero and stage first, or position already restored';
  return false;
 }
 const manifest=mfWorld.flags['rpg:continue-manifest'];
 const cp=mfRecoveredCheckpoint();
 if(!mfContinue.validResume(manifest)||
    !mfContinueGuard.canContinue(manifest,cp,selectedHero,mfWorld.region,stage)){
  if(notice)notice.textContent='Saved hero or stage does not match current adventure';
  return false;
 }
 // Reuse Phase 14's one-time guarded resume operation, no duplicate HP path.
 const success=mfResumeCheckpoint();
 if(notice)notice.textContent=success?'Adventure position and HP restored':'Unable to restore current adventure';
 return success;
}
document.getElementById('mfContinueBtn').addEventListener('click',mfContinueFromSavedAdventure);
mfContinueLabel();
`;
function patchJs(s){
 return once(s,'function loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}',
 runtime.replace('__BODY__',body)+'\nfunction loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}','resume runtime');
}
function patchHtml(s){
 return once(s,'<span id="mfContinueStatus"',
 '<button type="button" id="mfContinueBtn" aria-label="Restore saved adventure in current stage">Continue saved adventure</button><span id="mfContinueStatus"','continue button');
}
module.exports={patchJs,patchHtml};
