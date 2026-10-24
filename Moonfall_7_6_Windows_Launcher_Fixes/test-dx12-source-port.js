'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const game=fs.readFileSync(path.join(__dirname,'modular/game.js'),'utf8');
const sources=require('./prepare-dx12-heroes.js').heroSources;
const pkg=require('./package.json');
const assets=[
 ['Z','art-001.webp','hero-z.png'],
 ['Nyra','art-004.png','hero-nyra.png'],
 ['Bear enemy','art-005.webp','enemy-bear.png'],
 ['Creep enemy 1','art-006.webp','enemy-creep-1.png'],
 ['Creep enemy 2','art-007.webp','enemy-creep-2.png'],
 ['Creep enemy 3','art-008.webp','enemy-creep-3.png'],
 ['Fire Titan','art-009.png','boss-titan-fire.png'],
 ['Nature Titan','art-010.png','boss-titan-nature.png']
];
assert.equal(sources.length,assets.length);
for(let i=0;i<assets.length;i++){
 const [name,source,target]=assets[i];
 assert.deepEqual([sources[i].name,sources[i].source,sources[i].target],assets[i]);
 assert(game.includes('assets/'+source),'Generated game missing original art '+source);
 assert(pkg.build.extraResources.some(v=>v.to==='dx12/'+target&&v.from.endsWith('/Release/'+target)), 'Installer missing '+target);
}
assert(/const BEAR_ART=['"]assets\/art-005\.webp['"]/.test(game),'Bear source identification changed');
assert(/const CREEP_ART=\[['"]assets\/art-006\.webp['"],\s*['"]assets\/art-007\.webp['"],\s*['"]assets\/art-008\.webp['"]\]/.test(game),'Creep source identification changed');
assert(/const titanImages=\[/.test(game)&&game.includes("assets/art-009.png")&&game.includes("assets/art-010.png"),'Titan source identification changed');
assert(game.includes('second?145:190'),'Original boss attack interval changed');
assert(game.includes('n=second?9:5'),'Original boss volley counts changed');
assert(game.includes('const pattern=stage%3'),'Original boss targeting mode changed');
assert(game.includes('Moonsteel Edge')&&game.includes('Crimson Vitality'),'Original boon names changed');
const cpp=fs.readFileSync(path.join(__dirname,'../native/dx12-prototype/arena.hpp'),'utf8');
assert(cpp.includes('145.f:190.f')&&cpp.includes('second?9:5'),'Native patterns diverged from game counts');
const rooms=require('./roguelike-rooms.js');
assert(rooms.logic===undefined,'Keep original stage transforms encapsulated');
// Verify the exact room and elite source constants, then their native equivalents.
const roomSource=fs.readFileSync(path.join(__dirname,'roguelike-rooms.js'),'utf8');
for(const token of ["speed:1.17,cadence:1.2","speed:1.05,cadence:1.0","speed:1.0,cadence:1.4","frame%85","i*.21","*4.2","damage:13"])
  assert(roomSource.includes(token),'Main-game room/elite source constant changed: '+token);
for(const token of ["{1.17f,1.2f}","{1.05f,1.f}","{1.f,1.4f}","85.f/60.f","float(i)*.21f","4.2f*60.f","3.5f,13.f,true"])
  assert(cpp.includes(token),'Native room/elite rule missing: '+token);
console.log('PASS: original enemies/Titan artwork; boss volleys; 3 room modifiers and elite attack; packaged resources');
