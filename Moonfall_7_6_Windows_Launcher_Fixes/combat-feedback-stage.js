// Lightweight, accessible combo feedback. Never touches hero artwork.
'use strict';
function one(s,a,b,n){const i=s.indexOf(a);if(i<0||s.indexOf(a,i+a.length)>=0)throw Error('Combat feedback hook mismatch: '+n);return s.slice(0,i)+b+s.slice(i+a.length)}
const logic=[
'let mfToastTicks=0;',
'let mfReducedEffects=(()=>{try{return localStorage.getItem("moonfall-reduced-effects")==="true" || (localStorage.getItem("moonfall-reduced-effects")===null && window.matchMedia("(prefers-reduced-motion: reduce)").matches)}catch{return false}})();',
'function mfShowMilestone(){',
' if(mfCombo!==3&&mfCombo!==5)return;',
' const el=document.getElementById("mfCombatToast");',
' if(!el)return;el.textContent=mfCombo===5?"🔥 FIVE-HIT RHYTHM":"⚔ TRIPLE STRIKE";',
' if(mfReducedEffects){el.classList.add("hidden");return;}',
' el.classList.remove("hidden");mfToastTicks=48;',
'}',
'function mfToastStep(){',
' if(mfToastTicks>0&&--mfToastTicks===0){document.getElementById("mfCombatToast").classList.add("hidden");}',
'}',
'function mfToggleEffects(){',
' mfReducedEffects=!mfReducedEffects;',
' try{localStorage.setItem("moonfall-reduced-effects",String(mfReducedEffects));}catch{}',
' const b=document.getElementById("mfEffectsBtn");',
' b.textContent=mfReducedEffects?"✨ Effects: Reduced":"✨ Effects: Full";',
' b.setAttribute("aria-pressed",String(mfReducedEffects));',
' if(mfReducedEffects){document.getElementById("mfCombatToast").classList.add("hidden");mfToastTicks=0;}',
'}'
].join('\n');
function patchJs(s){
 s=one(s,'function mfUpdateCombo(){',logic+'\nfunction mfUpdateCombo(){','visual feedback runtime');
 s=one(s,'mfLastAttack=frame;mfUpdateCombo();burst(',
 'mfLastAttack=frame;mfUpdateCombo();mfShowMilestone();burst(','combo milestones');
 s=one(s,'function tick(){','function tick(){mfToastStep();','toast timer');
 s=one(s,"document.getElementById('mfEvadeBtn').addEventListener('click',mfEvade);",
 "document.getElementById('mfEvadeBtn').addEventListener('click',mfEvade);document.getElementById('mfEffectsBtn').addEventListener('click',mfToggleEffects);document.getElementById('mfEffectsBtn').textContent=mfReducedEffects?'✨ Effects: Reduced':'✨ Effects: Full';document.getElementById('mfEffectsBtn').setAttribute('aria-pressed',String(mfReducedEffects));",
 'accessible effects toggle');
 return s;
}
function patchHtml(s){
 s=one(s,'<button id="mfEvadeBtn">✨ EVADE (SHIFT)</button>',
 '<button id="mfEvadeBtn">✨ EVADE (SHIFT)</button><button id="mfEffectsBtn" type="button" aria-pressed="false">✨ Effects: Full</button><div id="mfCombatToast" class="hidden" aria-live="off"></div>',
 'effects button');
 const css='#mfCombatToast{position:fixed;left:50%;top:24%;transform:translateX(-50%);pointer-events:none;z-index:25;font-weight:900;letter-spacing:2px;font-size:23px;color:#ffdb86;text-shadow:0 0 18px #bb70ff,0 2px 5px #150516}@media(prefers-reduced-motion:reduce){#mfCombatToast{display:none}}';
 return one(s,'</style>',css+'</style>','toast styles');
}
module.exports={patchJs,patchHtml};
