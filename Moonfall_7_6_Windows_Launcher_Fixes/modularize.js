// Build Moonfall's modular Windows game from the original standalone HTML.
// Source artwork and gameplay are never modified in place.
'use strict';

const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const root = __dirname;
const source = fs.readFileSync(path.join(root, 'game.html'), 'utf8');
const matches = [...source.matchAll(/<script>([\s\S]*?)<\/script>/g)];
if (matches.length !== 1) throw new Error(`Expected exactly one inline game script, found ${matches.length}`);
const scriptMatch = matches[0];

const transforms = [
  require('./auto-level-cards.js'),
  require('./combat-polish.js'),
  require('./roguelike-boons.js'),
  require('./roguelike-rooms.js'),
  require('./phase6-world-integration.js'),
  require('./stage-one-expansion.js'),
  require('./boss-combat-stage.js'),
  require('./sanctuary-upgrades.js'),
  require('./weapon-evolutions.js'),
  require('./combat-feedback-stage.js'),
  require('./hades-combo.js'),
  require('./speech-bubbles.js'),
  require('./render-scale.js'),
  require('./performance-overlay.js'),
  require('./arcane-ui.js'),
  require('./environment-atmosphere.js'),
  require('./combat-telegraph-clarity.js'),
  require('./phase7-world-visuals.js'),
];
const mobs = require('./integrate-mob-variants.js');
const mobCatalog = fs.readFileSync(path.join(root, 'mob-variants.js'), 'utf8');

function transform(input, operation) {
  return transforms.reduce((current, transformer) => {
    const fn = transformer[operation];
    return typeof fn === 'function' ? fn(current) : current;
  }, input);
}

const gameScript = mobs.patchJs(transform(scriptMatch[1], 'patchJs'));
const html = transform(source, 'patchHtml')
  .replace(scriptMatch[0], '<script src="game.js" defer></script>');
if (!html.includes('<script src="game.js" defer></script>')) throw new Error('Missing generated game.js script reference');

const output = path.join(root, 'modular');
const assetDir = path.join(output, 'assets');
fs.mkdirSync(assetDir, {recursive: true});

let imageCount = 0;
const files = new Map();
const bundledScript = (mobCatalog + '\n' + gameScript).replace(
  /data:image\/(webp|png|jpeg);base64,([A-Za-z0-9+/=]+)/g,
  (_match, ext, encoded) => {
    const name = 'art-' + String(++imageCount).padStart(3, '0') + '.' + ext;
    const bytes = Buffer.from(encoded, 'base64');
    if (!bytes.length || bytes.toString('base64').replace(/=+$/, '') !== encoded.replace(/=+$/, '')) {
      throw new Error('Invalid embedded image data: ' + name);
    }
    files.set(name, bytes);
    return 'assets/' + name;
  }
);

// Validate all generated JavaScript before changing outputs.
new vm.Script(bundledScript, {filename: 'modular/game.js'});

// Avoid rewriting unchanged outputs; preserve unrelated assets.
function writeIfChanged(file, data) {
  const bytes = Buffer.isBuffer(data) ? data : Buffer.from(data);
  if (fs.existsSync(file) && fs.statSync(file).size === bytes.length && fs.readFileSync(file).equals(bytes)) return;
  const temporary = file + '.moonfall-tmp';
  try {
    fs.writeFileSync(temporary, bytes);
    fs.renameSync(temporary, file);
  } finally {
    if (fs.existsSync(temporary)) fs.unlinkSync(temporary);
  }
}
for (const [name, bytes] of files) writeIfChanged(path.join(assetDir, name), bytes);
for (const name of fs.readdirSync(assetDir)) {
  if (/^art-\d{3,}\.(webp|png|jpeg)$/.test(name) && !files.has(name)) {
    fs.unlinkSync(path.join(assetDir, name));
  }
}
writeIfChanged(path.join(output, 'game.html'), html);
writeIfChanged(path.join(output, 'game.js'), bundledScript);
console.log('Moonfall modular build:', imageCount, 'original image assets extracted');
