// Optional low-overhead frame pacing overlay. Does not modify gameplay or character art.
'use strict';
function mfPerfOnce(source, find, replacement, label) {
 const pos=source.indexOf(find);
 if(pos<0||source.indexOf(find,pos+find.length)>=0)throw Error('Performance integration mismatch: '+label);
 return source.slice(0,pos)+replacement+source.slice(pos+find.length);
}
function mfPerfRuntime() {
 const mfPerfKey='moonfall-performance-overlay-v1';
 const mfPerfButton=document.getElementById('mfPerfBtn');
 const mfPerfHud=document.getElementById('mfPerfHud');
 let mfPerfEnabled=false, mfPerfPrevious=0, mfPerfStart=0, mfPerfFrames=0, mfPerfWorst=0, mfPerfSlow=0;
 try{mfPerfEnabled=localStorage.getItem(mfPerfKey)==='true'}catch(_){}
 function mfPerfRefresh(){
  mfPerfButton.textContent=mfPerfEnabled?'📊 FPS: ON':'📊 FPS: OFF';
  mfPerfButton.setAttribute('aria-pressed',String(mfPerfEnabled));
  mfPerfHud.style.display=mfPerfEnabled?'block':'none';
 }
 function mfPerfToggle(){
  mfPerfEnabled=!mfPerfEnabled;
  mfPerfPrevious=0;mfPerfStart=0;mfPerfFrames=0;mfPerfWorst=0;mfPerfSlow=0;
  mfPerfHud.textContent='FPS: measuring…';
  try{localStorage.setItem(mfPerfKey,String(mfPerfEnabled))}catch(_){}
  mfPerfRefresh();
 }
 mfPerfButton.addEventListener('click',mfPerfToggle);
 window.addEventListener('keydown',function(event){
  if(event.code!=='F6'||event.repeat||event.ctrlKey||event.altKey||event.metaKey)return;
  event.preventDefault();
  mfPerfToggle();
 });
 mfPerfRefresh();
 function mfPerfFrame(){
  if(!mfPerfEnabled)return;
  if(!playing||paused){
   mfPerfPrevious=0;mfPerfStart=0;mfPerfFrames=0;mfPerfWorst=0;mfPerfSlow=0;
   mfPerfHud.textContent=paused?'FPS: paused':'FPS: idle';
   return;
  }
  const now=performance.now();
  if(!mfPerfPrevious){mfPerfPrevious=now;mfPerfStart=now;return}
  const elapsed=now-mfPerfPrevious;
  mfPerfPrevious=now;
  // Frame time includes vsync scheduling and game work; this is not GPU-only time.
  if(!Number.isFinite(elapsed)||elapsed<=0)return;
  mfPerfWorst=Math.max(mfPerfWorst,elapsed);
  if(elapsed>50)mfPerfSlow++;
  mfPerfFrames++;
  const windowMs=now-mfPerfStart;
  if(windowMs<1000)return;
  const fps=Math.round(mfPerfFrames*1000/windowMs);
  mfPerfHud.textContent='FPS '+fps+' | worst '+mfPerfWorst.toFixed(0)+' ms | >50 ms '+mfPerfSlow;
  mfPerfStart=now;mfPerfFrames=0;mfPerfWorst=0;mfPerfSlow=0;
 }
}
function patchJs(source){
 const full=mfPerfRuntime.toString();
 const logic=full.slice(full.indexOf('{')+1,full.lastIndexOf('}'));
 return mfPerfOnce(source,
  'function loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
  logic+'\nfunction loop(){mfPerfFrame();pollController();tick();draw();requestAnimationFrame(loop)}',
  'frame loop');
}
function patchHtml(source){
 source=mfPerfOnce(source,
 '<button id="mfScaleBtn" type="button" title="Resolution scaling, not NVIDIA DLSS">🖥 Render: Native 100%</button>',
 '<button id="mfScaleBtn" type="button" title="Resolution scaling, not NVIDIA DLSS">🖥 Render: Native 100%</button><button id="mfPerfBtn" type="button" aria-pressed="false" title="Show FPS and frame pacing; F6">📊 FPS: OFF</button><output id="mfPerfHud" style="display:none;position:fixed;top:8px;right:8px;z-index:50;padding:6px 10px;background:rgba(10,8,24,.93);color:#fff;border:1px solid #bbaaff;font:bold 13px system-ui;pointer-events:none" aria-live="off">FPS: measuring…</output>',
 'performance controls');
 return source;
}
module.exports={patchJs,patchHtml};
