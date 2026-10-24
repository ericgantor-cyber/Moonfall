'use strict';
const assert=require('node:assert/strict');
const path=require('node:path');
const {prototypePath,launchPrototype,EXE}=require('./dx12-launch.js');
const pkg=require('./package.json');
const fs=require('node:fs');
assert.equal(EXE,'MoonfallDX12Prototype.exe');
assert.equal(
 prototypePath({isPackaged:true,resourcesPath:path.join('C:','Programs','Moonfall','resources'),appDir:'irrelevant'}),
 path.join('C:','Programs','Moonfall','resources','dx12',EXE));
assert.equal(
 prototypePath({isPackaged:false,resourcesPath:'irrelevant',appDir:path.join('workspace','Moonfall_7_6_Windows_Launcher_Fixes')}),
 path.resolve('workspace','native','dx12-prototype','build','Release',EXE));
assert.throws(()=>launchPrototype('missing.exe',{fileExists:()=>false}),/not installed/);
let invoked;
const child={on(){}};
const executable=path.resolve('work','resources','dx12',EXE);
const launched=launchPrototype(executable,{
 fileExists:file=>{assert.equal(file,executable);return true;},
 spawnChild:(file,args,options)=>{invoked={file,args,options};return child;}
});
assert.equal(launched,child);
assert.equal(invoked.file,executable);
assert.deepEqual(invoked.args,[]);
assert.equal(invoked.options.cwd,path.dirname(executable));
assert.equal(invoked.options.stdio,'ignore');
assert.equal(invoked.options.windowsHide,false);
assert(!Object.hasOwn(invoked.options,'shell'),'must not allow shell execution');
assert(pkg.scripts['build:windows'].includes('npm run build:dx12'));
assert(pkg.scripts['build:dx12'].includes('cmake --build'));
assert(pkg.build.files.includes('dx12-launch.js'));
assert(pkg.build.extraResources.some(x=>x.to==='dx12/'+EXE&&x.from.endsWith('/Release/'+EXE)));
const source=fs.readFileSync('main.js','utf8');
assert(source.includes("label:'Play DX12 Combat Arena (experimental)'"));
assert(source.includes("prototypePath({isPackaged:app.isPackaged,resourcesPath:process.resourcesPath,appDir:__dirname})"));
assert(source.includes("child.on('error'"),'spawn errors must be handled');
console.log('PASS: bundled DX12 executable path, safe launcher, source wiring, and installer configuration');
