'use strict';
const assert = require('node:assert/strict');
const vm = require('node:vm');
const speech = require('./speech-bubbles.js');

const fixture = 'function reset(){} function tick(){} function beginStage(){} function loop(){pollController();tick();draw();requestAnimationFrame(loop)} if(playing&&combatCombo>=2&&frame-comboLastHit<=165){}';
const patched = speech.patchJs(fixture);
new vm.Script(patched); // Catch bad quotes and mismatched braces in generated JS.
assert.match(patched, /The shadows will not stop me/);
assert.match(speech.patchHtml('<style></style><button id="mfEffectsBtn" type="button" aria-pressed="false">✨ Effects: Full</button>'), /id="mfSpeechBtn"/);
assert.throws(() => speech.patchJs(fixture + 'function reset(){}'), /hook mismatch/);

const control = {
  textContent: '', attributes: {},
  addEventListener(_type, callback) {this.click = callback;},
  setAttribute(name, value) {this.attributes[name] = value;}
};
const announce = {textContent: ''};
const store = new Map();
const player = {x:400,y:380,r:22,hp:100,max:100};
const ctx = {
  save() {}, restore() {}, beginPath() {}, roundRect() {}, fill() {}, stroke() {}, fillText() {},
  measureText(text) {return {width: text.length * 8};}
};
const env = {frame:0,playing:true,paused:false,skillOpen:false,gearOpen:false,
  player, enemies:[], selectedHero:'z',stage:1,combatCombo:0,ctx,W:1120,H:630,
  document:{getElementById(id) {return id==='mfSpeechBtn'?control:id==='mfSpeechAnnounce'?announce:null;}},
  localStorage:{getItem(key){return store.get(key)??null;},setItem(key,value){store.set(key,value);}}
};
const context = vm.createContext(env);
const runtimeStart = patched.indexOf('let mfSpeech = null;');
const runtimeEnd = patched.indexOf('function loop(){pollController();tick();draw();requestAnimationFrame(loop)}', runtimeStart);
assert(runtimeStart >= 0 && runtimeEnd > runtimeStart);
vm.runInContext(patched.slice(runtimeStart, runtimeEnd), context);
const run = statement => vm.runInContext(statement,context);

run('mfSpeechNewStage()');
assert.match(announce.textContent, /shadows/i);
assert.equal(control.attributes['aria-pressed'],'true');
run('mfSpeechDraw()');

player.hp=20;env.frame=1;
run('mfSpeechStep()');
assert.equal(announce.textContent,'My strength is fading!');
assert.equal(run('mfSpeech.priority'),4);
assert.equal(run('mfSay(player,"lower priority",20,1)'),false);

player.hp=90;env.frame=500;
run('mfSpeechStep()');
assert.equal(run('mfLowHealthSpoken'),false);
const boss={boss:true,hp:100,max:100,x:200,y:150,r:45};env.enemies.push(boss);env.frame=510;
run('mfSpeechStep()');
assert.equal(run('mfBossSpoken.has(enemies[0])'),true);
assert.equal(run('mfSpeech.priority'),3);
boss.phaseTwo=true;env.frame=690;
run('mfSpeechStep()');
assert.equal(run('mfBossPhaseSpoken.has(enemies[0])'),true);
assert.equal(announce.textContent,'This is not my final form!');

control.click();
assert.equal(store.get('moonfall-speech-enabled'),'false');
assert.equal(control.attributes['aria-pressed'],'false');
assert.equal(run('mfSpeech'),null);
assert.equal(run('mfSay(player,"should not show")'),false);
control.click();
assert.equal(store.get('moonfall-speech-enabled'),'true');
run('mfSpeechReset()');
assert.equal(run('mfBossSpoken.has(enemies[0])'),false);
console.log('PASS: dialogue syntax, controls, warnings, priorities, boss phases, rendering and reset');
