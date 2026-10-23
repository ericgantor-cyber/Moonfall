const assert=require('node:assert/strict');
const test=require('node:test');
const mobs=require('./mob-variants');
test('catalog contains 11,111 deterministic entries',()=>{
 assert.equal(mobs.COUNT,11111);
 for(let i=0;i<mobs.COUNT;i++){
  const v=mobs.variant(i);
  assert.equal(v.id,i);
  assert.ok(v.stats.health>0&&v.stats.speed>0&&v.stats.damage>0);
  assert.ok(v.appearance.hue>=0&&v.appearance.hue<360);
 }
 assert.deepEqual(mobs.variant(101),mobs.variant(101));
});
test('random spawning is reproducible',()=>{
 assert.deepEqual(mobs.randomVariant(333,88),mobs.randomVariant(333,88));
 assert.throws(()=>mobs.variant(11111),RangeError);
 assert.equal(mobs.applyToEnemy({},3).mobVariantId,3);
});
