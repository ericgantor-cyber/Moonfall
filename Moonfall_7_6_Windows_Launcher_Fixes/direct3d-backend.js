// Safe opt-in Direct3D 11 ANGLE backend selection for Electron on Windows.
// This configures Chromium. It does NOT add a native DirectX renderer or DLSS.
'use strict';
const fs=require('node:fs');
const path=require('node:path');
const SETTINGS='moonfall-graphics-backend.json';
function validateBackend(value){return value==='d3d11'?'d3d11':'auto'}
function location(app){return path.join(app.getPath('userData'),SETTINGS)}
function loadBackend(app){
 try {
  const obj=JSON.parse(fs.readFileSync(location(app),'utf8'));
  return validateBackend(obj && obj.backend);
 }catch(_){return 'auto'}
}
function saveBackend(app,value){
 if(value!=='auto'&&value!=='d3d11')throw new TypeError('Invalid graphics backend');
 const target=location(app),folder=path.dirname(target),temp=target+'.tmp';
 fs.mkdirSync(folder,{recursive:true});
 try{
  fs.writeFileSync(temp,JSON.stringify({backend:value},null,2)+'\n','utf8');
  fs.renameSync(temp,target);
 }finally{
  try{fs.unlinkSync(temp)}catch(_){}
 }
}
function configureBackend(app,platform,argv=[]){
 const mode=argv.includes('--moonfall-safe-graphics')?'auto':loadBackend(app);
 if(platform==='win32'&&mode==='d3d11'){
  // Must be set before Electron app.ready; ANGLE backs Chromium WebGL/graphics.
  app.commandLine.appendSwitch('use-gl','angle');
  app.commandLine.appendSwitch('use-angle','d3d11');
 }
 return platform==='win32'?mode:'auto';
}
module.exports={validateBackend,loadBackend,saveBackend,configureBackend};
