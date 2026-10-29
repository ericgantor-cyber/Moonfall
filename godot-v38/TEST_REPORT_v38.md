# Moonfall v38.0 Overhaul Test Report

**Build artifact type:** Godot 4.7.2 source. **No Windows executable or installer was built in this environment.**

## Verified
- **40/40** offline Python verifiers passed, executed as separate child processes and checked again AFTER cleaning the source package.
- **66** unique Godot engine suites registered with exact agreement between the Windows batch and Python runners. **Not executed**, since Godot was unavailable.
- Strict engine-log gate remains active, including the v38 test log.
- **70/70 original image SHA-256 hashes match** the original v36.2 art manifest. No changes to Z or Nyra images.
- 96 older Markdown notes moved into optional historical archive. No gameplay scripts, tests, art, saves, build tools, CI workflows or project scenes removed.
- `scripts/native_v37_4.gd`: noncumulative dedicated swim-pose anchor, reset when leaving water; inherited v16 combat sprite transform remains independent.
- `scripts/native_v38_0.gd`: F2 status panel for region, HP, air, depth and danger; hidden by default; bounded to at most 4 updates/second; no save schema change.
- v38 gameplay test registered in batch and Python. Code verified offline, but **in-engine physics and UI behavior remain unverified**.

## Pending on real Windows/Godot
- Actual Godot 4.7.2 parse/import, 66 engine suites, GPU frame-rate and 4K gameplay testing.
- Real installer compilation, Windows install/uninstall, keyboard/controller and accessibility hands-on testing.
- Full 3D faithful Z/Nyra models: current characters still use their original 2D artwork as billboards in the 3D world.
- Game executable and installer filenames retain v34.4 because legacy release gates use those names. The gameplay source is v38.0; installed metadata still needs a coordinated release-version migration after successful runtime testing.

## Scope
One connected world; no new modes, construction system or global survival clock. Save schema preserved.
