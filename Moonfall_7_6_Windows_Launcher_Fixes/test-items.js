const fs=require('fs'),vm=require('vm'),assert=require('assert');
const game=fs.readFileSync(__dirname+'/game.html','utf8');
const match=game.match(/const waveUpgradePool=\[([\s\S]*?)\];/);
assert(match,'Upgrade pool not found');
const pool=vm.runInNewContext('['+match[1]+']');
assert.strictEqual(pool.length,1605,'Expected 1,605 cards');
const names=new Set(pool.map(x=>x.name));
assert.strictEqual(names.size,1605,'Upgrade names must be unique');
for(const [index,item] of pool.entries()){
 assert(typeof item.name==='string'&&item.name.length>0,`Bad item name ${index}`);
 assert(typeof item.description==='string'&&item.description.length>0,`Bad description ${index}`);
 assert(typeof item.apply==='function',`Missing effect ${index}`);
}
// The latest 1,000 cards are data-driven and should execute without relying on scene state.
for(let i=605;i<pool.length;i++){
 const state={waveDamageBonus:1,spellMultiplier:1,maxHP:100,hp:50,waveMoveBonus:1,waveAttackCooldown:17,updateLevelHUD:()=>{}};
 const context=vm.createContext(state);
 const effect=vm.runInContext('({' +pool[i].apply.toString()+ '}).apply',context);
 effect();
 for(const key of ['waveDamageBonus','spellMultiplier','maxHP','hp','waveMoveBonus','waveAttackCooldown'])assert(Number.isFinite(state[key]),`Nonfinite effect ${i}: ${key}`);
 assert(state.hp>=0&&state.hp<=state.maxHP,`Invalid health effect ${i}`);
 assert(state.waveAttackCooldown>=7,`Invalid attack cooldown ${i}`);
}
assert(game.includes('bs.chosen.length>10000'),'Upgrade history save validation is too restrictive');
assert(game.includes('mfChosenBoonNames.length>10000'),'Upgrade history not bounded');
console.log('PASS: 1,605 unique upgrades, 1,000 executable item effects, save-history limit');
