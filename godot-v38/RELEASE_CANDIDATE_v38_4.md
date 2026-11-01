# Moonfall v38.4 Windows release candidate

**Status: preparation only. Not yet published.**

Source commit: `e7775a94c9cd9a8ab8d5becd94440d807afee746` (merged `main`).

## Highlights
- Two-hour in-game day/night cycle (approximately 60 minutes daylight and 60 minutes nighttime including transitions).
- Celestial Wait interface and save-clock compatibility for older checkpoints.
- New classless custom hero creator with name, palette, hair, eyes, skin, face details, accessories, body shape, live preview, randomize and reset.
- Gameplay HUD shows the chosen custom hero name and in-world HH:MM time.
- Source clock regression parser fix.

## Compatibility and scope
- Six-region connected world retained.
- Z and Nyra original character artwork unchanged.
- Does not introduce new game modes or force class choices.
- Previous save checkpoint should be backed up before trying this release candidate.

## Verification required before publication
- [ ] Godot Windows build on merged `main` commit succeeds.
- [ ] Windows installer and portable game artifacts are present and not expired.
- [ ] Run generated Windows installer, launch game and uninstall on Windows runner.
- [ ] Validate checksums from build evidence; do not invent a hash.
- [ ] Play-test creator, randomize/reset, default hero selection, custom hero save/load, 24-hour Wait, world clock at midnight, boss combat and performance on real Windows hardware.
- [ ] Verify Windows 10 and Windows 11 compatibility on real devices.
- [ ] Attach matching verified binaries to a new GitHub Release and mark clearly as pre-release until manual checks pass.

Build workflow (merged main): https://github.com/ericgantor-cyber/Moonfall/actions/runs/41340640516

Do not substitute an older PR artifact for the merged-main build. The GitHub workflow artifact labels may contain historical version strings.
