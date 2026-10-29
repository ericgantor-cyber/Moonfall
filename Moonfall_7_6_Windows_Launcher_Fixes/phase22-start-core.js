'use strict';
// Pure start-up validation. Never apply mismatched progress to a different character.
function continuationPlan(manifest,checkpoint,profile){
 if(!manifest||manifest.version!==1||!checkpoint||checkpoint.version!==1||
    !['z','nyra'].includes(manifest.hero)||
    !['witchwood','foundry','city','caverns','bosslands'].includes(manifest.region)||
    !Number.isInteger(manifest.stage)||manifest.stage<1||manifest.stage>1000||
    checkpoint.region!==manifest.region||checkpoint.stage!==manifest.stage||
    !Number.isFinite(checkpoint.x)||!Number.isFinite(checkpoint.y)||
    checkpoint.x!==manifest.x||checkpoint.y!==manifest.y||
    checkpoint.at!==manifest.checkpointFrame||
    !Number.isFinite(checkpoint.hp)||checkpoint.hp<=0||
    !Number.isInteger(manifest.rank)||manifest.rank<1||manifest.rank>100||
    !profile||!Number.isInteger(profile.level)||profile.level!==manifest.rank)return null;
 return Object.freeze({hero:manifest.hero,region:manifest.region,stage:manifest.stage,
   x:checkpoint.x,y:checkpoint.y,rank:manifest.rank});
}
module.exports={continuationPlan};
