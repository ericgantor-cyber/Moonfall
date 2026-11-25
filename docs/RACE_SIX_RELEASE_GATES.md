# Moonfall: six release gates for persistent races

These are **not** claims of successful native UE5 builds. PR #914 stacks on #913, #912, #911. Do not merge until each level is verified.

1. **UE5.4 compiler:** Generate project files on Windows with the Visual Studio C++ toolchain; build editor and Win64 shipping targets; fix compile errors (including SaveGame UHT reflection, FDateTime/Math calls, and input delegate signature).
2. **Windows runtime:** Launch packaged game on Windows 10 and 11; verify no freezes or crashes entering combat, dying, quitting, and restarting. Check FPS and accessible UI scaling.
3. **Save/load:** Choose each of 12 races; earn XP, equip items, cast and activate power, quit/restart; verify one persistent save slot and no rollbacks. Verify lost power cooldown after immediate exit is prevented.
4. **Cooldown/clock:** Validate 45-second cooldown, time spent offline, clock moved backward/forward, paused game and death. The UTC expiry uses a 45s cap on restoration; external clock changes need test.
5. **Migration:** Preserve copies of V1 and V2 saves; load them with V3 without gear, XP or checkpoint loss. Refuse unsupported/future versions without overwriting them.
6. **Controller:** Verify Gamepad FaceButton Top activates power in ordinary gameplay but only confirms equipment in inventory; ensure menu/death guards, keyboard T and remapping work. Confirm control overlap with other action mappings.

Current static checks only inspect code text, not game behavior. A real Unreal Windows build and packaged smoke tests remain mandatory.

Never change original Z/Nyra character artwork or convert the connected RPG world into separate runs.
