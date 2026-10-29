'use strict';
// Prevent checkpoint respawn loops from immediate contact damage.
function recoveryProtected(now,until){return Number.isFinite(now)&&Number.isFinite(until)&&now<until;}
function protectionEnd(now,duration=90){
 if(!Number.isFinite(now)||now<0)return 0;
 return Math.floor(now)+Math.max(0,Math.min(180,Math.floor(duration)));
}
module.exports={recoveryProtected,protectionEnd};
