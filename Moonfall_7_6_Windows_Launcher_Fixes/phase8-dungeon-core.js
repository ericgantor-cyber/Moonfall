'use strict';
// Deterministic, asset-free dungeon geometry. Keep the central lane unobstructed.
const CHAMBERS=3;
const PALETTES=Object.freeze({
 witchwood:'#83e3a5',foundry:'#fcb171',city:'#cba5fa',
 caverns:'#85d9f5',bosslands:'#ff749a'
});
function validSize(w,h){return Number.isFinite(w)&&Number.isFinite(h)&&w>=320&&h>=240}
function makeDungeonRoom({region='witchwood',stage=1,chamber=0,width,height}){
 if(!validSize(width,height)||!Object.hasOwn(PALETTES,region)||
  !Number.isInteger(stage)||stage<1||!Number.isInteger(chamber)||chamber<0||chamber>=CHAMBERS)
  throw Error('Invalid dungeon room');
 const W=width,H=height,flip=(stage+chamber+region.length)%2;
 const pillars=[];
 for(let i=0;i<4;i++){
  const row=i>=2?1:0,col=i%2;
  const cx=W*(col?.71:.29)+(flip?5:-5),cy=H*(row?.76:.25);
  const w=Math.min(74,W*.085),h=Math.min(67,H*.105);
  pillars.push({x:cx-w/2,y:cy-h/2,w,h});
 }
 const hazards=[];
 if(chamber!==0)hazards.push({
  x:W*(flip?.57:.43)-W*.045,y:H*(chamber===1?.22:.73)-H*.043,
  w:W*.09,h:H*.086
 });
 if(chamber===2)hazards.push({
  x:W*.49-W*.045,y:H*.22-H*.043,w:W*.09,h:H*.086
 });
 return {id:region+':'+stage+':'+chamber,region,stage,chamber,width:W,height:H,color:PALETTES[region],
  pillars,hazards,doors:{left:chamber>0,right:chamber<CHAMBERS-1}};
}
function hitCircleRect(x,y,r,box){
 const px=Math.max(box.x,Math.min(x,box.x+box.w));
 const py=Math.max(box.y,Math.min(y,box.y+box.h));
 return (x-px)**2+(y-py)**2<r*r;
}
// Axis-separated sweep: prevents high-speed dash through pillars, and allows sliding.
function resolveDungeonMotion(from,to,r,pillars){
 if(!Number.isFinite(from.x)||!Number.isFinite(from.y)||
  !Number.isFinite(to.x)||!Number.isFinite(to.y)||!Number.isFinite(r)||r<=0)throw Error('Invalid player position');
 let x=to.x,y=to.y;
 for(const b of pillars){
  const left=b.x-r,right=b.x+b.w+r,top=b.y-r,bottom=b.y+b.h+r;
  if(from.y>top&&from.y<bottom){
   if(from.x<=left&&x>left)x=left;
   else if(from.x>=right&&x<right)x=right;
  }
 }
 for(const b of pillars){
  const left=b.x-r,right=b.x+b.w+r,top=b.y-r,bottom=b.y+b.h+r;
  if(x>left&&x<right){
   if(from.y<=top&&y>top)y=top;
   else if(from.y>=bottom&&y<bottom)y=bottom;
  }
 }
 // Resolve a corrupted spawn inside a pillar without changing the underlying sprite.
 for(const b of pillars){
  if(!hitCircleRect(x,y,r,b))continue;
  const distances=[
   {d:Math.abs(x-(b.x-r)),axis:'x',v:b.x-r},
   {d:Math.abs(x-(b.x+b.w+r)),axis:'x',v:b.x+b.w+r},
   {d:Math.abs(y-(b.y-r)),axis:'y',v:b.y-r},
   {d:Math.abs(y-(b.y+b.h+r)),axis:'y',v:b.y+b.h+r}
  ].sort((a,b)=>a.d-b.d);
  const c=distances[0];if(c.axis==='x')x=c.v;else y=c.v;
 }
 return {x,y};
}
function doorway(room,x,y,width,height,cooldownFrames){
 if(cooldownFrames>0||Math.abs(y-height*.5)>height*.11)return null;
 if(room.doors.left&&x<=49)return 'left';
 if(room.doors.right&&x>=width-49)return 'right';
 return null;
}
module.exports={CHAMBERS,PALETTES,makeDungeonRoom,hitCircleRect,resolveDungeonMotion,doorway};
