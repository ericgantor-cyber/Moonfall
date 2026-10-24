'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const os=require('node:os');
const path=require('node:path');
const {validateBackend,loadBackend,saveBackend,configureBackend}=require('./direct3d-backend.js');
const folder=fs.mkdtempSync(path.join(os.tmpdir(),'moonfall-gpu-'));
let switches=[];
const app={
 getPath(name){assert.equal(name,'userData');return folder;},
 commandLine:{appendSwitch(name,value){switches.push([name,value]);}}
};
try {
 assert.equal(validateBackend('auto'),'auto');
 assert.equal(validateBackend('invalid'),'auto');
 assert.equal(validateBackend('d3d11'),'d3d11');
 assert.equal(loadBackend(app),'auto','missing preference must be safe');
 assert.equal(configureBackend(app,'win32'), 'auto');
 assert.equal(switches.length,0,'automatic mode should leave Chromium untouched');
 saveBackend(app,'d3d11');
 assert.equal(loadBackend(app),'d3d11');
 assert.equal(configureBackend(app,'win32'),'d3d11');
 assert.deepEqual(switches,[['use-gl','angle'],['use-angle','d3d11']]);
 switches=[];
 assert.equal(configureBackend(app,'win32',['--moonfall-safe-graphics']), 'auto');
 assert.deepEqual(switches,[],'safe graphics override must not request D3D11');
 assert.equal(loadBackend(app),'d3d11','safe override must not modify saved preference');
 assert.equal(configureBackend(app,'darwin'), 'auto');
 assert.deepEqual(switches,[],'ANGLE D3D11 must not be requested off Windows');
 assert.throws(()=>saveBackend(app,'vulkan'),TypeError);
 saveBackend(app,'auto');
 assert.equal(loadBackend(app),'auto');
 fs.writeFileSync(path.join(folder,'moonfall-graphics-backend.json'),'invalid JSON');
 assert.equal(loadBackend(app),'auto','broken settings must fall back safely');
 fs.writeFileSync(path.join(folder,'moonfall-graphics-backend.json'),JSON.stringify({backend:'unexpected'}));
 assert.equal(loadBackend(app),'auto','unknown settings must fall back safely');
 const pkg=require('./package.json');
 assert(pkg.build.files.includes('direct3d-backend.js'),'packaged app must contain backend helper');
 const source=fs.readFileSync(path.join(__dirname,'main.js'),'utf8');
 assert(source.includes("configureBackend(app,process.platform,process.argv)"),'backend not selected at startup');
 assert(source.includes("Renderer: ANGLE Direct3D 11"),'graphics menu missing');
 assert(source.indexOf('configureBackend(app,process.platform,process.argv)') <
   source.indexOf('app.whenReady().then('),'backend flag set too late');
 assert(source.includes('--moonfall-safe-graphics'),'safe startup override missing');
 console.log('PASS: Direct3D11 flags, safe fallback, settings persistence, menu and package checks');
} finally {
 fs.rmSync(folder,{recursive:true,force:true});
}
