'use strict';
// Phase 13 checkpoint capture only. Respawn application is kept separate pending game-over QA.
const fs=require('node:fs'),path=require('node:path');
function one(s,a,b,label){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Phase 13 hook mismatch: '+label);return s.slice(0,i)+b+s.slice(i+a.length)}
const source=fs.readFileSync(path.join(__dirname,'phase13-checkpoint-core.js'),'utf8');
const body=source.replace(/module\.exports=\{[^}]+\};\s*$/,'');
if(body===source)throw Error('Phase 13 core export mismatch');
const runtime=`
const mfCheckpoint=(()=>{
__CORE__
return {checkpoint,matches,respawn};
})();
let mfCheckpointLastFrame=-1;
function mfCheckpointCapture(){
 if(!playing||paused||!player||!mfWorld||frame===mfCheckpointLastFrame||frame%300!==0)return;
 mfCheckpointLastFrame=frame;
 const cp=mfCheckpoint.checkpoint({region:mfWorld.region,stage,player,frame});
 if(!cp)return;
 mfWorld.flags['rpg:checkpoint']=cp;
 mfWorldSave();
}
function mfCheckpointInspect(){
 const cp=mfWorld&&mfWorld.flags['rpg:checkpoint'];
 return mfCheckpoint.matches(cp,mfWorld.region,stage)?cp:null;
}
`;
function patchJs(s){
 s=one(s,'function loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
  runtime.replace('__CORE__',body)+'\nfunction loop(){pollController();tick();draw();requestAnimationFrame(loop)}','runtime');
 s=one(s,'function loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
  'function loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}','checkpoint capture');
 return s;
}
module.exports={patchJs};
