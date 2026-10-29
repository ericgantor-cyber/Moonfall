'use strict';
// Minimal checkpoint redundancy. Invalid save payloads are never applied to gameplay.
function valid(cp,region,stage){
 return !!cp&&cp.version===1&&cp.region===region&&cp.stage===stage&&
 Number.isFinite(cp.x)&&Number.isFinite(cp.y)&&cp.x>=0&&cp.y>=0&&
 cp.x<=10000&&cp.y<=10000&&Number.isFinite(cp.at)&&cp.at>=0&&
 (cp.hp===null||cp.hp===undefined||(Number.isFinite(cp.hp)&&cp.hp>0))&&
 (cp.maxHp===null||cp.maxHp===undefined||(Number.isFinite(cp.maxHp)&&cp.maxHp>0));
}
function select(primary,backup,region,stage){
 if(valid(primary,region,stage))return {checkpoint:primary,source:'primary'};
 if(valid(backup,region,stage))return {checkpoint:backup,source:'backup'};
 return {checkpoint:null,source:'none'};
}
function savePair(primary,backup,next){
 if(!valid(next,next&&next.region,next&&next.stage))return {primary,backup};
 // Keep a last-known-good copy from the same stage before replacing the primary.
 const previous=valid(primary,next.region,next.stage)?primary:
   valid(backup,next.region,next.stage)?backup:null;
 return {primary:next,backup:previous};
}
module.exports={valid,select,savePair};
