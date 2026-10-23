// Exercise the generated Windows game's combat helper code in isolation.
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const path=require('node:path');
const catalog=require('./mob-variants.js');
assert.equal(catalog.COUNT,11111);
for(let i=0;i<300;i++){
 const v=catalog.randomVariant(42831,i);
 assert.deepEqual(v,catalog.randomVariant(42831,i),'mob selection must be deterministic');
 assert(v.stats.health>0&&v.stats.speed>0&&v.stats.damage>0,'invalid mob stats');
 const e={hp:42};catalog.applyToEnemy(e,v);
 assert.equal(e.hp,42,'applying a variant should not unexpectedly mutate base HP');
}
assert.throws(()=>catalog.variant(11111),RangeError);
const source=fs.readFileSync(path.join(__dirname,'modular/game.js'),'utf8');
function section(start,end){
 const a=source.indexOf(start),b=source.indexOf(end,a+start.length);
 assert(a>=0&&b>a,'Missing generated-game section: '+start);
 return source.slice(a,b);
}
const roleCode=section('function mfRoleMove(e,a,s){','function mfSetRoom(){');
const countCode=section('function mfNormalEnemyCount(){','let mfRouteChoice=null');
const env={frame:0,stage:1,Math,player:{x:100,y:100},shots:[],enemies:[],mfMobDamage:(_e,n)=>n,burst(){}};
vm.createContext(env);
vm.runInContext(roleCode+'\n'+countCode,env);
const tank={x:0,y:100,mobVariant:{role:'tank',id:0}};
env.mfRoleMove(tank,0,10);
assert.equal(tank.x,8,'tank speed should be reduced');
const ranged={x:90,y:100,mobVariant:{role:'ranged',id:0}};
env.mfRoleMove(ranged,0,10);
assert(ranged.x<90,'ranged enemy should retreat from close player');
env.enemies=[{mfElite:false},{mfElite:true},{mfElite:false}];
assert.equal(env.mfNormalEnemyCount(),2,'elite mobs must not count toward normal spawns');
const shooter={x:0,y:0,hp:30,max:30,mobVariant:{role:'ranged',id:0,stats:{attackRate:1}}};
env.enemies=[shooter];env.mfRoleAttack(shooter);
assert.equal(env.shots.length,1,'ranged mob should shoot');
const healer={x:0,y:0,hp:30,max:30,mobVariant:{role:'support',id:0,stats:{attackRate:1}}};
const injured={x:3,y:4,hp:5,max:30};env.enemies=[healer,injured];env.mfRoleAttack(healer);
assert.equal(injured.hp,15,'support mob should heal an ally');
assert(source.includes('mfRouteChoice=[2,4,5,6,7,8].includes(stage)?0:null;'),'boss chamber must bypass irrelevant route choice');
assert(!source.includes('enemies.filter(e=>!e.mfElite)'),'spawn loop must not allocate arrays');
assert(!source.includes('mfVector=mfRoleMove'),'enemy movement must not allocate movement vectors');
console.log('PASS: generated mob roles, elite counting, procedural determinism, boss route and allocation checks');
