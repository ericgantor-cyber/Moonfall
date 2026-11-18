// Offline, dependency-free smoke tests for the standalone visual editor.
// Verifies JavaScript syntax, default probabilities, and CSV round trips.
// A real browser/Unreal interaction test is still required for release.
import fs from "node:fs";
import vm from "node:vm";
import assert from "node:assert/strict";
import { fileURLToPath } from "node:url";
import path from "node:path";

const folder=path.dirname(fileURLToPath(import.meta.url));
const html=fs.readFileSync(path.join(folder,"..","MoonfallUnreal","Data","Moonfall_LootEditor.html"),"utf8");
const found=html.match(/<script>\s*([\s\S]*?)\s*<\/script>/);
assert.ok(found,"Editor needs its offline script");
const script=found[1];
new vm.Script(script); // Parse the full browser code, including event listeners.
const cutoff=script.indexOf('byId("reset").addEventListener');
assert.ok(cutoff>0,"Expected event-registration boundary");
const helper=vm.runInNewContext(script.slice(0,cutoff)+"\n;({FIELDS,SOURCES,defaultRows,validate,summary,parseCsv,csvField})");
const original=helper.defaultRows();
assert.equal(original.length,41,"17 global and 24 regional rows");
assert.equal(helper.validate(original).errors.length,0);
const asText=helper.FIELDS.join(",")+"\r\n"+
 original.map(row=>helper.FIELDS.map(field=>helper.csvField(row[field])).join(",")).join("\r\n")+"\r\n";
const recovered=helper.parseCsv(asText);
assert.equal(recovered.length,41);
assert.equal(helper.validate(recovered).errors.length,0);
const generic=helper.summary(original,"Boss",-1,60);
assert.equal(generic.total,100);
assert.equal(generic.weights[3],75);
assert.equal(generic.weights[4],20);
assert.equal(generic.weights[5],5);
const region=helper.summary(original,"Boss",0,60);
assert.equal(region.total,108);
assert.equal(region.weights[3],83);
assert.equal(region.weights[5],5);
const low=helper.summary(original,"Treasure",0,1);
assert.equal(low.total,98);
assert.equal(low.weights[5],0);
const invalid=JSON.parse(JSON.stringify(original));
invalid[0].Weight=10001;
assert.ok(helper.validate(invalid).errors.length>0);
invalid[0].Weight=44;
const elite=invalid.find(row=>row.Source==="Elite");
assert.ok(elite);
elite.Rarity=0;
assert.ok(helper.validate(invalid).errors.length>0,"Invalid elite rarity floor must fail");
const incomplete=JSON.parse(JSON.stringify(original)).filter(row=>row.Source!=="Elite");
assert.ok(helper.validate(incomplete).errors.length>0,"Incomplete tables fail closed");
assert.equal(script.includes("innerHTML"),false,"Do not inject user-authored row names as markup");
assert.equal(script.includes("fetch("),false,"Offline editor must not contact servers");
assert.equal(script.includes("eval("),false,"Do not evaluate CSV content as code");
for(const id of ["export","import","add","reset","preview-source","preview-zone","preview-level","rows"]){
 assert.ok(html.includes('id="'+id+'"'),"Missing accessible UI control: "+id);
}
console.log("Moonfall visual loot editor: syntax, 41 rows, rarity odds, input validation and CSV roundtrip OK");
