# Moonfall Native v28: AI scheduling modernization

- Split the legacy creature-update loop into `_update_beast_step`, retaining exactly the existing movement, ecology and combat calculations.
- New v28 scheduler allows only *motionless, distant, calm, non-boss, non-rift* beasts to receive full AI ticks once every four frames. Elapsed time is accumulated and caught up so attack and cooldown timers do not slow down.
- Engaged, alerted, moving, hunting, fleeing, staggered, retreating, telegraphing, boss and rift creatures remain full-rate. Ecology scans remain on their original cadence; collision grid unchanged.
- Counters `v28_idle_skips` and `v28_ai_live_steps` let test code compare scheduling decisions.
- Added `ai_budget.gd`, `v28_ai_scheduler.gd`, Windows CI and Inno setup version updates.
- Existing v27 save schema 2 and all original image files remain unchanged.

**Caution:** Real performance gains vary by region and number of distant idle beasts. A synthetic scheduling test is not a graphics FPS benchmark. Windows controller/GPU and installation testing remains outstanding.
