// Moonfall's procedural enemy variety catalog.
// 11,111 reproducible variants. Generate on demand; never allocate 11,111 meshes.
// CommonJS and browser global API; no third-party dependencies.
(function(root,factory){
  const api=factory();
  if(typeof module==='object' && module.exports) module.exports=api;
  else root.MoonfallMobVariants=api;
})(typeof globalThis!=='undefined'?globalThis:this,function(){
  'use strict';
  const COUNT=11111;
  const FAMILIES=['wraith','golem','beast','crawler','specter','slime','construct','dragonkin','thornling','shade','imp','reaver','sentinel','mimic','elemental','hollow'];
  const ELEMENTS=['fire','ice','storm','earth','void','poison','arcane','moonlight'];
  const ROLES=['chaser','tank','ranged','flanker','support','ambusher'];
  const TITLES=['Feral','Ancient','Eclipsed','Cursed','Elder','Ravenous','Fractured','Dread','Lunar','Wild','Phantom','Iron','Howling','Blazing','Frostbound','Twisted'];
  const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
  function hash(n){
    n=(n+0x6D2B79F5)|0;
    n=Math.imul(n^(n>>>15),n|1);
    n^=n+Math.imul(n^(n>>>7),n|61);
    return (n^(n>>>14))>>>0;
  }
  function variant(id){
    if(!Number.isInteger(id)||id<0||id>=COUNT)throw new RangeError('mob id must be 0..11110');
    let seed=hash(id+0x4D4F4F4E);
    const next=()=>{seed=hash(seed);return seed/4294967296;};
    const pick=a=>a[Math.floor(next()*a.length)];
    const family=pick(FAMILIES),element=pick(ELEMENTS),role=pick(ROLES);
    const size=+(0.7+next()*1.2).toFixed(2);
    const elite=next()<0.055;
    const stats={
      health: +(clamp((0.7+next()*1.35)*(elite?2.2:1),0.5,5)).toFixed(2),
      damage: +(clamp((0.65+next()*1.25)*(elite?1.45:1),0.4,4)).toFixed(2),
      speed: +(clamp((0.72+next()*0.72)/(size**0.28),0.5,2)).toFixed(2),
      attackRate: +(0.7+next()*0.8).toFixed(2),
      experience: +(elite?2.5:0.85+next()*0.5).toFixed(2)
    };
    const hue=Math.floor(next()*360);
    return {id,key:'mob-'+String(id).padStart(5,'0'),
      name:pick(TITLES)+' '+element+' '+family+(elite?' (Elite)':''),
      family,element,role,elite,size,stats,
      appearance:{hue,accentHue:(hue+90+Math.floor(next()*100))%360,
        pattern:Math.floor(next()*8),horns:Math.floor(next()*4),
        glow:next()<0.55,eyes:1+Math.floor(next()*4)}};
  }
  // Different spawn index + run seed gives a reproducible, well-distributed pool.
  function randomVariant(runSeed,spawnIndex){
    const seed=Number.isFinite(runSeed)?Math.trunc(runSeed):0;
    const index=Number.isFinite(spawnIndex)?Math.trunc(spawnIndex):0;
    return variant(hash(seed^hash(index))%COUNT);
  }
  function applyToEnemy(enemy,mob){
    if(!enemy||typeof enemy!=='object')throw new TypeError('enemy required');
    const v=typeof mob==='number'?variant(mob):mob;
    enemy.mobVariantId=v.id;
    enemy.mobVariant=v;
    // Multipliers only: callers decide which base stats, visuals and AI to use.
    return enemy;
  }
  return Object.freeze({COUNT,FAMILIES,ELEMENTS,ROLES,variant,randomVariant,applyToEnemy});
});
