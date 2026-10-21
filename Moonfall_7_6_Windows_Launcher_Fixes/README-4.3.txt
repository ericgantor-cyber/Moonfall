Moonfall 4.3 Crowd Optimization

Keeps 500 active and 500 saved enemy caps.
Distance-aware enemy separation: nearby mobs update separation every 3 frames, midrange every 6, distant every 12.
Each separation pass inspects at most 24 nearby candidates. Distant mob facing updates less often. Movement and attacks still update every frame.
This reduces worst-case AI work, but does not establish an FPS benchmark or guarantee 500-enemy playability.
