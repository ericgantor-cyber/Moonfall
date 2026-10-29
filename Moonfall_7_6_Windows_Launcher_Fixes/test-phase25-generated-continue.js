'use strict';
// Fail the Windows build if a promised resume UI hook disappears during modularization.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const html=fs.readFileSync(path.join(__dirname,'modular/game.html'),'utf8');
const js=fs.readFileSync(path.join(__dirname,'modular/game.js'),'utf8');
for(const id of ['mfResumeBtn','mfDeathBtn','mfContinueBtn','mfContinueStatus']){
 assert(html.includes('id="'+id+'"'),'Missing continuation control '+id);
}
for(const fn of ['mfRecoveredCheckpoint','mfResumeCheckpoint','mfContinueFromSavedAdventure','mfTryDeathRecovery','mfP18TryLethalRecovery']){
 assert(js.includes('function '+fn+'('),'Missing runtime '+fn);
}
assert(js.includes("rpg:continue-manifest"),'Missing continuation storage');
assert(js.includes("delete mfWorld.flags['rpg:continue-manifest']"),'Missing stale save cleanup');
assert(html.includes('src="game.js"'),'Modular game script link is absent');
console.log('Phase25 modular continuation acceptance: PASS');
