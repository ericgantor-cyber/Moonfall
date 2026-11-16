'use strict';
// Pure, bounded dungeon topology for continuous exploration. No waves, assets, or global game state.
const DUNGEONS_PER_REGION=5;
const CHAMBERS_PER_DUNGEON=14;
const REGIONS=Object.freeze(['witchwood','foundry','city','caverns','bosslands']);
function integerIn(value,min,max){return Number.isInteger(value)&&value>=min&&value<=max}
function validate(region,dungeon,chamber,width,height){
 if(!REGIONS.includes(region)||!integerIn(dungeon,0,DUNGEONS_PER_REGION-1)||
 !integerIn(chamber,0,CHAMBERS_PER_DUNGEON-1)||!Number.isFinite(width)||
 !Number.isFinite(height)||width<320||height<240)throw Error('Invalid continuous dungeon coordinates');
}
function roomAt({region='witchwood',dungeon=0,chamber=0,width=960,height=540}={}){
 validate(region,dungeon,chamber,width,height);
 const seed=(REGIONS.indexOf(region)*37+dungeon*17+chamber*11)>>>0;
 const blockers=[];
 // Four side pillars, central navigation lane intentionally unobstructed.
 for(let i=0;i<4;i++){
  const col=i%2,row=Math.floor(i/2);
  const w=Math.min(width*.078,66),h=Math.min(height*.103,58);
  const x=width*(col?.72:.28)+(seed%7-3)*2-w/2;
  const y=height*(row?.75:.25)-h/2;
  blockers.push({x,y,w,h});
 }
 const hazards=[];
 if(chamber%3===2){
  const w=width*.075,h=height*.075;
  hazards.push({x:width*(seed%2?.58:.42)-w/2,y:height*.34-h/2,w,h});
 }
 return {
  id:`${region}:${dungeon}:${chamber}`,region,dungeon,chamber,width,height,
  blockers,hazards,
  left:chamber>0||dungeon>0,
  right:chamber<CHAMBERS_PER_DUNGEON-1||dungeon<DUNGEONS_PER_REGION-1
 };
}
function neighbor(region,dungeon,chamber,direction){
 validate(region,dungeon,chamber,960,540);
 if(direction!=='left'&&direction!=='right')throw Error('Invalid direction');
 let ordinal=dungeon*CHAMBERS_PER_DUNGEON+chamber+(direction==='right'?1:-1);
 if(ordinal<0||ordinal>=DUNGEONS_PER_REGION*CHAMBERS_PER_DUNGEON)return null;
 return {region,dungeon:Math.floor(ordinal/CHAMBERS_PER_DUNGEON),chamber:ordinal%CHAMBERS_PER_DUNGEON};
}
function circleHitsRect(x,y,r,b){
 const closestX=Math.max(b.x,Math.min(x,b.x+b.w));
 const closestY=Math.max(b.y,Math.min(y,b.y+b.h));
 return (x-closestX)**2+(y-closestY)**2<r*r;
}
// Swept segment against expanded rectangle to prevent high-speed tunnelling.
function segmentHitsRect(a,b,rect,padding=0){
 if(!a||!b||!rect||![a.x,a.y,b.x,b.y,rect.x,rect.y,rect.w,rect.h,padding].every(Number.isFinite))return false;
 const x=rect.x-padding,y=rect.y-padding,w=rect.w+2*padding,h=rect.h+2*padding;
 const dx=b.x-a.x,dy=b.y-a.y;let lo=0,hi=1;
 for(const [start,delta,min,size] of [[a.x,dx,x,w],[a.y,dy,y,h]]){
  if(Math.abs(delta)<1e-9){if(start<min||start>min+size)return false;continue}
  const t0=(min-start)/delta,t1=(min+size-start)/delta;
  lo=Math.max(lo,Math.min(t0,t1));hi=Math.min(hi,Math.max(t0,t1));
  if(lo>hi)return false;
 }
 return hi>=0&&lo<=1;
}
function blockedSegment(a,b,r,blockers){
 return (blockers||[]).some(w=>segmentHitsRect(a,b,w,Math.max(0,r)));
}
function resolveMotion(from,to,r,blockers){
 if(!from||!to||![from.x,from.y,to.x,to.y,r].every(Number.isFinite)||r<=0)throw Error('Invalid movement');
 let x=to.x,y=to.y;
 for(const b of blockers||[]){
  const left=b.x-r,right=b.x+b.w+r,top=b.y-r,bottom=b.y+b.h+r;
  if(from.y>top&&from.y<bottom&&from.x<=left&&x>left)x=left;
  else if(from.y>top&&from.y<bottom&&from.x>=right&&x<right)x=right;
 }
 for(const b of blockers||[]){
  const left=b.x-r,right=b.x+b.w+r,top=b.y-r,bottom=b.y+b.h+r;
  if(x>left&&x<right&&from.y<=top&&y>top)y=top;
  else if(x>left&&x<right&&from.y>=bottom&&y<bottom)y=bottom;
 }
 for(const b of blockers||[]){
  if(!circleHitsRect(x,y,r,b))continue;
  const choices=[{d:Math.abs(x-(b.x-r)),x:b.x-r,y},{d:Math.abs(x-(b.x+b.w+r)),x:b.x+b.w+r,y},
   {d:Math.abs(y-(b.y-r)),x,y:b.y-r},{d:Math.abs(y-(b.y+b.h+r)),x,y:b.y+b.h+r}];
  choices.sort((a,b)=>a.d-b.d);x=choices[0].x;y=choices[0].y;
 }
 return {x,y};
}
// Bounded visibility-graph search: at most four corners per blocking pillar.
function chooseWaypoint(from,goal,blockers,r=12){
 if(!from||!goal||![from.x,from.y,goal.x,goal.y,r].every(Number.isFinite))return goal;
 const walls=blockers||[];
 const blocker=walls.find(w=>segmentHitsRect(from,goal,w,Math.max(0,r)));
 if(!blocker)return goal;
 const pad=Math.max(4,r+8);
 const points=[
  {x:blocker.x-pad,y:blocker.y-pad},
  {x:blocker.x+blocker.w+pad,y:blocker.y-pad},
  {x:blocker.x-pad,y:blocker.y+blocker.h+pad},
  {x:blocker.x+blocker.w+pad,y:blocker.y+blocker.h+pad}
 ];
 let best=null,score=Infinity;
 for(const candidate of points){
  if(blockedSegment(from,candidate,Math.max(0,r),walls))continue;
  const onward=blockedSegment(candidate,goal,Math.max(0,r),walls);
  const d=Math.hypot(candidate.x-from.x,candidate.y-from.y)+Math.hypot(candidate.x-goal.x,candidate.y-goal.y)+(onward?10000:0);
  if(d<score){score=d;best=candidate;}
 }
 return best||goal;
}
module.exports={DUNGEONS_PER_REGION,CHAMBERS_PER_DUNGEON,REGIONS,roomAt,neighbor,circleHitsRect,segmentHitsRect,blockedSegment,resolveMotion,chooseWaypoint};
