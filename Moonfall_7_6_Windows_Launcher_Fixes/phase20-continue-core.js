'use strict';
// Phase 20: compact versioned resume manifest. This is metadata, never a live-game restore.
const REGIONS=new Set(['witchwood','foundry','city','caverns','bosslands']);
const HEROES=new Set(['z','nyra']);
function resumeManifest({checkpoint,hero,rank}={}){
 if(!checkpoint||checkpoint.version!==1||!REGIONS.has(checkpoint.region)||
    !Number.isInteger(checkpoint.stage)||checkpoint.stage<1||checkpoint.stage>1000||
    !Number.isFinite(checkpoint.x)||!Number.isFinite(checkpoint.y)||
    checkpoint.x<0||checkpoint.x>10000||checkpoint.y<0||checkpoint.y>10000||
    !Number.isInteger(checkpoint.at)||checkpoint.at<0)return null;
 const selected=String(hero||'').toLowerCase();
 if(!HEROES.has(selected))return null;
 const level=Number.isInteger(rank)?Math.max(1,Math.min(100,rank)):1;
 return {version:1,hero:selected,region:checkpoint.region,stage:checkpoint.stage,
   x:checkpoint.x,y:checkpoint.y,checkpointFrame:checkpoint.at,rank:level};
}
function validResume(value){
 if(!value||value.version!==1)return false;
 const reconstructed=resumeManifest({checkpoint:{
  version:1,region:value.region,stage:value.stage,
  x:value.x,y:value.y,at:value.checkpointFrame
 },hero:value.hero,rank:value.rank});
 return !!reconstructed&&JSON.stringify(reconstructed)===JSON.stringify(value);
}
module.exports={resumeManifest,validResume};
