const fs=require('fs'),assert=require('assert');
const s=fs.readFileSync('game.html','utf8');
assert(s.includes('const MF_MAX_ENEMIES=500;'));
for(const expression of [
 'aiMinions.length<MF_MAX_ENEMIES',
 'Math.min(aiCurrentWaveSize,MF_MAX_ENEMIES-aiMinions.length)',
 'aiMinions.slice(0,MF_MAX_ENEMIES)',
 'state.mobs.length>MF_MAX_ENEMIES',
 "aiMinions.length+'/'+MF_MAX_ENEMIES+' mobs'"
]) assert(s.includes(expression),`Missing cap enforcement: ${expression}`);
assert(!s.includes('const MF_MAX_ENEMIES=120;'));
console.log('PASS: 500 active/saved enemy cap wired to spawning, snapshots, validation, HUD');
