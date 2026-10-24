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
console.log('PASS: original Moonfall bear, creep, Titan sprite mapping; boss volleys; room blessings; packed resource names');
