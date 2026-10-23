// Persistent Sanctuary skill shop. Original artwork and 11,111 mob catalog untouched.
'use strict';
function one(s,a,b,n){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)>=0)throw Error('Sanctuary hook mismatch: '+n);return s.slice(0,i)+b+s.slice(i+a.length)}
const runtime=[
'const mfSanctuaryKey="moonfall-sanctuary-v1";',
'const mfSanctuaryTypes=[["vitality","❤️ Health","+30 max HP"],["haste","⚡ Rhythm","+5% attack speed"],["evasion","✨ Agility","Faster evade recovery"]];',
'function mfSanctuaryRead(){',
' try{const s=JSON.parse(localStorage.getItem(mfSanctuaryKey)||"{}");',
'  const num=v=>Number.isInteger(v)?Math.max(0,Math.min(1000,v)):0;',
'  return {shards:num(s.shards),vitality:Math.min(5,num(s.vitality)),haste:Math.min(5,num(s.haste)),evasion:Math.min(5,num(s.evasion))};',
' }catch{return {shards:0,vitality:0,haste:0,evasion:0}}',
'}',
'let mfSanctuary=mfSanctuaryRead();',
'function mfSanctuaryWrite(){try{localStorage.setItem(mfSanctuaryKey,JSON.stringify(mfSanctuary))}catch{}}',
'function mfSanctuaryEarn(n){mfSanctuary.shards=Math.min(1000,mfSanctuary.shards+n);mfSanctuaryWrite();mfSanctuaryDraw();}',
'function mfSanctuaryBuy(type){',
' if(playing||upgradePending||!mfSanctuaryTypes.some(x=>x[0]===type))return;',
' const lvl=mfSanctuary[type],cost=lvl+1;',
' if(lvl>=5||mfSanctuary.shards<cost)return;',
' mfSanctuary.shards-=cost;mfSanctuary[type]++;mfSanctuaryWrite();mfSanctuaryDraw();',
'}',
'function mfSanctuaryDraw(){',
' const el=document.getElementById("mfSanctuary");if(!el)return;',
' el.classList.toggle("hidden",Boolean(playing||upgradePending));',
' el.innerHTML="<h3>🌙 Moonfall Sanctuary • "+mfSanctuary.shards+" shards</h3><p>Complete a run to earn 2 shards. Buy permanent upgrades between runs.</p>"+',
' mfSanctuaryTypes.map(([id,name,bonus])=>{const lvl=mfSanctuary[id],cost=lvl+1;',
' return "<button type=\\"button\\" data-sanctuary=\\""+id+"\\" "+(lvl>=5||mfSanctuary.shards<cost?"disabled":"")+">"+name+" ("+lvl+"/5) • "+bonus+" • "+(lvl>=5?"MAX":cost+" shards")+"</button>";}).join("");',
'}',
'document.getElementById("mfSanctuary").addEventListener("click",e=>{',
' const item=e.target.closest("[data-sanctuary]");if(item)mfSanctuaryBuy(item.dataset.sanctuary);',
'});'
].join('\n');
function patchJs(s){
 s=one(s,'function mfCompleteRun(){',runtime+'\nfunction mfCompleteRun(){','shop runtime');
 s=one(s,'const maxHp=()=>(mfMastery*40+val(0)+',
 'const maxHp=()=>(mfSanctuary.vitality*30+mfMastery*40+val(0)+','permanent HP');
 s=one(s,'attackTimer=Math.max(2,Math.round(60/atkRate()*.78));',
 'attackTimer=Math.max(2,Math.round(60/(atkRate()*(1+mfSanctuary.haste*.05))*.78));','permanent attack speed');
 s=one(s,'mfEvadeFrames=17;mfEvadeCooldown=72;',
 'mfEvadeFrames=17;mfEvadeCooldown=Math.max(40,72-mfSanctuary.evasion*5);','evade mastery');
 s=one(s,"if(title.includes('FALLEN')||title.includes('SAVED')){mfCompleteRun();",
 "if(title.includes('FALLEN')||title.includes('SAVED')){if(title.includes('SAVED')){mfCompleteRun();mfSanctuaryEarn(2);}",'earn on win');
 s=one(s,'function reset(){','function reset(){mfSanctuaryDraw();','sanctuary refresh');
 s+='\nmfSanctuaryDraw();\n';
 return s;
}
function patchHtml(s){
 s=one(s,'<div id="mfBoonChoices" class="hidden"',
 '<div id="mfSanctuary" class="hidden" aria-label="Permanent upgrades between runs"></div><div id="mfBoonChoices" class="hidden"','sanctuary panel');
 const css='#mfSanctuary{margin:10px auto;padding:10px;max-width:630px;max-height:48vh;overflow:auto;border:2px solid #9161af;border-radius:12px;background:#21102c}#mfSanctuary h3{color:#ffe394;margin:0 0 6px}#mfSanctuary p{font-size:13px}#mfSanctuary button{display:block;width:100%;margin:5px 0;padding:9px;background:#3c2054;color:#fff;border:1px solid #c18fe1;border-radius:7px;font:inherit;cursor:pointer}#mfSanctuary button:disabled{opacity:.45;cursor:default}';
 return one(s,'</style>',css+'</style>','sanctuary styles');
}
module.exports={patchJs,patchHtml};
