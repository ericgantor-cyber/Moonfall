// Report embedded asset sizes without changing Moonfall's gameplay or artwork.
const fs = require('node:fs');
const path = require('node:path');
const html = fs.readFileSync(path.join(__dirname, 'game.html'), 'utf8');
const images = [...html.matchAll(/data:image\/([\w+.-]+);base64,([A-Za-z0-9+/=]+)/g)];
const total = images.reduce((n, match) => n + match[0].length, 0);
console.log('Moonfall source size:', (Buffer.byteLength(html) / 1048576).toFixed(2), 'MiB');
console.log('Embedded images:', images.length);
console.log('Embedded image text:', (total / 1048576).toFixed(2), 'MiB');
images.forEach((match, i) => console.log(
  String(i + 1).padStart(2), match[1], (match[0].length / 1048576).toFixed(2) + ' MiB'
));
console.log('Note: These assets are NOT removed. Preserve original character artwork.');
