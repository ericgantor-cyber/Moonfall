// Integrates the existing deterministic mob catalog on demand, without mesh bloat.
function one(s,a,b,label){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)>=0)throw Error('Mob hook missing or ambiguous: '+label);return s.slice(0,i)+b+s.slice(i+a.length)}
function patchJs(s){
 const swap=(a,b,label)=>{s=one(s,a,b,label)};
 swap('function beginStage(){','function beginStage(){mfMobSpawnIndex=0;','stage start');
 swap('function reset(){','function reset(){mfMobRunSeed=Math.floor(Math.random()*2147483647);mfMobSpawnIndex=0;','run reset');
 const helpers=[
 'let mfMobRunSeed=0,mfMobSpawnIndex=0;',
 'function mfCreateMob(e){',
 ' const v=MoonfallMobVariants.randomVariant(mfMobRunSeed,mfMobSpawnIndex++);',
 ' MoonfallMobVariants.applyToEnemy(e,v);',
 ' e.r=Math.max(12,Math.min(42,Math.round(e.r*v.size)));',
 ' e.max=Math.round(e.max*v.stats.health);e.hp=e.max;',
 ' return e;',
 '}',
 'function mfMobDamage(e,n){return n*(e.mobVariant?.stats.damage||1)}',
 ''].join('\n');
 swap('function tick(){',helpers+'function tick(){','mob helpers');
 swap('enemies.push({x:edge===0?0:edge===1?W:Math.random()*W,y:edge===2?0:edge===3?H:Math.random()*H,r:[24,20,23][type],hp,max:hp,fire:Math.floor(Math.random()*70),type})','enemies.push(mfCreateMob({x:edge===0?0:edge===1?W:Math.random()*W,y:edge===2?0:edge===3?H:Math.random()*H,r:[24,20,23][type],hp,max:hp,fire:Math.floor(Math.random()*70),type}))','normal spawn');
 swap('let speed=[1.28,1.65,1.12][e.type??1]*(mfRoom?.speed||1);','let speed=[1.28,1.65,1.12][e.type??1]*(mfRoom?.speed||1)*(e.mobVariant?.stats.speed||1);','mob speed');
 swap('gainXP(e.boss?120:8);','gainXP(e.boss?120:Math.round(8*(e.mobVariant?.stats.experience||1)));','mob XP');
 swap('receiveDamage(e.boss?14:e.type===0?9:e.type===2?7:5)','receiveDamage(mfMobDamage(e,e.boss?14:e.type===0?9:e.type===2?7:5))','contact damage');
 const art=['}else{const type=e.type??1;',
 'if(e.mobVariant){const v=e.mobVariant;ctx.save();',
 "ctx.strokeStyle='hsl('+v.appearance.hue+' 95% 63%)';",
 'ctx.lineWidth=v.elite?5:3;ctx.shadowBlur=v.appearance.glow?16:0;',
 'ctx.shadowColor=ctx.strokeStyle;ctx.beginPath();ctx.arc(e.x,e.y,e.r+8,0,Math.PI*2);ctx.stroke();',
 'if(v.appearance.pattern%2===0){for(let k=0;k<v.appearance.horns;k++){const a=k*Math.PI*2/Math.max(1,v.appearance.horns);',
 'ctx.beginPath();ctx.moveTo(e.x+Math.cos(a)*(e.r+3),e.y+Math.sin(a)*(e.r+3));',
 'ctx.lineTo(e.x+Math.cos(a)*(e.r+12),e.y+Math.sin(a)*(e.r+12));ctx.stroke();}}',
 'ctx.restore();}',
 'circle(e.x,e.y,e.r+5,'].join('');
 swap('}else{const type=e.type??1;circle(e.x,e.y,e.r+5,',art,'distinct appearance');
 return s;
}
module.exports={patchJs};
