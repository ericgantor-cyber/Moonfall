'use strict';
const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const game=fs.readFileSync(path.join(__dirname,'game.html'),'utf8');
const begin=game.indexOf('const mfCheckpointKey=');
const end=game.indexOf('const mfManualCheckpointKey=',begin);
assert(begin>=0&&end>begin,'Checkpoint functions not found');
let code="const mfManualCheckpointKey='moonfall.checkpoint.manual.v1';\n"+game.slice(begin,end);
const validatorBegin=game.indexOf('function mfValidateCheckpoint(d){');
const validatorEnd=game.indexOf("document.getElementById('mfExportCheckpoint')",validatorBegin);
assert(validatorBegin>=0&&validatorEnd>validatorBegin,'Validator not found');
code+='\n'+game.slice(validatorBegin,validatorEnd);
const slots=new Map();
const localStorage={getItem:k=>slots.has(k)?slots.get(k):null,setItem:(k,v)=>slots.set(k,String(v)),removeItem:k=>slots.delete(k)};
const ctx=vm.createContext({localStorage,console,Date,Math,JSON,Number,Object,Array,String,Error});
vm.runInContext(code,ctx);
const run=expression=>vm.runInContext(expression,ctx);
const current='moonfall.checkpoint.v1',previous='moonfall.checkpoint.previous.v1',pending='moonfall.checkpoint.pending.v1',manual='moonfall.checkpoint.manual.v1';
function data(savedAt,level=2){return {version:1,savedAt,hero:'Z',level,xp:10,maxHP:200,hp:190,kills:1,chapter:1,seconds:30,attackMultiplier:1,spellMultiplier:1,position:[0,0],waveStats:{move:1,cooldown:1,damage:1,healing:0}}}
function commit(d,opts){ctx.testData=d;ctx.testOpts=opts||{};return run('mfCommitCheckpoint(testData,testOpts)')}
function valid(d){ctx.testData=d;return run('mfValidateCheckpoint(testData)')}
function read(k){ctx.testKey=k;return run('mfReadCheckpointSlot(testKey)')}
const now=Date.now();
assert(valid(data(now)),'Valid checkpoint rejected');
commit(data(now-2000));
assert(read(current).integrity,'New checkpoint is not sealed');
commit(data(now-1000,3));
assert(read(previous).integrity,'Previous backup is not sealed');
assert.equal(read(previous).level,2);
const corrupted=JSON.parse(slots.get(current));corrupted.level=999;slots.set(current,JSON.stringify(corrupted));
assert.equal(read(current),null,'Tampered checkpoint accepted');
run('mfRecoverInterruptedSave()');
assert.equal(read(current).level,2,'Recovery did not use valid previous checkpoint');
assert.equal(slots.has(pending),false,'Pending checkpoint was not cleared');
// Newest dated manual backup beats an older previous backup.
slots.delete(current);slots.set(previous,JSON.stringify(data(now-3000,2)));
slots.set(manual,JSON.stringify(data(now-500,4)));
run('mfRecoverInterruptedSave()');
assert.equal(read(current).level,4,'Newest backup not chosen');
// A staged write newer than active wins, preserving old active as backup.
slots.set(current,JSON.stringify(data(now-3000,5)));
slots.set(pending,JSON.stringify(data(now-100,6)));
run('mfRecoverInterruptedSave()');
assert.equal(read(current).level,6,'Newer pending save not promoted');
assert.equal(read(previous).level,5,'Old active save not preserved');
assert(!slots.has(pending),'Staged marker remains after successful recovery');
// Rejected future timestamps and malformed data.
assert(!valid(data(now+600000)),'Far-future timestamp accepted');
assert(!valid({...data(now),hp:500}),'Impossible HP accepted');
// Boss state must match the chapter and use supported phase bounds.
const withBoss=(chapter,kind,phase=0)=>({...data(now),chapter,bossState:{kind,hp:50,max:100,phase}});
assert(valid(withBoss(8,'dragon',3)),'Valid dragon snapshot rejected');
assert(valid(withBoss(5,'phoenix',2)),'Valid Phoenix snapshot rejected');
assert(!valid(withBoss(8,'phoenix')),'Wrong-chapter boss snapshot accepted');
assert(!valid(withBoss(5,'dragon')),'Wrong-chapter dragon accepted');
assert(!valid(withBoss(2,'screamer',1)),'Impossible Screamer phase accepted');
assert(!valid(withBoss(5,'egg',1)),'Impossible egg phase accepted');
console.log('PASS: 3.2 save commit, checksum, backup, recovery, timestamp and HP regression tests');
