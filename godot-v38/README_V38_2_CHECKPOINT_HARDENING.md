# Moonfall v38.2 checkpoint safety patch

This source patch hardens the existing Godot v38.1 resume system instead of adding a second one. The existing title-screen Continue and saved inventory/equipment remain in `native_v11.gd` and inherited successors.

The checkpoint decoder now rejects absurd coordinates, negative progression fields and nonpositive saved HP, level or world-day values. A new offline regression verifier checks the source guardrails. Z and Nyra original assets remain byte-for-byte unchanged.

**Not tested here:** Godot engine execution, Windows export, installer and interactive resume/recovery. Use `BUILD_MOONFALL_ONE_CLICK.bat` on Windows for the build gates.
