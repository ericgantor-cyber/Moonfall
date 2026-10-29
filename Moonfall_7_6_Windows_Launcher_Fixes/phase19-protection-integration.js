'use strict';
const fs=require('node:fs'),path=require('node:path');
function one(s,a,b,label){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Phase19 hook missing/ambiguous: '+label);return s.slice(0,i)+b+s.slice(i+a.length)}
const core=fs.readFileSync(path.join(__dirname,'phase19-protection-core.js'),'utf8');
const body=core.replace(/module\.exports=\{[^}]+\};\s*$/,'');
if(body===core)throw Error('Phase19 core export mismatch');
function patchJs(s){
 const before='const receiveDamage=d=>{if(mfEvadeFrames>0)return;if(mfP18TryLethalRecovery(d))return;';
 const after='const receiveDamage=d=>{if(mfEvadeFrames>0||mfP19.recoveryProtected(frame,mfP19ProtectedUntil))return;if(mfP18TryLethalRecovery(d))return;';
 s=one(s,before,after,'damage guard');
 const marker=' mfDeathRecoveryUsed=true;\n player.x=state.x;player.y=state.y;player.hp=state.hp;';
 const changed=' mfDeathRecoveryUsed=true;\n mfP19ProtectedUntil=mfP19.protectionEnd(frame);\n player.x=state.x;player.y=state.y;player.hp=state.hp;';
 // Both manual recovery and lethal interception use this assignment and should receive protection.
 let count=0;
 s=s.replaceAll(marker,()=>{count++;return changed});
 if(count!==2)throw Error('Phase19 expected two recovery sites, found '+count);
 s=one(s,'function reset(){mfDeathRecoveryUsed=false;',
   'function reset(){mfP19ProtectedUntil=0;mfDeathRecoveryUsed=false;','new run reset');
 const runtime=`const mfP19=(()=>{\n__CORE__\nreturn {recoveryProtected,protectionEnd};\n})();\nlet mfP19ProtectedUntil=0;\n`;
 return one(s,'function loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}',
   runtime.replace('__CORE__',body)+'function loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}','runtime');
}
module.exports={patchJs};
