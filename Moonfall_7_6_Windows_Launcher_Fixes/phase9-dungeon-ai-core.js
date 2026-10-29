'use strict';
// Spatial algorithms with no gameplay state or art dependencies.
const ENCOUNTER_LIMIT=4;
function expanded(box,pad=0){
 return {x:box.x-pad,y:box.y-pad,w:box.w+pad*2,h:box.h+pad*2};
}
// Liang-Barsky slab clipping handles fast projectiles crossing a wall in one tick.
function segmentRectHit(from,to,box,padding=0){
 if(!from||!to||!box||![from.x,from.y,to.x,to.y,box.x,box.y,box.w,box.h,padding].every(Number.isFinite))return false;
 const b=expanded(box,Math.max(0,padding)),dx=to.x-from.x,dy=to.y-from.y;
 let lo=0,hi=1;
 for(const [p,v,d,span] of [[from.x,b.x,dx,b.w],[from.y,b.y,dy,b.h]]){
  if(Math.abs(d)<1e-9){if(p<v||p>v+span)return false;continue;}
  const a=(v-p)/d,c=(v+span-p)/d;
  lo=Math.max(lo,Math.min(a,c));hi=Math.min(hi,Math.max(a,c));
  if(lo>hi)return false;
 }
 return hi>=0&&lo<=1;
}
function shotBlocked(from,to,r,pillars){
 if(!pillars)return false;
 for(const wall of pillars)if(segmentRectHit(from,to,wall,Math.max(0,r||0)))return true;
 return false;
}
function obstacleBetween(from,to,pillars,r=0){
 for(const wall of pillars||[])if(segmentRectHit(from,to,wall,r))return wall;
 return null;
}
function distance(a,b){return Math.hypot(a.x-b.x,a.y-b.y)}
// Local visibility-graph waypoint navigation. Only evaluate when a blocker is in the way.
// Upper bound is four candidates per blocker and four room walls.
function chooseWaypoint(from,goal,pillars,r=12){
 const wall=obstacleBetween(from,goal,pillars,r+2);
 if(!wall)return goal;
 const pad=r+18,corners=[
  {x:wall.x-pad,y:wall.y-pad},
  {x:wall.x+wall.w+pad,y:wall.y-pad},
  {x:wall.x-pad,y:wall.y+wall.h+pad},
  {x:wall.x+wall.w+pad,y:wall.y+wall.h+pad}
 ];
 let best=null,cost=Infinity;
 for(const waypoint of corners){
  if(obstacleBetween(from,waypoint,pillars,r))continue;
  const onward=obstacleBetween(waypoint,goal,pillars,r);
  const score=distance(from,waypoint)+distance(waypoint,goal)+(onward?10000:0);
  if(score<cost){cost=score;best=waypoint}
 }
 // If all direct corners are obstructed, move tangent to the nearest wall edge.
 if(!best){
  best=corners.reduce((a,b)=>distance(from,a)<distance(from,b)?a:b);
 }
 return best;
}
function encounterCount(stage,chamber){
 if(!Number.isInteger(stage)||!Number.isInteger(chamber)||chamber<0||chamber>2)return 0;
 if([2,4,5,6,7,8].includes(stage)||chamber===0)return 0;
 return Math.min(ENCOUNTER_LIMIT,chamber+1);
}
module.exports={ENCOUNTER_LIMIT,segmentRectHit,shotBlocked,obstacleBetween,chooseWaypoint,encounterCount};
