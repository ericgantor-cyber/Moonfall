// Original, stage-clear roguelike blessings. Never modifies hero art.
function once(s,a,b,label){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)>=0)throw Error('Boon hook mismatch: '+label);return s.slice(0,i)+b+s.slice(i+a.length)}
const logic=String.raw`const mfBoonPool=[
{name:'⚔️ Moonsteel Edge',description:'+25% attack damage',apply(){damageLevel++}},
{name:'❤️ Crimson Vitality',description:'+100 maximum HP and recover 30% HP',apply(){stats[0]=Math.min(20,stats[0]+1);player.max=maxHp();player.hp=Math.min(player.max,player.hp+player.max*.30)}},
{name:'⚡ Phantom Rhythm',description:'+0.1 attacks per second',apply(){stats[2]=Math.min(20,stats[2]+1)}},
{name:'🛡️ Eclipse Guard',description:'+10 armor',apply(){stats[3]=Math.min(20,stats[3]+1)}},
{name:'🩸 Shadow Drain',description:'+1% lifesteal',apply(){stats[4]=Math.min(20,stats[4]+1)}},
{name:'💚 Lunar Renewal',description:'+1 HP regeneration per second',apply(){stats[5]=Math.min(20,stats[5]+1)}},
{name:'💥 Starfall Precision',description:'+2% critical chance',apply(){stats[6]=Math.min(20,stats[6]+1)}},
{name:'🔥 Celestial Impact',description:'+10% critical damage',apply(){stats[7]=Math.min(20,stats[7]+1)}}
];
let mfBoonChoices=[],mfBoonRerolls=1;
function mfRollBoons(){
 const pool=mfBoonPool.map((_,i)=>i);
 for(let i=pool.length-1;i>0;i--){const j=Math.floor(Math.random()*(i+1));[pool[i],pool[j]]=[pool[j],pool[i]];}
 mfBoonChoices=pool.slice(0,3);mfDrawBoons();
}
function mfDrawBoons(){
 const root=document.getElementById('mfBoonChoices');root.classList.toggle('hidden',!upgradePending);
 if(!upgradePending){root.innerHTML='';return;}
 root.innerHTML='<p>Choose one blessing before the next battle.</p><div class="mf-boon-grid">'+mfBoonChoices.map((id,i)=>'<button class="mf-boon" data-boon="'+i+'"><b>'+mfBoonPool[id].name+'</b><span>'+mfBoonPool[id].description+'</span></button>').join('')+'</div><button id="mfBoonReroll" type="button" class="btn" '+(mfBoonRerolls?'':'disabled')+'>🎲 Reroll ('+mfBoonRerolls+' left)</button>';
}
function mfTakeBoon(slot){
 if(!upgradePending||!Number.isInteger(slot)||slot<0||slot>=mfBoonChoices.length)return;
 mfBoonPool[mfBoonChoices[slot]].apply();upgradePending=false;mfBoonChoices=[];mfDrawBoons();
 player.max=maxHp();player.hp=Math.min(player.max,player.hp+50);
 beginStage();playing=true;document.getElementById('overlay').classList.add('hidden');showLevelChoices();updateHUD();
}
document.getElementById('mfBoonChoices').addEventListener('click',event=>{
 const card=event.target.closest('[data-boon]');if(card){mfTakeBoon(Number(card.dataset.boon));return;}
 if(event.target.closest('#mfBoonReroll')&&mfBoonRerolls>0&&upgradePending){mfBoonRerolls--;mfRollBoons();}
});
`;
function patchJs(s){
 s=once(s,'function next(){',logic+'function next(){','boon logic');
 s=once(s,"document.getElementById('start').textContent='CONTINUE WITH +25% DAMAGE';document.getElementById('overlay').classList.remove('hidden')","document.getElementById('start').textContent='CHOOSE A BOON ABOVE';document.getElementById('overlay').classList.remove('hidden');mfRollBoons()",'stage clear');
 s=once(s,"if(upgradePending){upgradePending=false;damageLevel++;player.max=maxHp();player.hp=Math.min(player.max,player.hp+50);beginStage();playing=true;document.getElementById('overlay').classList.add('hidden');showLevelChoices()}else","if(upgradePending){return;}else",'prevent skipping choices');
 s=once(s,'function reset(){bossWarnings=[];','function reset(){mfBoonChoices=[];mfBoonRerolls=1;bossWarnings=[];','reset');
 return s;
}
function patchHtml(s){
 s=once(s,'<button class="btn" id="start">','<div id="mfBoonChoices" class="hidden" aria-label="Stage clear blessing choices"></div><button class="btn" id="start">','boon markup');
 const css='.mf-boon-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:9px;margin:12px 0}.mf-boon{background:#281238;color:#fff;border:2px solid #b760dd;border-radius:12px;padding:13px;min-height:110px;display:flex;flex-direction:column;gap:9px;align-items:center;justify-content:center;cursor:pointer;font:inherit}.mf-boon:hover,.mf-boon:focus-visible{border-color:#ffe59a;background:#45205d}.mf-boon span{font-size:13px}#mfBoonChoices{max-width:690px;max-height:50vh;overflow:auto}#mfBoonReroll{margin-bottom:10px}#mfBoonReroll:disabled{opacity:.35}@media(max-width:650px){.mf-boon-grid{grid-template-columns:1fr}.mf-boon{min-height:70px}}';
 return once(s,'</style>',css+'</style>','boon CSS');
}
module.exports={patchJs,patchHtml};
