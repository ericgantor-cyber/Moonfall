// Build Moonfall's modular Windows game from the original standalone HTML.
// Source artwork and gameplay are never modified in place.
'use strict';

const fs = require('node:fs');
const path = require('node:path');

const root = __dirname;
const source = fs.readFileSync(path.join(root, 'game.html'), 'utf8');
const scriptMatch = source.match(/<script>([\s\S]*?)<\/script>/);
if (!scriptMatch) throw new Error('Moonfall inline game script not found');

const transforms = [
  require('./auto-level-cards.js'),
  require('./combat-polish.js'),
  require('./roguelike-boons.js'),
  require('./roguelike-rooms.js'),
  require('./stage-one-expansion.js'),
  require('./boss-combat-stage.js'),
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

const output = path.join(root, 'modular');
const assetDir = path.join(output, 'assets');
fs.mkdirSync(assetDir, {recursive: true});

let imageCount = 0;
const files = new Map();
const bundledScript = (mobCatalog + '\n' + gameScript).replace(
  /data:image\/(webp|png|jpeg);base64,([A-Za-z0-9+/=]+)/g,
  (_match, ext, encoded) => {
    const name = 'art-' + String(++imageCount).padStart(3, '0') + '.' + ext;
    files.set(name, Buffer.from(encoded, 'base64'));
    return 'assets/' + name;
  }
);

// Only clean previously generated image files. Never touch unrelated assets.
for (const [name, bytes] of files) fs.writeFileSync(path.join(assetDir, name), bytes);
for (const name of fs.readdirSync(assetDir)) {
  if (/^art-\d{3,}\.(webp|png|jpeg)$/.test(name) && !files.has(name)) {
    fs.unlinkSync(path.join(assetDir, name));
  }
}
fs.writeFileSync(path.join(output, 'game.html'), html);
fs.writeFileSync(path.join(output, 'game.js'), bundledScript);
console.log('Moonfall modular build:', imageCount, 'original image assets extracted');
