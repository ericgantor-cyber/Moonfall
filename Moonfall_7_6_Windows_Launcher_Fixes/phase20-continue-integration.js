'use strict';
const fs=require('node:fs'),path=require('node:path');
function once(s,a,b,label){
 const at=s.indexOf(a);if(at<0||s.indexOf(a,at+a.length)!==-1)
  throw Error('Phase20 hook missing or ambiguous: '+label);
 return s.slice(0,at)+b+s.slice(at+a.length);
}
const src=fs.readFileSync(path.join(__dirname,'phase20-continue-core.js'),'utf8');
const body=src.replace(/module\.exports=\{[^}]+\};\s*$/,'');
if(body===src)throw Error('Phase20 core export mismatch');
const runtime=`
const mfContinue=(()=>{
__BODY__
return {resumeManifest,validResume};
})();
function mfContinueRemember(){
 if(!mfWorld||!mfWorld.flags||!player||!playing)return;
 const cp=mfRecoveredCheckpoint();
 const m=mfContinue.resumeManifest({
   checkpoint:cp,hero:selectedHero,
   rank:mfRpgProfile().level
 });
 if(!m)return;
 mfWorld.flags['rpg:continue-manifest']=m;
 mfWorldSave();
}
function mfContinueLabel(){
 const label=document.getElementById('mfContinueStatus');
 if(!label||!mfWorld||!mfWorld.flags)return;
 const m=mfWorld.flags['rpg:continue-manifest'];
 label.textContent=mfContinue.validResume(m)?
  'Saved adventure: '+m.hero.toUpperCase()+' • '+m.region+' • stage '+m.stage+' • rank '+m.rank:
  'No valid adventure resume record';
}
`;
function patchJs(s){
 s=once(s,'function loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}',
  runtime.replace('__BODY__',body)+'\nfunction loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}','runtime');
 s=once(s,' mfWorldSave();\n}\nfunction mfCheckpointInspect(){',
  ' mfWorldSave();\n mfContinueRemember();mfContinueLabel();\n}\nfunction mfCheckpointInspect(){','checkpoint metadata');
 return s;
}
function patchHtml(s){
 return once(s,'<button type="button" id="mfDeathBtn"',
  '<span id="mfContinueStatus" role="status" aria-live="polite">No valid adventure resume record</span><button type="button" id="mfDeathBtn"','resume status');
}
module.exports={patchJs,patchHtml};
