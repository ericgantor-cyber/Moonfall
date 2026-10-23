// Exercise actual generated level-up logic without launching Electron.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict'),path=require('node:path');
const js=fs.readFileSync(path.join(__dirname,'modular/game.js'),'utf8');
function extract(a,b){
 const start=js.indexOf(a);assert(start>=0,'Missing '+a);
 const end=js.indexOf(b,start+a.length);assert(end>start,'Missing boundary '+b);
 return js.slice(start,end);
}
const gain=extract('function gainXP(n){','const spellNames=');
const choices=extract('function renderLevelCards(){','function toggleSkills(force){');
const cards={innerHTML:'',classList:{hidden:true,add(){this.hidden=true},remove(){this.hidden=false}}};
const scope={
 xp:24,level:1,skillPoints:0,pendingLevelChoices:0,activeLevelCards:[],
 stats:[0,0,0,0,0,0,0,0],statNames:['HP','ATK','SPD','ARM','LIFE','REGEN','CRIT','CRIT DMG'],
 increments:[100,15,.1,10,1,1,2,10],
 player:{x:2,y:3},playing:true,skillOpen:false,
 Math,burst(){},renderSkills(){},
 document:{getElementById(id){assert.equal(id,'levelCards');return cards}},
 toggleSkills(v){this.skillOpen=v}, // replaced below by a free function in VM
 upgrade(i){scope.stats[i]++;scope.skillPoints--},
};
const ctx=vm.createContext(scope);
vm.runInContext('function toggleSkills(v){skillOpen=v} '+choices+' '+gain,ctx);
vm.runInContext('gainXP(2)',ctx);
assert.equal(scope.level,2,'XP must increase level');
assert.equal(scope.pendingLevelChoices,1,'Level must queue choice');
assert.equal(scope.skillOpen,true,'Choice UI must open immediately without timers');
assert.equal(cards.classList.hidden,false,'Card container must be visible');
assert.equal(scope.activeLevelCards.length,3,'Exactly three cards must appear');
const selected=scope.activeLevelCards[0];
vm.runInContext('chooseLevelCard('+selected+')',ctx);
assert.equal(scope.pendingLevelChoices,0,'Choice must be consumed');
assert.equal(scope.skillOpen,false,'Combat must resume after choice');
assert.equal(scope.stats[selected],1,'Chosen upgrade must apply');
assert.equal(scope.skillPoints,2,'Remaining skill points must be retained');
console.log('PASS: level-up immediately shows three choices, applies one, resumes');
