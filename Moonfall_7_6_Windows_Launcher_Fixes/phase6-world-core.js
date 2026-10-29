'use strict';
// Phase 6 world-state and streaming foundation. No gameplay or asset mutations.
const REGIONS=Object.freeze({
 witchwood:['foundry','caverns'],
 foundry:['witchwood','city'],
 city:['foundry','bosslands'],
 caverns:['witchwood','bosslands'],
 bosslands:['city','caverns']
});
const DEFAULT_ROOM_LIMIT=5;
function regionExists(region){return Object.hasOwn(REGIONS,region)}
function assertRegion(region){if(!regionExists(region))throw new Error('Unknown region: '+region)}
function createWorldState(){
 return {version:1,region:'witchwood',visited:['witchwood'],unlocked:['witchwood'],
  defeatedBosses:[],openedChests:[],activatedShrines:[],flags:{}};
}
function validateWorldState(value){
 if(!value||value.version!==1||!regionExists(value.region))throw new Error('Unsupported world save');
 for(const field of ['visited','unlocked','defeatedBosses','openedChests','activatedShrines'])
  if(!Array.isArray(value[field])||value[field].some(x=>typeof x!=='string'))throw new Error('Invalid '+field);
 if(!value.unlocked.includes(value.region))throw new Error('Locked current region');
 if(!value.flags||typeof value.flags!=='object'||Array.isArray(value.flags))throw new Error('Invalid flags');
 for(const r of [...value.visited,...value.unlocked])assertRegion(r);
 return true;
}
function restoreWorldState(json){
 const data=typeof json==='string'?JSON.parse(json):json;
 validateWorldState(data);
 return {version:1,region:data.region,visited:[...new Set(data.visited)],
  unlocked:[...new Set(data.unlocked)],defeatedBosses:[...new Set(data.defeatedBosses)],
  openedChests:[...new Set(data.openedChests)],activatedShrines:[...new Set(data.activatedShrines)],
  flags:{...data.flags}};
}
function unlockRegion(state,region){
 validateWorldState(state);assertRegion(region);
 if(!state.unlocked.includes(region))state.unlocked.push(region);
 return state;
}
function travel(state,destination){
 validateWorldState(state);assertRegion(destination);
 if(!REGIONS[state.region].includes(destination))return false;
 if(!state.unlocked.includes(destination))return false;
 state.region=destination;
 if(!state.visited.includes(destination))state.visited.push(destination);
 return true;
}
function markUnique(state,field,id){
 if(!['defeatedBosses','openedChests','activatedShrines'].includes(field))throw new Error('Invalid marker');
 if(typeof id!=='string'||!id.trim())throw new Error('Invalid marker id');
 validateWorldState(state);
 if(!state[field].includes(id))state[field].push(id);
}
function createRoomStreamer({limit=DEFAULT_ROOM_LIMIT,load,unload}={}){
 if(!Number.isInteger(limit)||limit<1)throw new Error('Invalid streaming limit');
 if(typeof load!=='function'||typeof unload!=='function')throw new Error('load/unload callbacks required');
 const active=new Map();
 function visit(roomId){
  if(typeof roomId!=='string'||!roomId)throw new Error('Invalid room id');
  if(active.has(roomId)){const value=active.get(roomId);active.delete(roomId);active.set(roomId,value);return value}
  const value=load(roomId);
  if(active.size>=limit){
   const [oldId,oldValue]=active.entries().next().value;
   try{unload(oldId,oldValue)}
   catch(e){try{unload(roomId,value)}catch(_){}throw e}
   active.delete(oldId);
  }
  active.set(roomId,value);
  return value;
 }
 function clear(){for(const [id,value] of active){unload(id,value);active.delete(id)}}
 return {visit,clear,activeRooms:()=>[...active.keys()]};
}
module.exports={REGIONS,createWorldState,restoreWorldState,validateWorldState,unlockRegion,travel,markUnique,createRoomStreamer};
