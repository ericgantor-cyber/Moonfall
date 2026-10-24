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
  let mfSpeechFocus = false;
  try {
    mfSpeechEnabled = localStorage.getItem('moonfall-speech-enabled') !== 'false';
    mfSpeechFocus = localStorage.getItem('moonfall-speech-focus') === 'true';
  } catch (_) {}
  const mfHeroLines = {
    z: ['The shadows will not stop me.', 'Bring it on.', 'The moon guides my blade.', 'Another shadow falls.'],
    nyra: ['The eclipse is mine.', 'Stay close to the moonlight.', 'You cannot outrun the night.', 'Stars, answer my call.']
  };
  const mfBossLines = ['You cannot escape the moon!', 'This realm belongs to me!', 'Witness my true power!'];
  const mfEnemyLines = ['You cannot hide!', 'The shadows hunger!', 'Surround them!', 'The night is ours!', 'They are weakening!'];
  const mfSpeechButton = document.getElementById('mfSpeechBtn');
  const mfSpeechFocusButton = document.getElementById('mfSpeechFocusBtn');
  const mfSpeechAnnounce = document.getElementById('mfSpeechAnnounce');

  function mfSpeechRefreshButton() {
    mfSpeechButton.textContent = mfSpeechEnabled ? '💬 Dialogue: ON' : '💬 Dialogue: OFF';
    mfSpeechButton.setAttribute('aria-pressed', String(mfSpeechEnabled));
    mfSpeechFocusButton.textContent = mfSpeechFocus ? '🎯 Important lines only' : '💬 All dialogue';
    mfSpeechFocusButton.setAttribute('aria-pressed', String(mfSpeechFocus));
  }
  function mfSpeechToggle() {
    mfSpeechEnabled = !mfSpeechEnabled;
    if (!mfSpeechEnabled) mfSpeech = null;
    try { localStorage.setItem('moonfall-speech-enabled', String(mfSpeechEnabled)); } catch (_) {}
    mfSpeechRefreshButton();
    mfSpeechAnnounce.textContent = '';
  }
  function mfSpeechToggleFocus() {
    mfSpeechFocus = !mfSpeechFocus;
    if (mfSpeechFocus && mfSpeech && mfSpeech.priority < 3) mfSpeech = null;
    try { localStorage.setItem('moonfall-speech-focus', String(mfSpeechFocus)); } catch (_) {}
    mfSpeechRefreshButton();
  }
  mfSpeechButton.addEventListener('click', mfSpeechToggle);
  mfSpeechFocusButton.addEventListener('click', mfSpeechToggleFocus);
  mfSpeechRefreshButton();

  function mfSay(target, text, life = 140, priority = 1, announce = false) {
    if (!mfSpeechEnabled || !playing || !target || !text || (mfSpeechFocus && priority < 3)) return false;
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
    if (frame % 600 === 0 && !mfSpeech && !enemies.some(e => e.boss && e.hp > 0)) {
      for (const enemy of enemies) {
        if (enemy.hp > 0 && !enemy.boss) {
          mfSay(enemy, mfEnemyLines[(mfEnemyLineIndex++) % mfEnemyLines.length], 95, 1);
          break;
        }
      }
    }
  }
  function mfSpeechDraw() {
    if (!mfSpeechEnabled || !mfSpeech || !playing || paused || skillOpen || gearOpen || frame >= mfSpeech.expires) return;
    const e = mfSpeech.target;
    if (!Number.isFinite(e.x) || !Number.isFinite(e.y) || (e !== player && e.hp <= 0)) return;
    const label = e === player ? (selectedHero === 'nyra' ? 'NYRA' : 'Z') : e.boss ? 'BOSS' : 'ENEMY';
    const maxWidth = Math.min(310, W - 24);
    const fontSize = Math.max(12, Math.min(15, W / 55));
    const padding = 13;
    ctx.save();
    ctx.font = 'bold ' + fontSize + 'px system-ui';
    const words = mfSpeech.text.split(' ');
    const lines = [''];
    for (const word of words) {
      const current = lines[lines.length - 1];
      const trial = current ? current + ' ' + word : word;
      if (current && ctx.measureText(trial).width > maxWidth - padding * 2 && lines.length < 3) lines.push(word);
      else lines[lines.length - 1] = trial;
    }
    const width = Math.min(maxWidth, Math.max(120, ...lines.map(line => ctx.measureText(line).width + padding * 2)));
    const height = 33 + lines.length * (fontSize + 4);
    const x = Math.max(width / 2 + 5, Math.min(W - width / 2 - 5, e.x));
    const y = Math.max(height + 10, Math.min(H - 45, e.y - (e.r || 22) - 13));
    const opacity = Math.min(1, Math.max(0, (mfSpeech.expires - frame) / 16));
    if (!(typeof mfReducedMotion !== 'undefined' && mfReducedMotion && mfReducedMotion.matches)) ctx.globalAlpha = opacity;
    ctx.fillStyle = 'rgba(14,8,28,.96)';
    ctx.strokeStyle = e === player ? '#e0afff' : e.boss ? '#ff95a9' : '#ffd596';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(x - width / 2, y - height, width, height, 10);
    ctx.fill();
    ctx.stroke();
    // Small tail anchors each bubble to its speaker.
    ctx.beginPath();
    ctx.moveTo(x - 7, y);
    ctx.lineTo(x, y + 8);
    ctx.lineTo(x + 7, y);
    ctx.fill();
    ctx.stroke();
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#f5caff';
    ctx.font = 'bold 10px system-ui';
    ctx.fillText(label, x, y - height + 12);
    ctx.fillStyle = '#ffffff';
    ctx.font = 'bold ' + fontSize + 'px system-ui';
    for (let i = 0; i < lines.length; i++) ctx.fillText(lines[i], x, y - height + 28 + i * (fontSize + 4), width - padding * 2);
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
    '<button id="mfEffectsBtn" type="button" aria-pressed="false">✨ Effects: Full</button><button id="mfSpeechBtn" type="button" aria-pressed="true">💬 Dialogue: ON</button><button id="mfSpeechFocusBtn" type="button" aria-pressed="false">💬 All dialogue</button><span id="mfSpeechAnnounce" class="mf-sr-only" aria-live="polite"></span>',
    'dialogue controls');
  return replaceOnce(s, '</style>', '.mf-sr-only{position:absolute!important;width:1px;height:1px;padding:0;margin:-1px;overflow:hidden;clip-path:inset(50%);white-space:nowrap;border:0}</style>', 'accessible text');
}
module.exports = {patchJs, patchHtml};
