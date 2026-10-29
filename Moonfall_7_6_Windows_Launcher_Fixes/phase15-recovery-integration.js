'use strict';
const fs=require('node:fs'),path=require('node:path');
function one(s,a,b,label){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Phase15 hook mismatch: '+label);return s.slice(0,i)+b+s.slice(i+a.length)}
const src=fs.readFileSync(path.join(__dirname,'phase15-recovery-core.js'),'utf8');
const body=src.replace(/module\.exports=\{[^}]+\};\s*$/,'');
if(body===src)throw Error('Phase15 core export mismatch');
const runtime=`
const mfSaveRecovery=(()=>{
__CORE__
return {valid,select,savePair};
})();
function mfRecoveredCheckpoint(){
 if(!mfWorld||!mfWorld.flags)return null;
 const result=mfSaveRecovery.select(mfWorld.flags['rpg:checkpoint'],
   mfWorld.flags['rpg:checkpoint-backup'],mfWorld.region,stage);
 const status=document.getElementById('mfResumeStatus');
 if(status&&result.source==='backup')status.textContent='Recovered backup checkpoint';
 return result.checkpoint;
}
`;
function patchJs(s){
 s=one(s,'function loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}',
 runtime.replace('__CORE__',body)+'\nfunction loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}','recovery runtime');
 s=one(s," mfWorld.flags['rpg:checkpoint']=cp;\n mfWorldSave();",
 " const pair=mfSaveRecovery.savePair(mfWorld.flags['rpg:checkpoint'],mfWorld.flags['rpg:checkpoint-backup'],cp);\n mfWorld.flags['rpg:checkpoint']=pair.primary;\n mfWorld.flags['rpg:checkpoint-backup']=pair.backup;\n mfWorldSave();",'checkpoint backup');
 s=one(s," const cp=mfWorld.flags['rpg:checkpoint'];\n const state=mfResume.restorePosition(",
 " const cp=mfRecoveredCheckpoint();\n const state=mfResume.restorePosition(",'resume fallback');
 return s;
}
module.exports={patchJs};
