'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const text=fs.readFileSync(require.resolve('./phase20-continue-integration.js'),'utf8');
assert(text.includes("delete mfWorld.flags['rpg:continue-manifest']"));
const plan=require('./phase22-start-core.js');
assert.equal(plan.continuationPlan(null,null,null),null);
console.log('Phase23 continuation metadata: PASS');
