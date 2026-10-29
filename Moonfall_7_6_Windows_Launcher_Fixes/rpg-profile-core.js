'use strict';
// Versioned persistent RPG progression. Sanitizes untrusted local save data.
const VERSION=1;
const HEROES=Object.freeze(['z','nyra']);
const REGIONS=Object.freeze(['witchwood','foundry','city','caverns','bosslands']);
const MATERIALS=Object.freeze(['shard','ember','fang','moon']);
function integer(value,max=1000000){
 return Number.isSafeInteger(value)&&value>=0?Math.min(max,value):0;
}
function rankArray(values,length,max){
 return Array.from({length},(_,i)=>integer(Array.isArray(values)?values[i]:undefined,max));
}
function newProfile(hero){
 if(!HEROES.includes(hero))throw Error('Invalid hero');
 return {version:VERSION,hero,level:1,xp:0,skillPoints:0,stats:Array(8).fill(0),
  spellRanks:Array(4).fill(0),damageLevel:0,souls:0,pendingLevelChoices:0,
  checkpoint:{region:'witchwood',stage:0,chamber:0},
  materials:{shard:0,ember:0,fang:0,moon:0},loot:[],
  equipped:{weapon:null,armor:null},gearSeq:0,
  abilities:{hellUnlocked:false,bloodUnlocked:false,titanFire:false,titanNature:false}};
}
function cleanItem(item){
 if(!item||typeof item!=='object'||Array.isArray(item))return null;
 if(!Number.isInteger(item.template)||item.template<0||item.template>999||!Number.isSafeInteger(item.id)||item.id<1||item.id>1000000000)return null;
 return {template:item.template,id:item.id};
}
function cleanProfile(input,hero){
 if(!HEROES.includes(hero))throw Error('Invalid hero');
 if(!input||input.version!==VERSION||input.hero!==hero)throw Error('Unsupported RPG save');
 const out=newProfile(hero);
 out.level=Math.max(1,integer(input.level,9999));
 out.xp=integer(input.xp,Math.max(0,out.level*25-1));
 out.skillPoints=integer(input.skillPoints,30000);
 out.stats=rankArray(input.stats,8,20);
 out.spellRanks=rankArray(input.spellRanks,4,5);
 out.damageLevel=integer(input.damageLevel,10000);
 out.souls=integer(input.souls,1000000000);
 out.pendingLevelChoices=integer(input.pendingLevelChoices,100);
 const cp=input.checkpoint||{};
 out.checkpoint={
  region:REGIONS.includes(cp.region)?cp.region:'witchwood',
  stage:integer(cp.stage,8),chamber:integer(cp.chamber,2)
 };
 const m=input.materials||{};
 for(const key of MATERIALS)out.materials[key]=integer(m[key],1000000000);
 out.gearSeq=integer(input.gearSeq,1000000000);
 out.loot=Array.isArray(input.loot)?input.loot.slice(0,300).map(cleanItem).filter(Boolean):[];
 const e=input.equipped||{};
 out.equipped={
  weapon:cleanItem(e.weapon),
  armor:cleanItem(e.armor)
 };
 const ab=input.abilities||{};
 for(const key of Object.keys(out.abilities))out.abilities[key]=ab[key]===true;
 return out;
}
function checkpoint(profile,region,stage,chamber){
 if(!REGIONS.includes(region)||!Number.isInteger(stage)||stage<0||stage>8||!Number.isInteger(chamber)||chamber<0||chamber>2)return false;
 profile.checkpoint={region,stage,chamber};return true;
}
function reviveItem(saved,definitions,slot){
 if(!saved||!Array.isArray(definitions))return null;
 const def=definitions[saved.template];
 if(!def||!['weapon','armor'].includes(def.slot)||slot&&def.slot!==slot)return null;
 return {...def,id:saved.id};
}
function serializeItem(item,definitions){
 if(!item||!Array.isArray(definitions))return null;
 const template=definitions.findIndex(d=>d&&d.name===item.name&&d.slot===item.slot);
 return cleanItem({template,id:item.id});
}
module.exports={VERSION,HEROES,REGIONS,MATERIALS,newProfile,cleanProfile,checkpoint,reviveItem,serializeItem};
