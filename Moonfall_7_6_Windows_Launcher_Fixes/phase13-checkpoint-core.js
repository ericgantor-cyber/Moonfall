'use strict';
// Defensively serialize only stable character fields, never transient enemies/projectiles.
const clamp=(v,min,max)=>Number.isFinite(v)?Math.max(min,Math.min(max,v)):min;
function checkpoint(data){
 if(!data||typeof data!=='object')return null;
 const {region,stage,player}=data;
 if(!['witchwood','foundry','city','caverns','bosslands'].includes(region)||
    !Number.isInteger(stage)||stage<1||stage>1000||!player||
    !Number.isFinite(player.x)||!Number.isFinite(player.y))return null;
 const hp=Number.isFinite(player.hp)?clamp(player.hp,1,100000):null;
 const maxHp=Number.isFinite(player.maxHp)?clamp(player.maxHp,1,100000):null;
 return {version:1,region,stage,
   x:clamp(player.x,0,10000),y:clamp(player.y,0,10000),
   hp:hp===null?null:Math.min(hp,maxHp===null?hp:maxHp),
   maxHp,at:Number.isFinite(data.frame)?Math.max(0,Math.floor(data.frame)):0};
}
function matches(cp,region,stage){
 return !!cp&&cp.version===1&&cp.region===region&&cp.stage===stage&&
   Number.isFinite(cp.x)&&Number.isFinite(cp.y)&&
   cp.x>=0&&cp.y>=0&&cp.x<=10000&&cp.y<=10000;
}
function respawn(cp,width,height){
 if(!matches(cp,cp&&cp.region,cp&&cp.stage))return null;
 return {x:clamp(cp.x,20,Math.max(20,width-20)),
  y:clamp(cp.y,20,Math.max(20,height-20)),
  hp:cp.maxHp===null?null:Math.max(1,Math.floor(cp.maxHp*.5))};
}
module.exports={checkpoint,matches,respawn};
