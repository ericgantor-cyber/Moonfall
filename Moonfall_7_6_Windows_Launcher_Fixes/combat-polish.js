// Fast, original action-roguelike combat polish. No artwork modifications.
function replaceOnce(s,a,b,label){
 const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)>=0)throw Error("Combat hook mismatch: "+label);
 return s.slice(0,i)+b+s.slice(i+a.length);
}
function patchJs(s){
 const patch=(a,b,label)=>{s=replaceOnce(s,a,b,label)};
 patch('const receiveDamage=d=>{player.hp-=nyraShield>0?0:d*100/(100+armor())};','const receiveDamage=d=>{if(mfEvadeFrames>0)return;player.hp-=nyraShield>0?0:d*100/(100+armor());mfCombo=0;mfUpdateCombo()};','damage');
 patch('attackTimer=Math.max(2,Math.round(60/atkRate()));burst(player.x,player.y,selectedHero',
 'attackTimer=Math.max(2,Math.round(60/atkRate()*.78));mfCombo=(frame-mfLastAttack<100)?Math.min(5,mfCombo+1):1;mfLastAttack=frame;mfUpdateCombo();burst(player.x,player.y,selectedHero','attack rhythm');
 patch('if(mouseAttackHeld||padAttack)shoot();frame++;','if(mouseAttackHeld||padAttack)shoot();frame++;if(mfEvadeFrames>0)mfEvadeFrames--;if(mfEvadeCooldown>0)mfEvadeCooldown--;if(mfCombo>0&&frame-mfLastAttack>150){mfCombo=0;mfUpdateCombo()}','tick');
 patch('function reset(){pendingLevelChoices=0;','function reset(){mfEvadeFrames=0;mfEvadeCooldown=0;mfCombo=0;mfLastAttack=-1000;mfUpdateCombo();pendingLevelChoices=0;','reset');
 patch("if(e.key.toLowerCase()==='e')dash();","if(e.key.toLowerCase()==='e')dash();if(e.key==='Shift'&&!e.repeat)mfEvade();",'input');
 // Nearest-target selection in linear time; avoid allocating and sorting all enemies for every shot.
 patch('let targets=enemies.filter(e=>e.hp>0);if(!targets.length)return;targets.sort((a,b)=>Math.hypot(a.x-player.x,a.y-player.y)-Math.hypot(b.x-player.x,b.y-player.y));let t=targets[0],a=',
 'let t=null,closest=Infinity;for(const enemy of enemies){if(enemy.hp<=0)continue;const dx=enemy.x-player.x,dy=enemy.y-player.y,d=dx*dx+dy*dy;if(d<closest){closest=d;t=enemy}}if(!t)return;let a=', 'target selection');
 patch('function draw(){paintBackground();',"function draw(){paintBackground();if(mfEvadeFrames>0){ctx.save();ctx.strokeStyle=selectedHero==='nyra'?'#fff099':'#bc78ff';ctx.lineWidth=5;ctx.shadowBlur=24;ctx.shadowColor=ctx.strokeStyle;ctx.beginPath();ctx.arc(player.x,player.y,player.r+12+Math.sin(frame*.5)*3,0,Math.PI*2);ctx.stroke();ctx.restore();}",'evade drawing');
 const logic=`let mfEvadeFrames=0,mfEvadeCooldown=0,mfCombo=0,mfLastAttack=-1000;
function mfUpdateCombo(){const el=document.getElementById('mfComboHud');if(el)el.textContent=mfCombo>1?'⚔ COMBO x'+mfCombo:'⚔ COMBO';}
function mfEvade(){
 if(!playing||paused||skillOpen||gearOpen||mfEvadeCooldown>0)return;
 let dx=(keys.ArrowRight||keys.d||padMove.right?1:0)-(keys.ArrowLeft||keys.a||padMove.left?1:0);
 let dy=(keys.ArrowDown||keys.s||padMove.down?1:0)-(keys.ArrowUp||keys.w||padMove.up?1:0);
 if(!dx&&!dy)dy=-1;
 const len=Math.hypot(dx,dy),oldX=player.x,oldY=player.y;
 player.x=Math.max(30,Math.min(W-30,player.x+dx/len*115));
 player.y=Math.max(70,Math.min(H-30,player.y+dy/len*115));
 mfEvadeFrames=17;mfEvadeCooldown=72;
 burst(oldX,oldY,selectedHero==='nyra'?'#ffe58f':'#9c64ff',18);
 burst(player.x,player.y,selectedHero==='nyra'?'#ffe58f':'#9c64ff',18);
}
`;
 patch('function dash(){',logic+'function dash(){','evade logic');
 patch("document.querySelectorAll('[data-key]').forEach(b=>","document.getElementById('mfEvadeBtn').addEventListener('click',mfEvade);document.querySelectorAll('[data-key]').forEach(b=>",'evade button');
 return s;
}
function patchHtml(s){
 s=replaceOnce(s,'<b id="status">Souls: 0</b>','<b id="status">Souls: 0</b><b id="mfComboHud" aria-live="off" style="color:#ffca8a">⚔ COMBO</b>','HUD');
 return replaceOnce(s,'<button id="dash">💨 DASH (E)</button>','<button id="dash">💨 DASH (E)</button><button id="mfEvadeBtn">✨ EVADE (SHIFT)</button>','button');
}
module.exports={patchJs,patchHtml};
