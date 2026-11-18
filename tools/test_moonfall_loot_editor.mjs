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
const helper=vm.runInNewContext(script.slice(0,cutoff)+"\n;({FIELDS,SOURCES,defaultRows,validate,summary,parseCsv,csvField,bulkAdjust,uniqueCopyName})");
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
assert.equal(generic.eligibleCount,3);
assert.equal(Math.round(generic.slots.reduce((a,b)=>a+b,0)),100);
assert.equal(Math.round(generic.traits.reduce((a,b)=>a+b,0)),100);
assert.equal(generic.traits[0],0,"Epic+ generic bosses have only named traits");
const region=helper.summary(original,"Boss",0,60);
assert.equal(region.total,108);
assert.equal(region.weights[3],83);
assert.equal(region.weights[5],5);
assert.equal(region.eligibleCount,4);
assert.ok(Math.abs(region.slots[0]-(100/3+8))<1e-8,
 "Region 1 boss row has weapon-only bitmask");
assert.ok(Math.abs(region.traits[1]-(100/3+8))<1e-8,
 "Region 1 boss row forces Force while generic rows have uniform named traits");
const low=helper.summary(original,"Treasure",0,1);
assert.equal(low.total,98);
assert.equal(low.weights[5],0);
const uncommon=helper.summary(original,"Regular",0,1);
assert.ok(Math.abs(uncommon.traits[0]-64.4)<1e-8,
 "Uncommon generic rows have 60% untraited chance");
const altered=helper.bulkAdjust(original.map(row=>({...row})),150,row=>row.Name==="Boss_R3");
assert.equal(altered.find(row=>row.Name==="Boss_R3").Weight,113,
 "Bulk edits round weights to integer values");
assert.equal(altered.find(row=>row.Name==="Boss_R4").Weight,20,
 "Bulk edits may not affect unmatched rows");
assert.throws(()=>helper.bulkAdjust(original.map(row=>({...row})),150,()=>false),
 /No rows match/);
const overweight=original.map(row=>({...row}));
overweight.find(row=>row.Name==="Boss_R3").Weight=10000;
assert.throws(()=>helper.bulkAdjust(overweight,200,row=>row.Name==="Boss_R3"),
 /exceeds/, "Never silently clamp a bulk edit");
assert.equal(helper.uniqueCopyName("Boss_R3",original),"Boss_R3_copy");
assert.equal(helper.uniqueCopyName("Boss_R3",original.concat({Name:"boss_r3_COPY"})),"Boss_R3_copy2");
const invalid=JSON.parse(JSON.stringify(original));
invalid[0].Weight=10001;
assert.ok(helper.validate(invalid).errors.length>0);
invalid[0].Weight=44;
const elite=invalid.find(row=>row.Source==="Elite");
assert.ok(elite);
elite.Rarity=0;
assert.ok(helper.validate(invalid).errors.length>0,"Invalid elite rarity floor must fail");
const caseCollision=original.map(row=>({...row}));
caseCollision[1].Name=caseCollision[0].Name.toLowerCase();
assert.ok(helper.validate(caseCollision).errors.some(error=>error.includes("Duplicate")),
 "Unreal FName case-insensitive collisions must fail");
const missingName=original.map(row=>({...row}));
missingName[0].Name="Cannot import spaces";
assert.ok(helper.validate(missingName).errors.some(error=>error.includes("Invalid row name")));
const incomplete=JSON.parse(JSON.stringify(original)).filter(row=>row.Source!=="Elite");
assert.ok(helper.validate(incomplete).errors.length>0,"Incomplete tables fail closed");
assert.ok(script.includes("file.size>1024*1024"),"Limit imported CSV file size");
assert.ok(script.includes("const candidate=parseCsv(await file.text())"),"Parse into a temporary candidate");
assert.ok(script.indexOf("const checked=validate(candidate)") < script.indexOf("rows=checked.rows"),"Validate before replacing current edits");
assert.ok(script.includes("Import rejected; your current table is unchanged."),"Import failure must explain edits are preserved");
const malformed=helper.parseCsv("Name,Source,ZoneIndex,MinimumLevel,Rarity,Weight,SlotMask,ForcedTrait\nBad,Regular,-1,1,0,99999,7,0\n");
assert.ok(helper.validate(malformed).errors.length>0,"Out-of-range imported weights must be rejected");
assert.equal(script.includes("innerHTML"),false,"Do not inject user-authored row names as markup");
assert.equal(script.includes("fetch("),false,"Offline editor must not contact servers");
assert.equal(script.includes("eval("),false,"Do not evaluate CSV content as code");
for(const id of ["export","import","add","reset","undo","redo","bulk-factor","bulk-apply",
 "filter-zone","page-size","page-prev","page-next","page-status",
 "slots","traits","row-odds","unsaved","preview-source","preview-zone","preview-level","rows"]){
 assert.ok(html.includes('id="'+id+'"'),"Missing accessible UI control: "+id);
}
assert.ok(script.includes("rememberEdit();rows.splice(index,1)"),"Deletion must support undo");
assert.ok(script.includes("rememberEdit();rows=defaultRows()"),"Reset must support undo");
assert.ok(script.includes("rows=undoHistory.pop()"),"Undo must restore previous table");
assert.ok(script.includes("rows=redoHistory.pop()"),"Redo must restore next table");
assert.ok(script.includes("window.addEventListener(\"beforeunload\""),"Warn before losing unexported edits");

// Exercise real editor event handlers with a minimal dependency-free DOM shim.
// Does not replace a graphical browser/Unreal integration test.
class FakeElement {
 constructor(){this.value="";this.disabled=false;this.className="";this.textContent="";
   this.children=[];this.handlers={};this.style={};}
 append(...children){this.children.push(...children);}
 replaceChildren(...children){this.children=[...children];}
 setAttribute(){}
 addEventListener(event,handler){this.handlers[event]=handler;}
 click(){if(this.handlers.click)this.handlers.click();}
}
const nodes=new Map(),get=id=>{
 if(!nodes.has(id))nodes.set(id,new FakeElement());
 return nodes.get(id);
};
get("filter").value="All";
get("filter-zone").value="all";
get("page-size").value="80";
get("bulk-factor").value="150";
get("preview-source").value="Boss";
get("preview-zone").value="0";
get("preview-level").value="60";
get("search").value="";
const browser=vm.createContext({
 document:{getElementById:get,createElement:()=>new FakeElement(),addEventListener(){}},
 window:{addEventListener(){}}, console
});
vm.runInContext(script,browser);
const liveRows=()=>vm.runInContext("rows.map(row=>({...row}))",browser);
assert.equal(liveRows().length,41);
get("add").click();
assert.equal(liveRows().length,42,"Add button should insert a new row");
assert.equal(get("undo").disabled,false,"Undo is available after editing");
get("undo").click();
assert.equal(liveRows().length,41,"Undo restores a prior table");
get("redo").click();
assert.equal(liveRows().length,42,"Redo restores the later table");
get("undo").click();
const bossBefore=liveRows().find(row=>row.Name==="Boss_R3").Weight;
get("filter").value="Boss";
get("filter").handlers.change();
get("bulk-apply").click();
assert.equal(liveRows().find(row=>row.Name==="Boss_R3").Weight,Math.round(bossBefore*1.5));
assert.equal(liveRows().find(row=>row.Name==="Regular_R0").Weight,44,
 "Bulk edit must honor reward source filters");
get("undo").click();
assert.equal(liveRows().find(row=>row.Name==="Boss_R3").Weight,bossBefore);
get("filter").value="All";get("filter").handlers.change();
get("page-size").value="40";get("page-size").handlers.change();
assert.equal(get("page-next").disabled,false,"Forty-one default rows span two pages");
get("page-next").click();
assert.ok(get("page-status").textContent.includes("page 2/2"));
get("page-prev").click();
assert.ok(get("page-status").textContent.includes("page 1/2"));
console.log("Moonfall loot editor: syntax, 41 rows, exact odds, safe edit history, bulk changes, pagination and offline event-handler smoke OK");
