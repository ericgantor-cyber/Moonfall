// Resolution scaling for the 2D canvas renderer; this is NOT NVIDIA DLSS.
'use strict';
function one(s,needle,value,label) {
  const index=s.indexOf(needle);
  if(index<0||s.indexOf(needle,index+needle.length)!==-1)throw Error('Render scale integration mismatch: '+label);
  return s.slice(0,index)+value+s.slice(index+needle.length);
}
function mfResolutionRuntime(){
  const mfScaleModes=[
    {id:'native',name:'Native',ratio:1},
    {id:'quality',name:'Quality',ratio:.85},
    {id:'balanced',name:'Balanced',ratio:.70},
    {id:'performance',name:'Performance',ratio:.55}
  ];
  const mfScaleKey='moonfall-resolution-scale-v1';
  const mfScaleCanvas=ctx.canvas;
  const mfScaleBaseWidth=mfScaleCanvas.width;
  const mfScaleBaseHeight=mfScaleCanvas.height;
  const mfScaleBtn=document.getElementById('mfScaleBtn');
  let mfScaleIndex=0,mfScaleApplied=-1;
  try {
    const stored=localStorage.getItem(mfScaleKey);
    const index=mfScaleModes.findIndex(mode=>mode.id===stored);
    if(index!==-1)mfScaleIndex=index;
  }catch(_){}
  function mfScaleButtonUpdate(){
    if(!mfScaleBtn)return;
    const mode=mfScaleModes[mfScaleIndex];
    mfScaleBtn.textContent='🖥 Render: '+mode.name+' '+Math.round(mode.ratio*100)+'%';
    mfScaleBtn.title='Render-resolution scaling. Not NVIDIA DLSS.';
    mfScaleBtn.setAttribute('aria-label','Render resolution: '+mode.name+', '+Math.round(mode.ratio*100)+' percent. Activate for next setting.');
  }
  function mfScaleNext(){
    mfScaleIndex=(mfScaleIndex+1)%mfScaleModes.length;
    try{localStorage.setItem(mfScaleKey,mfScaleModes[mfScaleIndex].id);}catch(_){}
    mfScaleButtonUpdate();
  }
  if(mfScaleBtn)mfScaleBtn.addEventListener('click',mfScaleNext);
  mfScaleButtonUpdate();
  function mfApplyRenderScale(){
    if(!mfScaleCanvas||!mfScaleBaseWidth||!mfScaleBaseHeight)return;
    const ratio=mfScaleModes[mfScaleIndex].ratio;
    if(mfScaleApplied!==mfScaleIndex){
      // Retain the same displayed canvas dimensions when the backing buffer shrinks.
      const rect=mfScaleCanvas.getBoundingClientRect();
      if(!mfScaleCanvas.style.width&&rect.width>0)mfScaleCanvas.style.width=rect.width+'px';
      if(!mfScaleCanvas.style.height&&rect.height>0)mfScaleCanvas.style.height=rect.height+'px';
      const width=Math.max(1,Math.round(mfScaleBaseWidth*ratio));
      const height=Math.max(1,Math.round(mfScaleBaseHeight*ratio));
      if(mfScaleCanvas.width!==width)mfScaleCanvas.width=width;
      if(mfScaleCanvas.height!==height)mfScaleCanvas.height=height;
      mfScaleApplied=mfScaleIndex;
    }
    // Canvas size changes reset context state, so use absolute scaling every frame.
    ctx.setTransform(mfScaleCanvas.width/mfScaleBaseWidth,0,0,
      mfScaleCanvas.height/mfScaleBaseHeight,0,0);
    ctx.imageSmoothingEnabled=true;
    ctx.imageSmoothingQuality='high';
  }
}
function patchJs(s){
  const source=mfResolutionRuntime.toString();
  const runtime=source.slice(source.indexOf('{')+1,source.lastIndexOf('}'));
  return one(s,
    'function loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
    runtime+'\nconst mfOriginalGraphicsDraw=draw;\ndraw=function(){mfApplyRenderScale();mfOriginalGraphicsDraw()};\nfunction loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
    'draw wrapper');
}
function patchHtml(s){
  return one(s,
    '<button id="mfSpeechFocusBtn" type="button" aria-pressed="false">💬 All dialogue</button>',
    '<button id="mfSpeechFocusBtn" type="button" aria-pressed="false">💬 All dialogue</button><button id="mfScaleBtn" type="button" title="Resolution scaling, not NVIDIA DLSS">🖥 Render: Native 100%</button>',
    'graphics button');
}
module.exports={patchJs,patchHtml};
