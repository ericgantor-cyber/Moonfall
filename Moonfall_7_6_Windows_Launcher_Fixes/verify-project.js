const fs=require('fs'),vm=require('vm'),path=require('path');
const game=fs.readFileSync(path.join(__dirname,'game.html'),'utf8');
const scripts=[...game.matchAll(/<script([^>]*)>([\s\S]*?)<\/script>/g)];
let tested=0;
for(const [,attrs,code] of scripts){
 if(!code.trim())continue;
 // Module script supports top-level await and imports, checked using Node's CLI.
 const file=path.join(__dirname,`._moonfall_check_${tested++}.${attrs.includes('module')?'mjs':'js'}`);
 fs.writeFileSync(file,code);
 try{
  const result=require('child_process').spawnSync(process.execPath,['--check',file],{encoding:'utf8'});
  if(result.status!==0)throw new Error(result.stderr);
 }finally{fs.unlinkSync(file)}
}
// Upgrade pool format varies across game versions; avoid brittle source-string counts.
if(!game.includes("import('/vendor/three.module.js')"))throw new Error('Offline engine missing');
if(/cdn\.jsdelivr\.net\/npm\/three|unpkg\.com\/three|esm\.sh\/three/.test(game))throw new Error('Remote engine fallback found');
if(/mfScanEnabled|mfScanTimer/.test(game))throw new Error('Single-switch controls reintroduced');
if(!game.includes('waveStats:{move:waveMoveBonus'))throw new Error('Wave bonus checkpoint missing');
const main=require('child_process').spawnSync(process.execPath,['--check',path.join(__dirname,'main.js')],{encoding:'utf8'});
if(main.status!==0)throw new Error(main.stderr);
const pkg=JSON.parse(fs.readFileSync(path.join(__dirname,'package.json'),'utf8'));
if(!pkg.build?.nsis?.createDesktopShortcut)throw new Error('Installer desktop shortcut missing');
if(!game.includes('aiStartFrame=frame; // Prevent AI first-frame reset'))throw new Error('Restored timer guard missing');
if(!game.includes('mfCheckpointRecency(pending)>mfCheckpointRecency(active)'))throw new Error('Staged recovery comparison missing');
if(!game.includes('mfCommitCheckpoint(previous,{backup:false})'))throw new Error('Recovery history protection missing');
console.log('PASS: '+tested+' scripts, '+'game upgrade logic, offline engine, wave-stat saves, installer configuration');

if(!game.includes('mfNewestCheckpoint([previous,manual])'))throw new Error('Newest backup recovery missing');
if(!game.includes('d.savedAt>Date.now()+300000'))throw new Error('Timestamp validation missing');

if(!game.includes('boonState:{patrons:{...boonPatronCounts}'))throw new Error('Boon progression save missing');
if(!game.includes('boonPatronCounts={...bs.patrons}'))throw new Error('Boon progression restore missing');

if(!game.includes('const smoothing=1-Math.pow(.72,dtFrames)'))throw new Error('Frame-independent AI smoothing missing');
if(!game.includes('aiTacticHoldUntil=aiElapsedSeconds+8'))throw new Error('AI tactic hysteresis missing');
if(!game.includes("aiTactic==='pressure'"))throw new Error('AI pressure tactic missing');

if(game.includes("aiElapsedSeconds=0;aiSpawnClock=3;aiVolleyClock=0;"))throw new Error("AI first-frame timer reset remains");
if(game.includes("m.flank=(i%2"))throw new Error("Scout flanking tied to array position");
if(!game.includes("const nextTactic=hp<maxHP*.35?"))throw new Error("Low health pressure precedence missing");

// Moonfall 3.0 combat regression checks.
if(!game.includes('const displacement=Math.hypot(movedX,movedZ)'))throw new Error('Teleport prediction detection missing');
if(game.includes('player.position.x-(aiLastX-rawVX*dtFrames)'))throw new Error('Broken teleport detection returned');
if(!game.includes('new THREE.Mesh(mfHazardGeometry,mfHazardMaterial)'))throw new Error('Shared hazard resources missing');
if(/new THREE.Mesh\(new THREE.RingGeometry\(1\.7,2\.1,36\)/.test(game))throw new Error('Per-hazard GPU allocations returned');
console.log('PASS: combat teleport and hazard resource regressions');

// Moonfall 3.1 regression checks.
for(const needle of ['function mfCheckpointHash(data)', 'function mfSealCheckpoint(data)', 'mfCheckpointHash(d)!==d.integrity', "checkpoint:mfSealCheckpoint(d)", "mfSaveHealthButton.textContent='🔎 Check save health'"]) {
 if(!game.includes(needle))throw new Error('Missing save integrity feature: '+needle);
}
if(game.includes('if(!playing||won||waveUpgradeOpen||aiPaused)return null'))throw new Error('Paused manual saving blocked');
console.log('PASS: checkpoint integrity, sealed exports, paused saving, diagnostics');
const saveTests=require('child_process').spawnSync(process.execPath,[path.join(__dirname,'test-save-system.js')],{encoding:'utf8'});
if(saveTests.status!==0)throw new Error('Save regression tests failed: '+saveTests.stderr+' '+saveTests.stdout);
process.stdout.write(saveTests.stdout);
const itemTests=require('child_process').spawnSync(process.execPath,[path.join(__dirname,'test-items.js')],{encoding:'utf8'});
if(itemTests.status!==0)throw new Error('Item regression tests failed: '+itemTests.stderr+' '+itemTests.stdout);
process.stdout.write(itemTests.stdout);

// 4.4 rendering regression checks.
for(const needle of ['function mfUpdateMobRenderLOD()', 'children[k].visible=!far', 'if(playing)mfUpdateMobRenderLOD();', 'const batch=Math.min(count,Math.max(20,Math.ceil(count/8)))', '<span id="mobUi">0</span> / 500']){
 if(!game.includes(needle))throw new Error('Missing render LOD: '+needle);
}
console.log('PASS: 500-mob render LOD and accurate HUD cap');

// 4.5 projectile reuse checks.
for(const needle of ['const mfBoltPool=[];', 'function mfReleaseBolt(b)', 'function mfClearBolts()', 'mfBoltPool.pop()||new THREE.Mesh', 'mfReleaseBolt(b);bolts.splice(i,1)']){
 if(!game.includes(needle))throw new Error('Missing projectile optimization: '+needle);
}
if(game.includes('bolts.forEach(b=>scene.remove(b.mesh));bolts=[];'))throw new Error('Unpooled bolt cleanup remains');
console.log('PASS: pooled projectile creation and cleanup');
