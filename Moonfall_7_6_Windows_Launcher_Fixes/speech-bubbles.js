// Moonfall dialogue upgrade: small canvas overlays; original artwork is unchanged.
'use strict';
function replaceOnce(s, before, after, label) {
  const i = s.indexOf(before);
  if (i < 0 || s.indexOf(before, i + before.length) >= 0) {
    throw Error('Speech bubble hook mismatch: ' + label);
  }
  return s.slice(0, i) + after + s.slice(i + before.length);
}

// Using function source avoids nested-template quoting bugs in dialogue text.
function speechRuntime() {
  let mfSpeech = null;
  let mfLowHealthSpoken = false;
  let mfBossSpoken = new WeakSet();
  let mfBossPhaseSpoken = new WeakSet();
  let mfComboSpoken = false;
  let mfEnemyLineIndex = 0;
  let mfSpeechEnabled = true;
  try { mfSpeechEnabled = localStorage.getItem('moonfall-speech-enabled') !== 'false'; } catch (_) {}
  const mfHeroLines = {
    z: ['The shadows will not stop me.', 'Bring it on.', 'The moon guides my blade.'],
    nyra: ['The eclipse is mine.', 'Stay close to the moonlight.', 'You cannot outrun the night.']
  };
  const mfBossLines = ['You cannot escape the moon!', 'This realm belongs to me!', 'Witness my true power!'];
  const mfEnemyLines = ['You cannot hide!', 'The shadows hunger!', 'Surround them!'];
  const mfSpeechButton = document.getElementById('mfSpeechBtn');
  const mfSpeechAnnounce = document.getElementById('mfSpeechAnnounce');

  function mfSpeechRefreshButton() {
    mfSpeechButton.textContent = mfSpeechEnabled ? '💬 Dialogue: ON' : '💬 Dialogue: OFF';
    mfSpeechButton.setAttribute('aria-pressed', String(mfSpeechEnabled));
  }
  function mfSpeechToggle() {
    mfSpeechEnabled = !mfSpeechEnabled;
    if (!mfSpeechEnabled) mfSpeech = null;
    try { localStorage.setItem('moonfall-speech-enabled', String(mfSpeechEnabled)); } catch (_) {}
    mfSpeechRefreshButton();
    mfSpeechAnnounce.textContent = '';
  }
  mfSpeechButton.addEventListener('click', mfSpeechToggle);
  mfSpeechRefreshButton();

  function mfSay(target, text, life = 140, priority = 1, announce = false) {
    if (!mfSpeechEnabled || !playing || !target || !text) return false;
    if (mfSpeech && frame < mfSpeech.expires && priority <= mfSpeech.priority) return false;
    mfSpeech = {target, text, expires: frame + life, priority};
    if (announce) mfSpeechAnnounce.textContent = text;
    return true;
  }
  function mfSpeechStep() {
    if (!playing || paused || skillOpen || gearOpen || !mfSpeechEnabled) return;
    if (mfSpeech && frame >= mfSpeech.expires) mfSpeech = null;
    if (player.hp <= player.max * .27 && !mfLowHealthSpoken) {
      if (mfSay(player, 'My strength is fading!', 170, 4, true)) mfLowHealthSpoken = true;
    } else if (player.hp > player.max * .45) mfLowHealthSpoken = false;
    if (combatCombo < 3) mfComboSpoken = false;
    if (combatCombo >= 10 && !mfComboSpoken) {
      if (mfSay(player, selectedHero === 'nyra' ? 'Feel the eclipse!' : 'The shadows fall!', 105, 2)) mfComboSpoken = true;
    }
    if (frame % 30 !== 0) return;
    for (const enemy of enemies) {
      if (!enemy.boss || enemy.hp <= 0) continue;
      if (!mfBossSpoken.has(enemy)) {
        if (mfSay(enemy, mfBossLines[stage % mfBossLines.length], 155, 3, true)) mfBossSpoken.add(enemy);
        return;
      }
      if ((enemy.phaseTwo || enemy.mfPhaseTwo) && !mfBossPhaseSpoken.has(enemy)) {
        if (mfSay(enemy, 'This is not my final form!', 155, 3, true)) mfBossPhaseSpoken.add(enemy);
        return;
      }
    }
    if (frame % 600 === 0 && !mfSpeech) {
      for (const enemy of enemies) {
        if (enemy.hp > 0 && !enemy.boss) {
          mfSay(enemy, mfEnemyLines[(mfEnemyLineIndex++) % mfEnemyLines.length], 95, 1);
          break;
        }
      }
    }
  }
  function mfSpeechDraw() {
    if (!mfSpeechEnabled || !mfSpeech || !playing || frame >= mfSpeech.expires) return;
    const e = mfSpeech.target;
    if (!Number.isFinite(e.x) || !Number.isFinite(e.y) || (e !== player && e.hp <= 0)) return;
    ctx.save();
    ctx.font = 'bold 15px system-ui';
    const width = Math.min(320, Math.max(130, ctx.measureText(mfSpeech.text).width + 24));
    const x = Math.max(width / 2 + 9, Math.min(W - width / 2 - 9, e.x));
    const y = Math.max(40, Math.min(H - 50, e.y - (e.r || 22) - 34));
    ctx.fillStyle = 'rgba(14,8,28,.96)';
    ctx.strokeStyle = e === player ? '#e0afff' : e.boss ? '#ff95a9' : '#ffd596';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(x - width / 2, y - 27, width, 39, 10);
    ctx.fill();
    ctx.stroke();
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#ffffff';
    ctx.fillText(mfSpeech.text, x, y - 7, width - 15);
    ctx.restore();
  }
  function mfSpeechReset() {
    mfSpeech = null;
    mfLowHealthSpoken = false;
    mfBossSpoken = new WeakSet();
    mfBossPhaseSpoken = new WeakSet();
    mfComboSpoken = false;
    mfEnemyLineIndex = 0;
    mfSpeechAnnounce.textContent = '';
  }
  function mfSpeechNewStage() {
    mfSpeech = null;
    mfBossSpoken = new WeakSet();
    mfBossPhaseSpoken = new WeakSet();
    mfSay(player, mfHeroLines[selectedHero]?.[(stage - 1 + mfHeroLines.z.length) % mfHeroLines.z.length] || 'The battle begins!', 140, 2, true);
  }
}
function patchJs(s) {
  const code = speechRuntime.toString();
  const logic = code.slice(code.indexOf('{') + 1, code.lastIndexOf('}'));
  s = replaceOnce(s,
    'function loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
    logic + '\nfunction loop(){pollController();tick();draw();requestAnimationFrame(loop)}',
    'runtime');
  s = replaceOnce(s, 'function reset(){', 'function reset(){mfSpeechReset();', 'run reset');
  s = replaceOnce(s, 'function tick(){', 'function tick(){mfSpeechStep();', 'update');
  s = replaceOnce(s, 'function beginStage(){', 'function beginStage(){mfSpeechNewStage();', 'stage dialogue');
  s = replaceOnce(s,
    ' if(playing&&combatCombo>=2&&frame-comboLastHit<=165){',
    ' mfSpeechDraw();\n if(playing&&combatCombo>=2&&frame-comboLastHit<=165){',
    'bubble drawing');
  return s;
}
function patchHtml(s) {
  s = replaceOnce(s,
    '<button id="mfEffectsBtn" type="button" aria-pressed="false">✨ Effects: Full</button>',
    '<button id="mfEffectsBtn" type="button" aria-pressed="false">✨ Effects: Full</button><button id="mfSpeechBtn" type="button" aria-pressed="true">💬 Dialogue: ON</button><span id="mfSpeechAnnounce" class="mf-sr-only" aria-live="polite"></span>',
    'dialogue controls');
  return replaceOnce(s, '</style>', '.mf-sr-only{position:absolute!important;width:1px;height:1px;padding:0;margin:-1px;overflow:hidden;clip-path:inset(50%);white-space:nowrap;border:0}</style>', 'accessible text');
}
module.exports = {patchJs, patchHtml};
