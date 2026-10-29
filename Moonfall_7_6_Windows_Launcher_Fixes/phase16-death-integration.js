'use strict';
// Opt-in recovery during active game when HP is depleted; do not alter game-over functions.
const fs=require('node:fs'),path=require('node:path');
function one(s,a,b,label){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Phase16 hook mismatch: '+label);return s.slice(0,i)+b+s.slice(i+a.length)}
const source=fs.readFileSync(path.join(__dirname,'phase16-death-core.js'),'utf8');
const body=source.replace(/module\.exports=\{[^}]+\};\s*$/,'');
if(body===source)throw Error('Phase16 core export mismatch');
const runtime=`
const mfDeathRecovery=(()=>{
__CORE__
return {deathRecovery};
})();
let mfDeathRecoveryUsed=false;
function mfTryDeathRecovery(){
 const message=document.getElementById('mfDeathStatus');
 if(mfDeathRecoveryUsed||!player||!mfWorld||!mfWorld.flags||
    !Number.isFinite(player.hp)||player.hp>0){
  if(message)message.textContent='Recovery only available once when health is depleted';
  return false;
 }
 const cp=mfRecoveredCheckpoint();
 const result=mfDeathRecovery.deathRecovery(cp,mfWorld.region,stage,W,H,
   player.maxHp,mfDeathRecoveryUsed);
 if(!result){
  if(message)message.textContent='No valid checkpoint for this region and stage';
  return false;
 }
 mfDeathRecoveryUsed=true;
 player.x=result.x;player.y=result.y;player.hp=result.hp;
 if(message)message.textContent='Returned to checkpoint with 35% health';
 return true;
}
document.getElementById('mfDeathBtn').addEventListener('click',mfTryDeathRecovery);
`;
function patchJs(s){
 s=one(s,'function loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}',
  runtime.replace('__CORE__',body)+'\nfunction loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}','death recovery runtime');
 s=one(s,'function reset(){','function reset(){mfDeathRecoveryUsed=false;','reset per run');
 return s;
}
function patchHtml(s){
 return one(s,'<button type="button" id="mfResumeBtn"',
 '<button type="button" id="mfDeathBtn" aria-label="Return to checkpoint when health is depleted">Return after defeat</button><span id="mfDeathStatus" role="status" aria-live="polite"></span><button type="button" id="mfResumeBtn"','accessible return control');
}
module.exports={patchJs,patchHtml};
