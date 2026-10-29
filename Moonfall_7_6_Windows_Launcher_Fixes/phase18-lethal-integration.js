'use strict';
// Use the existing receiveDamage hook to recover before fatal HP reaches game-over logic.
// Phase 16's voluntary recovery remains available as a fallback.
function one(s,a,b,label){
 const i=s.indexOf(a);
 if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Phase18 hook mismatch: '+label);
 return s.slice(0,i)+b+s.slice(i+a.length);
}
function patchJs(s){
 const before='const receiveDamage=d=>{if(mfEvadeFrames>0)return;player.hp-=nyraShield>0?0:d*100/(100+armor());mfCombo=0;mfUpdateCombo()}';
 const after='const receiveDamage=d=>{if(mfEvadeFrames>0)return;if(mfP18TryLethalRecovery(d))return;player.hp-=nyraShield>0?0:d*100/(100+armor());mfCombo=0;mfUpdateCombo()}';
 s=one(s,before,after,'lethal damage');
 const runtime=`
function mfP18TryLethalRecovery(rawDamage){
 if(!playing||paused||!player||!mfWorld||mfDeathRecoveryUsed||
    nyraShield>0||!Number.isFinite(rawDamage)||rawDamage<=0||
    !Number.isFinite(player.hp)||player.hp<=0)return false;
 const defense=armor();
 if(!Number.isFinite(defense)||defense<=-100)return false;
 const actual=rawDamage*100/(100+defense);
 if(!Number.isFinite(actual)||actual<player.hp)return false;
 const cp=mfRecoveredCheckpoint();
 const state=mfDeathRecovery.deathRecovery(cp,mfWorld.region,stage,W,H,player.maxHp,false);
 if(!state)return false;
 mfDeathRecoveryUsed=true;
 player.x=state.x;player.y=state.y;player.hp=state.hp;
 mfCombo=0;mfUpdateCombo();
 const msg=document.getElementById('mfDeathStatus');
 if(msg)msg.textContent='Recovered from lethal damage at saved checkpoint';
 return true;
}
`;
 return one(s,'function loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}',
 runtime+'\nfunction loop(){pollController();tick();mfCheckpointCapture();draw();requestAnimationFrame(loop)}','runtime');
}
module.exports={patchJs};
