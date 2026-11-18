// Offline visual-gallery JavaScript is validated without a browser/Unreal.
import fs from "node:fs";
import vm from "node:vm";
import assert from "node:assert/strict";
import path from "node:path";
import { fileURLToPath } from "node:url";

const dir=path.dirname(fileURLToPath(import.meta.url));
const gallery=fs.readFileSync(path.join(dir,"..","MoonfallUnreal","Data",
 "LootArt","Moonfall_LootArt_Gallery.html"),"utf8");
const match=gallery.match(/<script>\s*([\s\S]*?)\s*<\/script>/);
assert.ok(match,"Gallery must be dependency-free with an inline script");
const script=match[1];
new vm.Script(script);
const cutoff=script.indexOf("const el=id=>");
assert.ok(cutoff>0,"Expected pure art rules before DOM rendering");
const {ATLAS,choose,QUALITY}=vm.runInNewContext(
 script.slice(0,cutoff)+"\n;({ATLAS,choose,QUALITY})");
assert.equal(ATLAS.length,6);
assert.equal(ATLAS.reduce((n,a)=>n+a.names.length,0),51);
assert.deepEqual(Array.from(ATLAS,atlas=>atlas.names.length),[6,9,9,9,9,9]);
assert.equal(QUALITY.length,6);
for(const atlas of ATLAS){
 assert.ok(atlas.file.startsWith("Moonfall_Loot") &&
  atlas.file.endsWith(".png"));
 assert.equal(atlas.cols,3);
 assert.equal(atlas.cols*atlas.rows,atlas.names.length);
 assert.equal(new Set(atlas.names).size,atlas.names.length);
}
assert.equal(choose(0,0,0,1,true,true,true).atlas,0);
assert.equal(choose(0,0,0,1,true,true,true,true,true,true).atlas,5);
assert.equal(choose(1,2,1,7,true,true,true,true,true,true).col,(7+2+1)%3);
assert.equal(choose(2,2,1,8,true,true,true,true,true,true).atlas,1);
assert.equal(choose(2,2,1,7,true,true,true,true,true,false).atlas,1);
assert.equal(choose(1,2,1,4,true,true,true).col,2);
assert.equal(choose(1,2,1,4,true,true,true).row,1);
assert.equal(choose(2,3,3,7,true,true,true).atlas,1);
assert.equal(choose(0,3,1,8,true,true,true).atlas,2);
assert.equal(choose(0,3,1,8,true,true,true).col,0);
assert.equal(choose(2,5,3,25,true,true,true).row,2);
assert.equal(choose(0,5,2,16,true,true,true,true).atlas,3);
assert.equal(choose(1,4,1,16,true,true,true,true).row,1);
assert.equal(choose(2,5,3,25,true,true,true,true).col,(25+5+3)%3);
assert.equal(choose(0,3,1,16,true,true,true,true).atlas,2,
 "Epic is still Epic; masterwork is Emerald/Legendary only");
assert.equal(choose(0,4,1,15,true,true,true,true).atlas,2,
 "Masterwork requires tier 16");
assert.equal(choose(0,5,1,16,true,true,true,false).atlas,2,
 "Missing masterwork texture falls back to the relic atlas");
assert.equal(choose(0,5,1,22,true,true,true,true,true).atlas,4);
assert.equal(choose(2,5,3,25,true,true,true,true,true).col,(25+3+2)%3);
assert.equal(choose(1,5,1,21,true,true,true,true,true).atlas,3,
 "Ascendant artwork requires tier 22");
assert.equal(choose(1,4,1,25,true,true,true,true,true).atlas,3,
 "Emerald still selects Masterwork not Ascendant");
assert.equal(choose(1,5,1,22,true,true,true,true,false).atlas,3,
 "Absent Ascendant texture should choose Masterwork");
assert.equal(choose(0,0,1,1,true,true,true),null);
assert.equal(choose(0,5,1,0,true,true,true),null);
assert.equal(choose(0,5,1,8,false,false,false),null);
for(let slot=0;slot<3;slot++){
 for(let rarity=0;rarity<6;rarity++){
  for(let trait=0;trait<4;trait++){
   for(let tier=1;tier<=25;tier++){
    if(rarity===0&&trait!==0)continue;
    for(let mask=0;mask<64;mask++){
     const v=choose(slot,rarity,trait,tier,!!(mask&1),!!(mask&2),!!(mask&4),!!(mask&8),!!(mask&16),!!(mask&32));
     if(v){
      const a=ATLAS[v.atlas];
      assert.ok(v.col>=0&&v.col<a.cols&&v.row>=0&&v.row<a.rows,
        "Never read outside an atlas cell");
      assert.ok(a.names[v.row*a.cols+v.col],
        "Every selected illustration must have a name");
     }
    }
   }
  }
 }
}
assert.ok(!script.includes("innerHTML"),"User-entered search must not become HTML");
assert.ok(!script.includes("fetch("),"Gallery never contacts external servers");
assert.ok(!gallery.includes("http://")&&!gallery.includes("https://"));
for(const id of ["slot","rarity","trait","tier","base","variant","relic",
 "masterwork","ascendant","wayfarer","high-visibility","gallery","search","collection","picked-sprite"]){
 assert.ok(gallery.includes('id="'+id+'"'),"Missing accessible control "+id);
}
console.log("Moonfall gallery: 51 original art cells, 6 atlases, 100,800 safe combinations, offline syntax OK");
