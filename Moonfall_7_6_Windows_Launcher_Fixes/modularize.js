// Generates a modular game from the checked-in standalone HTML without altering original art.
const fs=require('node:fs'),path=require('node:path');
const root=__dirname,source=fs.readFileSync(path.join(root,'game.html'),'utf8');
const match=source.match(/<script>([\s\S]*?)<\/script>/);
if(!match)throw new Error('Moonfall inline game script not found');
const out=path.join(root,'modular'),assets=path.join(out,'assets');
fs.mkdirSync(assets,{recursive:true});
let count=0;
const patch=require('./auto-level-cards.js');
const combat=require('./combat-polish.js');
const boons=require('./roguelike-boons.js');
const rooms=require('./roguelike-rooms.js');
const js=rooms.patchJs(boons.patchJs(combat.patchJs(patch.patchJs(match[1])))).replace(/data:image\/(webp|png|jpeg);base64,([A-Za-z0-9+/=]+)/g,(_,ext,data)=>{
 const name='art-'+String(++count).padStart(3,'0')+'.'+ext;
 fs.writeFileSync(path.join(assets,name),Buffer.from(data,'base64'));
 return 'assets/'+name;
});
const html=rooms.patchHtml(boons.patchHtml(combat.patchHtml(patch.patchHtml(source)))).replace(match[0],'<script src="game.js" defer></script>');
fs.writeFileSync(path.join(out,'game.html'),html);
fs.writeFileSync(path.join(out,'game.js'),js);
console.log('Moonfall modular build:',count,'original image assets extracted');
