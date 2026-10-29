'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm');
const core=require('./rpg-profile-core.js'),integrate=require('./rpg-persistence-integration.js');
assert.deepEqual(core.HEROES,['z','nyra']);
assert.throws(()=>core.newProfile('unknown'),/Invalid hero/);
const corrupt={...core.newProfile('z'),level:-2,xp:-5,skillPoints:Infinity,stats:[2,99,-1,2.1],spellRanks:[7],checkpoint:{region:'evil',stage:999,chamber:-1},
 loot:[{template:0,id:1},{template:-1,id:9},null],equipped:{weapon:{template:1,id:10}},
 materials:{shard:-5},abilities:{bloodUnlocked:'true'}};
const clean=core.cleanProfile(corrupt,'z');
assert.equal(clean.level,1);
assert.equal(clean.xp,0);
assert.equal(clean.stats[1],20);
assert.equal(clean.checkpoint.region,'witchwood');
assert.equal(clean.loot.length,1);
assert.equal(clean.abilities.bloodUnlocked,false);
assert.throws(()=>core.cleanProfile({...corrupt,version:4},'z'));
assert.throws(()=>core.cleanProfile(corrupt,'nyra'));
const defs=[{name:'Worn Sword',slot:'weapon',atk:2},{name:'Stone Armor',slot:'armor',armor:2}];
assert.deepEqual(core.serializeItem({name:'Worn Sword',slot:'weapon',id:5},defs),{template:0,id:5});
assert.equal(core.reviveItem({template:1,id:5},defs,'weapon'),null);
assert.equal(core.reviveItem({template:0,id:5},defs,'weapon').atk,2);
const image='data:image/png;base64,AAABAA==';
const fixture=`const originalZAndNyraArt='${image}';
let selectedHero='z',stage=0,level=1,xp=0,skillPoints=0,damageLevel=0,souls=0,
 stats=Array(8).fill(0),spellRanks={z:[0,0,0,0],nyra:[0,0,0,0]},
 pendingLevelChoices=0,activeLevelCards=[],loot=[],gearSeq=0,
 equipped={weapon:null,armor:null},materials={shard:0,ember:0,fang:0,moon:0},
 hellUnlocked=false,bloodUnlocked=false,titanFire=false,titanNature=false,
 player={x:250,y:250,r:20,hp:100,max:100},playing=false,won=false,upgradePending=false,paused=false,
 frame=0,mfDungeonChamber=0,mfWorld={region:'witchwood',visited:['witchwood'],unlocked:['witchwood'],flags:{}},
 mfP11BonusApplied=false;
const gearDefs=[{name:'Worn Sword',slot:'weapon',atk:2},{name:'Stone Armor',slot:'armor',armor:2}];
const mfWorldCore={unlockRegion(w,region){if(!w.unlocked.includes(region))w.unlocked.push(region)}};
function maxHp(){return 100+stats[0]*10}
function reset(){level=1;xp=0;skillPoints=0;damageLevel=0;souls=0;stats=Array(8).fill(0);
 spellRanks={z:[0,0,0,0],nyra:[0,0,0,0]};loot=[];gearSeq=0;equipped={weapon:null,armor:null};
 materials={shard:0,ember:0,fang:0,moon:0};hellUnlocked=false;bloodUnlocked=false;
 titanFire=false;titanNature=false;stage=0;mfWorld.region='witchwood';mfDungeonChamber=0;
 frame=0;player={x:250,y:250,r:20,hp:100,max:100};beginStage();playing=true;}
function beginStage(){mfDungeonChamber=0}
function mfDungeonEnter(chamber){mfDungeonChamber=chamber}
function mfWorldSave(){worldSaves++}
function renderSkills(){}function renderGear(){}function renderSpellTree(){}function updateHUD(){}
function showLevelChoices(){}
function gainXP(n){xp+=n;while(xp>=level*25){xp-=level*25;level++;skillPoints+=3;}}
function upgrade(i){stats[i]++}function upgradeSpell(i){spellRanks[selectedHero][i]++}
function grantLoot(i){let item={...gearDefs[i],id:++gearSeq};loot.push(item);return item}
function craftItem(i){return grantLoot(i)}function equipItem(id){const index=loot.findIndex(x=>x.id===id);
 if(index>=0)equipped[loot[index].slot]=loot.splice(index,1)[0]}
function unequipItem(slot){if(equipped[slot])loot.push(equipped[slot]);equipped[slot]=null}
function overlay(title,msg,button){playing=false;document.getElementById('overlay').classList.remove('hidden')}
function tick(){frame++}
document.getElementById('start').onclick=()=>{reset()};
function loop(){pollController();tick();draw();requestAnimationFrame(loop)}
`;
const compiled=integrate.patchJs(fixture);
new vm.Script(compiled);
assert(compiled.includes(image),'art bytes must remain identical');
assert.match(compiled,/mfRpgRespawn/);
assert.throws(()=>integrate.patchJs('function tick(){}'),/hook/);
assert.match(integrate.patchHtml('<b id="mfP11Hud">Rune</b>'),/mfRpgSaveBtn/);
const storage=new Map();
function makeSession(){
 const nodes=new Map(),node=id=>{
  if(!nodes.has(id))nodes.set(id,{textContent:'',onclick:null,listeners:{},classList:{
   add(){},remove(){}},addEventListener(event,fn){this.listeners[event]=fn}});
  return nodes.get(id);
 };
 const context={worldSaves:0,document:{getElementById:node},localStorage:{
  getItem:k=>storage.get(k)||null,setItem:(k,v)=>storage.set(k,v)},
  console};
 vm.createContext(context);
 vm.runInContext(compiled,context);
 return {context,node,run:src=>vm.runInContext(src,context)};
}
let s=makeSession();
s.node('start').onclick();
assert.equal(s.run('level'),1);
assert(storage.has('moonfall-rpg-character-v1:z'));
s.run("gainXP(74);upgrade(1);upgradeSpell(2);souls=280;grantLoot(1);equipItem(1);mfWorld.region='foundry';stage=3;mfDungeonEnter(2);mfRpgSave()");
assert.equal(s.run('level'),2);
assert.equal(s.run('mfRpgCheckpoint.stage'),3);
assert.equal(s.run('mfRpgCheckpoint.chamber'),2);
assert.equal(s.run("equipped.armor.name"),'Stone Armor');
s.run("player.hp=0;overlay('☠ HERO HAS FALLEN','','RETRY')");
assert.equal(s.run('mfRpgDeathPending'),true);
s.node('start').onclick();
assert.equal(s.run('mfRpgDeathPending'),false);
assert.equal(s.run('level'),2);
assert.equal(s.run('stage'),3);
assert.equal(s.run('mfWorld.region'),'foundry');
assert.equal(s.run('mfDungeonChamber'),2);
assert.equal(s.run("equipped.armor.name"),'Stone Armor');
assert.equal(s.run('souls'),280);
assert.equal(s.run('player.hp'),s.run('player.max'));
s.run("selectedHero='nyra';reset()");
assert.equal(s.run('level'),1,'new hero must have independent progression');
s.run("gainXP(52);mfRpgSave()");
s.run("selectedHero='z';reset()");
assert.equal(s.run('level'),2,'switching back restores Z');
assert.equal(s.run('souls'),280);
s=makeSession();
s.node('start').onclick();
assert.equal(s.run('level'),2,'reload restores persisted character');
assert.equal(s.run('stage'),3);
assert.equal(s.run('mfWorld.region'),'foundry');
s.run("stage=8;mfWorld.region='foundry';overlay('THE MOON IS SAVED','','PLAY AGAIN')");
const zSave=JSON.parse(storage.get('moonfall-rpg-character-v1:z'));
assert.equal(zSave.checkpoint.stage,0,'victory returns to world checkpoint, not new character');
assert.equal(zSave.level,2);
s.node('start').onclick();
assert.equal(s.run('level'),2);
assert.equal(s.run('stage'),0);
storage.set('moonfall-rpg-character-v1:nyra','{invalid-json');
s.run("selectedHero='nyra';reset()");
assert.equal(s.run('level'),1,'corrupt save starts safely');
console.log('PASS: persistent RPG leveling, equipment, checkpoints, death, hero slots, victory, corruption and source art');
