// Automatically offer three upgrades for every earned level, without touching art.
function once(s,find,replacement,label){
  const i=s.indexOf(find);
  if(i<0||s.indexOf(find,i+find.length)!==-1)throw Error("Upgrade hook mismatch: "+label);
  return s.slice(0,i)+replacement+s.slice(i+find.length);
}
function patchJs(s){
 s=once(s,"skillOpen=false,stats=","skillOpen=false,pendingLevelChoices=0,activeLevelCards=[],stats=","state");
 s=once(s,"level++;skillPoints+=3;burst(","level++;skillPoints+=3;pendingLevelChoices++;burst(","XP");
 s=once(s,"renderSkills()}const spellNames=","renderSkills();if(pendingLevelChoices>0)showLevelChoices()}const spellNames=","XP scheduling");
 const logic=String.raw`function renderLevelCards(){
 const node=document.getElementById('levelCards');if(!node)return;
 if(pendingLevelChoices===0){node.classList.add('hidden');node.innerHTML='';return;}
 node.classList.remove('hidden');
 node.innerHTML='<h3>🌙 LEVEL '+level+' • CHOOSE ONE UPGRADE</h3><p>Choose one card to continue. '+pendingLevelChoices+' selection(s) remaining.</p><div class="level-card-grid">'+activeLevelCards.map(i=>'<button type="button" class="level-card" data-level-card="'+i+'"><b>'+statNames[i]+'</b><span>Rank '+stats[i]+' → '+(stats[i]+1)+'</span><span>+'+increments[i]+' upgrade</span></button>').join('')+'</div>';
}
function showLevelChoices(){
 if(pendingLevelChoices<1)return;
 if(!skillOpen)toggleSkills(true);
 if(activeLevelCards.length)return renderLevelCards();
 const pool=stats.map((r,i)=>r<20?i:-1).filter(i=>i>=0);
 if(!pool.length){pendingLevelChoices=0;toggleSkills(false);return;}
 for(let i=pool.length-1;i>0;i--){const j=Math.floor(Math.random()*(i+1));[pool[i],pool[j]]=[pool[j],pool[i]];}
 activeLevelCards=pool.slice(0,3);renderLevelCards();
}
function chooseLevelCard(i){
 if(pendingLevelChoices<1||!activeLevelCards.includes(i)||skillPoints<1||stats[i]>=20)return;
 pendingLevelChoices--;activeLevelCards=[];upgrade(i);
 if(pendingLevelChoices>0)showLevelChoices();else{renderLevelCards();toggleSkills(false);}
}
`;
 s=once(s,"function toggleSkills(force){",logic+"function toggleSkills(force){if(pendingLevelChoices>0&&skillOpen&&(force===false||force===undefined))return;","modal");
 s=once(s,"function reset(){bossWarnings=[];","function reset(){pendingLevelChoices=0;activeLevelCards=[];renderLevelCards();bossWarnings=[];","reset");
 s=once(s,"document.getElementById('skillgrid').addEventListener('click'","document.getElementById('levelCards').addEventListener('click',e=>{const b=e.target.closest('[data-level-card]');if(b)chooseLevelCard(Number(b.dataset.levelCard));});document.getElementById('skillgrid').addEventListener('click'","card click");
 s=once(s,"playing=true;document.getElementById('overlay').classList.add('hidden')}else","playing=true;document.getElementById('overlay').classList.add('hidden');showLevelChoices()}else","stage continue");
 return s;
}
function patchHtml(s){
 s=once(s,'<p id="pointsInfo">Skill points: 0</p>','<p id="pointsInfo">Skill points: 0</p><div id="levelCards" class="hidden" aria-live="polite"></div>','card area');
 const css=String.raw`#levelCards{border:2px solid #e5adff;background:#210d35;border-radius:14px;padding:15px;margin-bottom:18px;text-align:center}#levelCards h3{color:#ffe49a;margin:0 0 8px}.level-card-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px}.level-card{border:2px solid #c679f7;background:linear-gradient(145deg,#3a1756,#251036);color:#fff;border-radius:12px;min-height:125px;cursor:pointer;padding:12px;font:inherit;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:10px}.level-card:hover,.level-card:focus-visible{border-color:#ffe49a;box-shadow:0 0 14px #a955e5}.level-card span{font-size:13px}@media(max-width:650px){.level-card-grid{grid-template-columns:1fr}.level-card{min-height:80px}}`;
 return once(s,'</style>',css+'</style>','card CSS');
}
module.exports={patchJs,patchHtml};
