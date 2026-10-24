'use strict';
// Reports original embedded-art reference sites without printing artwork bytes.
const fs=require('node:fs');
const path=require('node:path');
const source=fs.readFileSync(path.join(__dirname,'../../Moonfall_7_6_Windows_Launcher_Fixes/modular/game.js'),'utf8');
const matcher=/assets\/art-\d{3,}\.(?:png|webp|jpeg)/g;
let match;
while((match=matcher.exec(source))){
 const before=source.slice(Math.max(0,match.index-160),match.index).replace(/\s+/g,' ');
 const after=source.slice(match.index+match[0].length,match.index+match[0].length+95).replace(/\s+/g,' ');
 console.log(match[0]+' | before: '+before.slice(-160)+' | after: '+after.slice(0,95));
}
