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
// The desktop shell serves the bundled Three.js module offline.
const shellSource=fs.readFileSync(path.join(__dirname,'main.js'),'utf8');
if(!shellSource.includes("node_modules','three','build','three.module.js"))throw new Error('Bundled Three.js engine path missing');
if(!shellSource.includes("'/vendor/three.module.js'"))throw new Error('Offline Three.js route missing');
if(['cdn.jsdelivr.net/npm/three','unpkg.com/three','esm.sh/three'].some(host=>game.includes(host)))throw new Error('Remote engine fallback found');
if(/mfScanEnabled|mfScanTimer/.test(game))throw new Error('Single-switch controls reintroduced');
const main=require('child_process').spawnSync(process.execPath,['--check',path.join(__dirname,'main.js')],{encoding:'utf8'});
if(main.status!==0)throw new Error(main.stderr);
const pkg=JSON.parse(fs.readFileSync(path.join(__dirname,'package.json'),'utf8'));
if(!pkg.build?.nsis?.createDesktopShortcut)throw new Error('Installer desktop shortcut missing');
// Version-specific behavior must be checked by executable regression tests,
// not exact whitespace-sensitive snippets copied from older builds.
console.log('PASS: '+tested+' inline scripts, offline engine, installer configuration');
const saveTests=require('child_process').spawnSync(process.execPath,[path.join(__dirname,'test-save-system.js')],{encoding:'utf8'});
if(saveTests.status!==0)throw new Error('Save regression tests failed: '+saveTests.stderr+' '+saveTests.stdout);
process.stdout.write(saveTests.stdout);
const itemTests=require('child_process').spawnSync(process.execPath,[path.join(__dirname,'test-items.js')],{encoding:'utf8'});
if(itemTests.status!==0)throw new Error('Item regression tests failed: '+itemTests.stderr+' '+itemTests.stdout);
process.stdout.write(itemTests.stdout);

console.log('PASS: executable save and item regression suites');
