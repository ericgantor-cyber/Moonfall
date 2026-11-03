'use strict';
// Integrate after stage-one-expansion registers procedural enemy movement.
function patchJs(s){
 const needle='e.x+=Math.cos(a)*s;e.y+=Math.sin(a)*s;';
 const i=s.indexOf(needle);
 if(i<0||s.indexOf(needle,i+needle.length)!==-1)
  throw Error('Dungeon enemy movement hook missing or ambiguous');
 return s.slice(0,i)+'if(typeof mfDungeonNavigateEnemy===\"function\")mfDungeonNavigateEnemy(e,a,s);else{e.x+=Math.cos(a)*s;e.y+=Math.sin(a)*s;}'+s.slice(i+needle.length);
}
module.exports={patchJs};
