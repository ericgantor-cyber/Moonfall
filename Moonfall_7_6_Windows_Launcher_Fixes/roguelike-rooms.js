// Original Moonfall room modifiers, elite minibosses, and permanent mastery.
// Does not change Z, Nyra, or embedded images.
function changeOnce(s,a,b,label){
 const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)!==-1)throw Error('Room integration point missing: '+label);
 return s.slice(0,i)+b+s.slice(i+a.length);
}
const logic=String.raw`const mfRoomTypes=[
 {name:'🩸 Bloodmoon Arena',color:'#ff466c',speed:1.17,cadence:1.2},
 {name:'🌑 Shadow Labyrinth',color:'#aa7bff',speed:1.05,cadence:1.0},
 {name:'🔥 Cinder Gauntlet',color:'#ffad59',speed:1.0,cadence:1.4}
];
let mfRoom=null,mfEliteSpawned=false,mfMastery=0,mfRunFinished=false;
const mfMasteryKey='moonfall-permanent-mastery-v1';
function mfReadMastery(){
 try{const n=Number(localStorage.getItem(mfMasteryKey));return Number.isFinite(n)?Math.max(0,Math.min(10,Math.floor(n))):0}catch{return 0}
}
function mfStoreMastery(n){try{localStorage.setItem(mfMasteryKey,String(n))}catch{}}
function mfNormalEnemyCount(){let count=0;for(const e of enemies)if(!e.mfElite)count++;return count}
function mfSetRoom(){
 mfEliteSpawned=false;
 mfRoom=![2,4,5,6,7,8].includes(stage)?mfRoomTypes[(stage+mfMastery)%mfRoomTypes.length]:null;
 const label=document.getElementById('mfRoomHud');
 if(label)label.textContent=mfRoom?mfRoom.name+' • ELITE AT '+Math.ceil(stages[stage].count*.5)+' KILLS':'⚔ BOSS CHAMBER';
}
function mfSpawnElite(){
 if(!mfRoom||mfEliteSpawned||killCount<Math.ceil(stages[stage].count*.5))return;
 mfEliteSpawned=true;
 const hp=1400+stage*350;
 enemies.push({x:W*.5,y:125,r:34,hp,max:hp,fire:0,type:0,mfElite:true});
 burst(W*.5,125,mfRoom.color,35);
}
function mfEliteAttack(e){
 if(!e.mfElite||e.hp<=0||frame%85!==0||shots.length>=240)return;
 const a=Math.atan2(player.y-e.y,player.x-e.x);
 let spawned=0;
 for(let i=-2;i<=2&&shots.length<240;i++){const dir=a+i*.21;
 shots.push({x:e.x,y:e.y,vx:Math.cos(dir)*4.2,vy:Math.sin(dir)*4.2,r:8,damage:13,friendly:false,color:mfRoom?.color||'#ff87ee'});spawned++}
 if(spawned)burst(e.x,e.y,mfRoom?.color||'#ff87ee',8);
}
function mfCompleteRun(){
 if(mfRunFinished)return;
 mfRunFinished=true;
 const next=Math.min(10,mfReadMastery()+1);
 mfStoreMastery(next);
 mfMastery=next;
}
`;
function patchJs(s){
 s=changeOnce(s,'function reset(){',logic+'function reset(){mfMastery=mfReadMastery();mfRunFinished=false;','run initialization');
 s=changeOnce(s,"const maxHp=()=>(val(0)+","const maxHp=()=>(mfMastery*40+val(0)+",'permanent vitality');
 s=changeOnce(s,'function beginStage(){bossWarnings=[];','function beginStage(){mfSetRoom();bossWarnings=[];','room setup');
 s=changeOnce(s,"if(![2,4,5,6,7,8].includes(stage)&&killCount+enemies.length<stages[stage].count&&frame%47===0)","if(![2,4,5,6,7,8].includes(stage)&&killCount+mfNormalEnemyCount()<stages[stage].count&&frame%Math.max(30,Math.round(47/(mfRoom?.cadence||1)))===0)",'spawn cadence');
 s=changeOnce(s,"let hp=[320,150,220][type]*(1+stage*.23);enemies.push(","let hp=[320,150,220][type]*(1+stage*.23);enemies.push(",'enemy hp');
 s=changeOnce(s,"let speed=[1.28,1.65,1.12][e.type??1];e.x+=Math.cos(a)*speed;","let speed=[1.28,1.65,1.12][e.type??1]*(mfRoom?.speed||1);mfEliteAttack(e);e.x+=Math.cos(a)*speed;",'enemy variety');
 s=changeOnce(s,"if(!e.boss){killCount++;player.hp=Math.min(player.max,player.hp+2)}","if(!e.boss){if(!e.mfElite)killCount++;else{souls+=60;gainXP(70)}mfSpawnElite();player.hp=Math.min(player.max,player.hp+2)}",'elite progression');
 s=changeOnce(s,"(!e.root&&![2,4,5,6,7,8].includes(stage)&&killCount>=stages[stage].count))next()","(!e.root&&![2,4,5,6,7,8].includes(stage)&&killCount>=stages[stage].count&&!enemies.some(z=>z.mfElite)))next()",'room exit lock');
 s=changeOnce(s,"if(title.includes('FALLEN')||title.includes('SAVED')){","if(title.includes('FALLEN')||title.includes('SAVED')){mfCompleteRun();msg+=' 🌙 Permanent mastery '+mfMastery+'/10: +'+(mfMastery*40)+' max HP next run.';",'permanent rewards');
 s=changeOnce(s,"document.getElementById('bossLabel').textContent='Stage '","document.getElementById('bossLabel').textContent='Stage '",'HUD keep');
 return s;
}
function patchHtml(s){
 return changeOnce(s,'<b id="status">Souls: 0</b>','<b id="status">Souls: 0</b><b id="mfRoomHud" style="color:#ffdaac">⚔ ROOM</b>','room HUD');
}
module.exports={patchJs,patchHtml};
