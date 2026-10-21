'use strict';
// Live boss phases for the modular Windows build. Art and existing loot stay intact.
(function(){
 const thresholds=[.7,.35];
 window.mfBossPhaseUpdate=function(enemy,stage){
  if(!enemy||!enemy.boss||!Number.isFinite(enemy.hp)||!Number.isFinite(enemy.max)||enemy.max<=0)return null;
  const ratio=Math.max(0,enemy.hp/enemy.max);
  const next=ratio<=thresholds[1]?2:ratio<=thresholds[0]?1:0;
  const previous=enemy.mfBossPhase||0;
  enemy.mfBossPhase=Math.max(previous,next);
  enemy.phaseTwo=enemy.mfBossPhase>=1;
  enemy.phaseThree=enemy.mfBossPhase>=2;
  return enemy.mfBossPhase>previous?{phase:enemy.mfBossPhase,stage}:null;
 };
})();
