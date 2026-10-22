// Generates a modular game from the checked-in standalone HTML without altering original art.
const fs=require('node:fs'),path=require('node:path');
const root=__dirname,source=fs.readFileSync(path.join(root,'game.html'),'utf8');
const match=source.match(/<script>([\s\S]*?)<\/script>/);
if(!match)throw new Error('Moonfall inline game script not found');
const out=path.join(root,'modular'),assets=path.join(out,'assets');
fs.mkdirSync(assets,{recursive:true});
let count=0;
let js=match[1].replace(/data:image\/(webp|png|jpeg);base64,([A-Za-z0-9+/=]+)/g,(_,ext,data)=>{
 const name='art-'+String(++count).padStart(3,'0')+'.'+ext;
 fs.writeFileSync(path.join(assets,name),Buffer.from(data,'base64'));
 return 'assets/'+name;
});
// Attach the tested boss controller to the actual combat loop at build time.
const phaseModule=fs.readFileSync(path.join(root,'boss-phases.js'),'utf8');
const browserModule=phaseModule.replace('module.exports={BOSS_PROFILES,createBossPhaseState,updateBossPhase,acknowledgeBossRewards};',
 'globalThis.MoonfallBossPhases={BOSS_PROFILES,createBossPhaseState,updateBossPhase,acknowledgeBossRewards};');
if(browserModule===phaseModule)throw new Error('Boss controller export changed');
fs.writeFileSync(path.join(out,'boss-phases.browser.js'),browserModule);
const attackHook='function bossRandomAttack(e){';
if(!js.includes(attackHook))throw new Error('Boss attack integration point missing');
const integration=`\n if(!e.mfBossPhase){
  const label=(stages[stage]?.name||'').toLowerCase();
  const kind=label.includes('dragon')?'dragon':label.includes('phoenix')?'phoenix':label.includes('scream')?'screamer':'default';
  e.mfBossPhase=MoonfallBossPhases.createBossPhaseState(kind,e.max);
 }
 const phaseEvent=MoonfallBossPhases.updateBossPhase(e.mfBossPhase,e.hp);
 e.phaseTwo=phaseEvent.phase>=1;
 e.mfFinalPhase=phaseEvent.phase>=2;
 if(phaseEvent.phaseChanged){
  burst(e.x,e.y,e.mfFinalPhase?'#ff3266':'#ffd166',38);
  e.lastRandomMove=e.mfFinalPhase?'FINAL PHASE':'ENRAGED';
  e.moveLabelTimer=150;
  e.nextRandomAttack=frame+35;
 }
`;
js=js.replace(attackHook,attackHook+integration);
// Phase three uses the existing projectile system with a faster, denser pattern.
js=js.replace('const enraged=e.phaseTwo===true;const kind=',
 'const enraged=e.phaseTwo===true;const kind=');
js=js.replace('const speed=(3.2+stage*.19)*diff;',
 'const speed=(3.2+stage*.19)*diff*(e.mfFinalPhase?1.25:1);');
js=js.replace('e.nextRandomAttack=frame+(enraged?58:105)+Math.floor(Math.random()*(enraged?65:115));',
 'e.nextRandomAttack=frame+(e.mfFinalPhase?35:enraged?58:105)+Math.floor(Math.random()*(e.mfFinalPhase?45:enraged?65:115));');
const html=source.replace(match[0],'<script src="boss-phases.browser.js" defer></script><script src="game.js" defer></script>');
fs.writeFileSync(path.join(out,'game.html'),html);
fs.writeFileSync(path.join(out,'game.js'),js);
console.log('Moonfall modular build:',count,'original image assets extracted');
