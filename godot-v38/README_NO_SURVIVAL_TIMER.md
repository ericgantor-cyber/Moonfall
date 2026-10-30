# Moonfall v33.8: Remove survival stopwatch

Requested UI rule: **No survival stopwatch** in Moonfall's in-game HUD or death feedback.

In the supplied v33.8 native Godot project, the gameplay HUD and respawn feedback already do not display a survival-time counter. No gameplay code needed to be removed. This patch adds `tests/verify_no_survival_timer.py` to the normal source validation process to prevent that counter being reintroduced.

**Remains:** Day/night cycle and internal combat cooldowns, attack windups, boss scheduling and status-effect timers. They are not a survival stopwatch.

**No death statistics screen is implemented in this native source**; a future death screen should omit survival time entirely and show level, kills, bosses and rewards instead.

Run: `python tests/verify_source.py`.

This ZIP is editable Godot source and is not a Windows installer.
