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
const bossMarker='function bossRandomAttack(e){';
if(!js.includes(bossMarker))throw new Error('Boss attack hook missing from game source');
js=js.replace(bossMarker,bossMarker+`\n const phaseShift=window.mfBossPhaseUpdate(e,stage);\n if(phaseShift){burst(e.x,e.y,phaseShift.phase===2?'#ff375a':'#d88bff',phaseShift.phase===2?50:28);e.nextRandomAttack=frame+40;}\n`);
js=js.replace('const enraged=e.phaseTwo===true;const kind=', 'const enraged=e.phaseTwo===true;const kind=');
js=js.replace('*(enraged?1.45:1);','*(e.phaseThree?1.9:enraged?1.45:1);');
const html=source.replace(match[0],'<script src="boss-phases-browser.js" defer></script><script src="game.js" defer></script>');
fs.writeFileSync(path.join(out,'game.html'),html);
fs.writeFileSync(path.join(out,'game.js'),js);
console.log('Moonfall modular build:',count,'original image assets extracted');
