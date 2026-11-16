# Moonfall celestial Wait release candidate checklist

This branch cumulatively contains the latest Godot Moonfall Wait and day/night changes. It is **not** a released build.

## Implemented in source
- Full day/night cycle extended from 8 to 16 real minutes.
- Wait selector for 1–24 in-game hours with keyboard, controller and clickable buttons.
- Celestial clock art drawn in Godot, with animated time-of-day transition and skip controls.
- Combat gating, time/day preview, post-wait save, and midnight clock regression tests.
- Original Z and Nyra artwork and the existing six-region connected world preserved.

## Mandatory release gates
- [ ] Latest commit's Godot Windows build/installer Actions job succeeds.
- [ ] Download and inspect artifact and RELEASE_EVIDENCE.json SHA-256 values.
- [ ] Test installed game on Windows 10 and Windows 11 (real hardware).
- [ ] Play-test Wait: 1h, 24h, midnight, keyboard, controller, mouse, combat refusal, cancel, and animation skip.
- [ ] Save/load after waiting, including multi-day midnight transitions.
- [ ] Test daylight/night lighting, weather, enemy interactions and no duplicate rewards.
- [ ] Verify interface at 720p, 1080p and high-DPI scaling and check frame rate.
- [ ] Review dependent draft PR chain before merging; avoid duplicate merges.

The Actions smoke tests validate headless startup and basic installed-game behavior, **not** human gameplay or Windows 10/11 hardware compatibility. The parallel Unreal prototype is not an Unreal compiled release.
