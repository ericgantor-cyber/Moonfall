// Moonfall desktop shell. Serves the game and its bundled Three.js engine locally.
const {app,BrowserWindow,shell,dialog,Menu}=require('electron');
const http=require('node:http');
const fs=require('node:fs');
const path=require('node:path');
const {formatGpuReport}=require('./gpu-support.js');
const {loadBackend,saveBackend,configureBackend}=require('./direct3d-backend.js');
// Configure Chromium's requested ANGLE backend before app.ready.
// Existing game saves remain on the same local origin regardless of mode.
const activeGraphicsBackend=configureBackend(app,process.platform,process.argv);

let server=null,mainWindow=null;
let rendererRecoveryAttempts=0;
const MAX_RENDERER_RECOVERY_ATTEMPTS=2;
// Keep the origin identical between launches. A random port changes the
// localStorage origin and makes saves/settings appear to disappear.
const GAME_PORT=47831;
const GAME_ORIGIN=`http://127.0.0.1:${GAME_PORT}`;
// Prevent duplicate game windows and concurrent save writers.
const singleInstance=app.requestSingleInstanceLock();
if(!singleInstance){app.quit();}
else app.on('second-instance',()=>{
 if(mainWindow&&!mainWindow.isDestroyed()){
  if(mainWindow.isMinimized())mainWindow.restore();
  mainWindow.focus();
 }
});
const gameRoot=fs.existsSync(path.join(__dirname,'modular','game.html'))?path.join(__dirname,'modular'):__dirname;
const gamePath=path.join(gameRoot,'game.html');
const enginePath=path.join(__dirname,'node_modules','three','build','three.module.js');
function serveFile(res,file,type){
 // Check before committing headers; never duplicate or lose the first chunk.
 fs.stat(file,(statErr,stat)=>{
  if(res.destroyed)return;
  if(statErr||!stat.isFile()){
   console.error('Moonfall file error:',statErr?.message||'Not a file');
   res.writeHead(500,{'Content-Type':'text/plain; charset=utf-8','Cache-Control':'no-store'});
   res.end('Moonfall cannot read '+path.basename(file)+'. Please reinstall Moonfall.');
   return;
  }
  const stream=fs.createReadStream(file);
  stream.once('open',()=>{
   if(res.destroyed){stream.destroy();return;}
   res.writeHead(200,{'Content-Type':type,'Cache-Control':'no-store','X-Content-Type-Options':'nosniff'});
   stream.pipe(res);
  });
  stream.on('error',err=>{
   console.error('Moonfall read error:',err.message);
   if(!res.headersSent){
    res.writeHead(500,{'Content-Type':'text/plain; charset=utf-8'});
    res.end('Moonfall could not load '+path.basename(file)+'. Please reinstall Moonfall.');
   }else if(!res.destroyed)res.destroy(err);
  });
  res.on('close',()=>stream.destroy());
 });
}
function startLocalGameServer(){
 return new Promise((resolve,reject)=>{
  server=http.createServer((req,res)=>{
   // Reject requests from other origins/hostnames, even though bound to loopback.
   const host=req.headers.host;
   if(host!==`127.0.0.1:${GAME_PORT}`){res.writeHead(403);res.end('Forbidden');return;}
   // Browsers and web pages outside the game must not read its local files.
   const origin=req.headers.origin;
   if(origin&&origin!==GAME_ORIGIN){res.writeHead(403);res.end('Forbidden');return;}
   if(req.method!=='GET'&&req.method!=='HEAD'){
    res.writeHead(405,{'Allow':'GET, HEAD'});res.end();return;
   }
   let route;
   try{route=new URL(req.url||'/', 'http://127.0.0.1').pathname}catch{route='/invalid'}
   if(route==='/'||route==='/game.html')return serveFile(res,gamePath,'text/html; charset=utf-8');
   if(route==='/vendor/three.module.js')return serveFile(res,enginePath,'text/javascript; charset=utf-8');
   if(route==='/game.js'&&gameRoot!==__dirname)return serveFile(res,path.join(gameRoot,'game.js'),'text/javascript; charset=utf-8');
   if(/^\/assets\/art-[0-9]{3}\.(webp|png|jpeg)$/.test(route)&&gameRoot!==__dirname){
    const ext=path.extname(route).slice(1);return serveFile(res,path.join(gameRoot,route.slice(1)),'image/'+(ext==='jpeg'?'jpeg':ext));
   }
   res.writeHead(404,{'Content-Type':'text/plain; charset=utf-8'});res.end('Not found');
  });
  const onStartupError=err=>{
   if(err.code==='EADDRINUSE')reject(new Error('Moonfall local save port '+GAME_PORT+' is busy. Close other Moonfall instances or restart Windows, then try again.'));
   else reject(err);
  };
  server.once('error',onStartupError);
  server.listen(GAME_PORT,'127.0.0.1',()=>{
   server.removeListener('error',onStartupError);
   server.on('error',err=>console.error('Moonfall server error:',err));
   resolve(GAME_ORIGIN+'/');
  });
 });
}
async function showGraphicsStatus(){
 if(!mainWindow||mainWindow.isDestroyed())return;
 let gpuInfo={};
 try{gpuInfo=await app.getGPUInfo('basic');}
 catch(err){console.warn('GPU device details unavailable:',err.message);}
 const details=formatGpuReport(app.getGPUFeatureStatus(),gpuInfo,app.isHardwareAccelerationEnabled())
  +'\n\nBackend requested for this launch: '+(activeGraphicsBackend==='d3d11'?'ANGLE Direct3D 11':'Chromium automatic')
  +'\nSaved backend for next launch: '+(loadBackend(app)==='d3d11'?'ANGLE Direct3D 11':'Chromium automatic')
  +'\nBackend selection is a request, not proof that the GPU is using Direct3D 11.'
  +'\nDirect3D 11 through ANGLE does not expose NVIDIA DLSS to Moonfall.';
 await dialog.showMessageBox(mainWindow,{
  type:'info',title:'Moonfall Graphics / NVIDIA',message:'Moonfall graphics diagnostics',
  detail:details,buttons:['OK']
 });
}
async function chooseGraphicsBackend(nextMode){
 if(process.platform!=='win32')return;
 try{saveBackend(app,nextMode)}
 catch(err){dialog.showErrorBox('Could not save graphics backend',String(err));return}
 if(!mainWindow||mainWindow.isDestroyed())return;
 await dialog.showMessageBox(mainWindow,{
  type:'info',title:'Moonfall renderer settings',
  message:'Graphics setting saved',
  detail:(nextMode==='d3d11'?'ANGLE Direct3D 11':'Chromium automatic')+
   ' will be requested the next time Moonfall launches.\nClose and reopen Moonfall when it is safe to stop the game.\n'+
   'If the experimental option causes graphics trouble, choose Automatic or start with --moonfall-safe-graphics. This setting does not enable DLSS.',
  buttons:['OK']
 });
}
function showNvidiaTips(){
 if(!mainWindow||mainWindow.isDestroyed())return;
 dialog.showMessageBox(mainWindow,{
  type:'info',title:'NVIDIA App recording',message:'Capture Moonfall with NVIDIA App',
  detail:'Install NVIDIA App and turn on its in-game overlay.\nPress Alt+Z to open NVIDIA controls, or Alt+F9 to start/stop recording.\nIf Moonfall is not recognized for capture, check the NVIDIA App recording settings. NVIDIA features depend on your GPU, drivers and app support.',
  buttons:['OK']
 }).catch(err=>console.error('NVIDIA help dialog:',err));
}
async function createWindow(){
 for(const file of [gamePath,enginePath]){
  if(!fs.existsSync(file))throw new Error('Missing required file: '+file+'\nPlease reinstall Moonfall.');
 }
 const url=await startLocalGameServer();
 mainWindow=new BrowserWindow({title:'Moonfall: Eclipse Reborn',width:1366,height:850,minWidth:900,minHeight:600,autoHideMenuBar:true,
  backgroundColor:'#080916',
  webPreferences:{nodeIntegration:false,contextIsolation:true,sandbox:true,webSecurity:true,backgroundThrottling:false}});
 mainWindow.on('closed',()=>{mainWindow=null;});
 // Accessible native launcher controls; press Alt to show the menu on Windows.
 const menu=Menu.buildFromTemplate([
  {label:'Moonfall',submenu:[
   {label:'Play / Restart Game',accelerator:'Ctrl+Shift+R',click:()=>{rendererRecoveryAttempts=0;mainWindow?.webContents.reload()}},
   {label:'Recover Frozen Game',click:()=>{rendererRecoveryAttempts=0;mainWindow?.webContents.reloadIgnoringCache()}},
   {label:'Fullscreen',accelerator:'F11',click:()=>{if(mainWindow&&!mainWindow.isDestroyed())mainWindow.setFullScreen(!mainWindow.isFullScreen())}},
   {type:'separator'},
   {label:'Exit',accelerator:'Alt+F4',click:()=>app.quit()}
  ]},
  {label:'Graphics',submenu:[
   {label:'Renderer: Automatic (recommended)',type:'radio',checked:loadBackend(app)!=='d3d11',
    click:()=>chooseGraphicsBackend('auto').catch(err=>console.error('Graphics backend:',err))},
   {label:'Renderer: ANGLE Direct3D 11 (experimental, Windows)',type:'radio',checked:loadBackend(app)==='d3d11',
    enabled:process.platform==='win32',click:()=>chooseGraphicsBackend('d3d11').catch(err=>console.error('Graphics backend:',err))},
   {type:'separator'},
   {label:'GPU Status / NVIDIA Check',click:()=>showGraphicsStatus().catch(err=>dialog.showErrorBox('GPU diagnostics error',String(err)))},
   {label:'NVIDIA Recording Tips',click:showNvidiaTips},
   {label:'Download NVIDIA App (Official)',click:()=>shell.openExternal('https://www.nvidia.com/en-us/software/nvidia-app/').catch(err=>dialog.showErrorBox('Could not open NVIDIA website',String(err)))}
  ]},
  {label:'View',submenu:[
   {label:'Show Game Settings',click:()=>{if(mainWindow&&!mainWindow.isDestroyed()){mainWindow.show();mainWindow.focus();}}},
   {label:'Reload (use only if stuck)',accelerator:'Ctrl+R',click:()=>mainWindow?.webContents.reload()}
  ]}
 ]);
 mainWindow.setMenu(menu);
 // A successful game load clears the crash retry budget for future incidents.
 mainWindow.webContents.on('did-finish-load',()=>{rendererRecoveryAttempts=0;});
 mainWindow.webContents.on('render-process-gone',(_event,details)=>{
  if(!mainWindow||mainWindow.isDestroyed()||app.isQuitting)return;
  console.error('Moonfall renderer stopped:',details.reason);
  if(rendererRecoveryAttempts<MAX_RENDERER_RECOVERY_ATTEMPTS){
   rendererRecoveryAttempts++;
   // Reload from the same origin so saved settings and autosaves remain accessible.
   mainWindow.webContents.loadURL(GAME_ORIGIN+'/').catch(err=>{
    console.error('Moonfall recovery failed:',err);
    dialog.showErrorBox('Moonfall recovery failed',String(err));
   });
  }else{
   dialog.showErrorBox('Moonfall needs a restart',
    'The graphics process stopped repeatedly ('+details.reason+'). Close Moonfall and update your graphics driver before retrying.');
  }
 });
 mainWindow.webContents.setWindowOpenHandler(({url})=>{if(/^https:\/\//i.test(url))shell.openExternal(url).catch(()=>{});return {action:'deny'}});
 mainWindow.webContents.on('will-navigate',(event,target)=>{if(target!==url)event.preventDefault()});
 mainWindow.webContents.on('did-fail-load',(_e,code,desc,validatedURL,isMainFrame)=>{
  if(isMainFrame&&code!==-3)dialog.showErrorBox('Moonfall load error',`Could not load game (${code}): ${desc}\n${validatedURL}`);
 });
 await mainWindow.loadURL(url);
}
app.whenReady().then(()=>{if(singleInstance)return createWindow();}).catch(err=>{dialog.showErrorBox('Moonfall startup error',String(err));app.quit()});
app.on('window-all-closed',()=>app.quit());
app.on('before-quit',()=>{app.isQuitting=true;if(server)server.close()});
