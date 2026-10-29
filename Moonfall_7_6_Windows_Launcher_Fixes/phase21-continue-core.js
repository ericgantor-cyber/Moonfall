'use strict';
// Validate a resume manifest against the exact checkpoint it describes.
function canContinue(manifest,checkpoint,hero,region,stage){
 if(!manifest||manifest.version!==1||!checkpoint||checkpoint.version!==1||
   manifest.region!==region||manifest.stage!==stage||
   checkpoint.region!==region||checkpoint.stage!==stage||
   manifest.hero!==hero||!['z','nyra'].includes(hero)||
   !Number.isFinite(manifest.x)||!Number.isFinite(manifest.y)||
   !Number.isFinite(checkpoint.x)||!Number.isFinite(checkpoint.y)||
   manifest.x!==checkpoint.x||manifest.y!==checkpoint.y||
   manifest.checkpointFrame!==checkpoint.at||
   !Number.isFinite(checkpoint.hp)||checkpoint.hp<=0)return false;
 return Number.isInteger(manifest.rank)&&manifest.rank>=1&&manifest.rank<=100;
}
module.exports={canContinue};
