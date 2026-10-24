// Lightweight, original room atmosphere; no texture or gameplay mutations.
'use strict';
function replaceOnce(source, needle, replacement) {
  const i=source.indexOf(needle);
  if(i<0 || source.indexOf(needle,i+needle.length)>=0)throw Error('Atmosphere drawing hook changed');
  return source.slice(0,i)+replacement+source.slice(i+needle.length);
}
const runtime=String.raw`
const mfAtmospherePalettes=[
 ['rgba(143,30,69,.16)','rgba(234,70,112,.28)','#ef8aa8'],
 ['rgba(42,37,108,.19)','rgba(151,120,250,.23)','#bcb1ff'],
 ['rgba(135,64,27,.16)','rgba(255,150,74,.31)','#ffbb79'],
 ['rgba(65,63,123,.12)','rgba(169,177,250,.2)','#bcb1ff']
];
let mfAtmosphereCache=null;
function mfDrawAtmosphere(){
 if(typeof ctx==='undefined'||typeof frame==='undefined'||W<=0||H<=0)return;
 const roomName=typeof mfRoom!=='undefined'&&mfRoom?mfRoom.name:'';
 const variant=roomName.includes('Bloodmoon')?0:roomName.includes('Shadow')?1:roomName.includes('Cinder')?2:3;
 const colors=mfAtmospherePalettes[variant];
 // Rebuild only on a room or canvas-size change, not every animation frame.
 if(!mfAtmosphereCache||mfAtmosphereCache.ctx!==ctx||mfAtmosphereCache.W!==W||mfAtmosphereCache.H!==H||mfAtmosphereCache.variant!==variant){
  const glow=ctx.createRadialGradient(W*.51,H*.48,Math.max(20,W*.03),W*.51,H*.48,Math.max(W,H)*.69);
  glow.addColorStop(0,colors[0]);glow.addColorStop(1,'rgba(5,5,17,0)');
  mfAtmosphereCache={ctx,W,H,variant,glow};
 }
 ctx.save();
 try{
  ctx.fillStyle=mfAtmosphereCache.glow;ctx.fillRect(0,0,W,H);
  ctx.strokeStyle=colors[1];ctx.lineWidth=1.2;
  ctx.beginPath();
  ctx.ellipse(W*.5,H*.55,W*.21,H*.11,0,0,Math.PI*2);
  ctx.ellipse(W*.5,H*.55,W*.27,H*.15,0,0,Math.PI*2);
  ctx.stroke();
  const reduced=(typeof mfReducedEffects!=='undefined'&&mfReducedEffects);
  if(!reduced){
   // Stateless motes, no allocation, no per-frame DOM work or extra sprites.
   for(let i=0;i<16;i++){
    const px=((i*179.3+frame*(.05+i%3*.018))%W+W)%W;
    const py=((i*113.7-frame*(.11+i%4*.023))%H+H)%H;
    ctx.globalAlpha=.18+.13*(1+Math.sin(frame*.019+i))*.5;
    ctx.fillStyle=colors[2];
    ctx.fillRect(px,py,1.5+(i%3)*.55,1.5+(i%3)*.55);
   }
  }
 }finally{ctx.restore()}
}
`;
function patchJs(source){
 return replaceOnce(source,'function draw(){paintBackground();',runtime+'\nfunction draw(){paintBackground();mfDrawAtmosphere();');
}
module.exports={patchJs};
