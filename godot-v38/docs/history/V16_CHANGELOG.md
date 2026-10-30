# Moonfall Native v16: combat and terrain polish

- Six distinctive seamless 96×96 procedural terrain color maps, generated once per region. Each map repeats via a shared StandardMaterial3D ground mesh; no external art replacement.
- Attacks, dodge and casts gain short sprite transform animations and bounded visual tells, without editing Z/Nyra artwork or changing collision/damage timing.
- Cosmetic particles automatically expire and are capped to 12 active effects.
- Distant actors skip the v14 rig bobbing calculations beyond 40 world units. The existing v15 leg/wing animation cull remains 36 units.
- A two-second lightweight simulation frame-rate sample is exposed for debugging only (not a GPU benchmark).
- One connected six-region world, existing skills/rifts/saves preserved.

**Visual limitation:** This is a procedural animation/material polish pass, not a professional motion-capture or PBR asset replacement.
**Testing limitation:** Native Windows graphics and controller hardware still require real-device testing.
