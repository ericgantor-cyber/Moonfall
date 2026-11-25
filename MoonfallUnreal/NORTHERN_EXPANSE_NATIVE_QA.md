# Northern Expanse UE5.4 native acceptance test

Scope: **isolated preview map only**. The portable GitHub checks do not prove
the Unreal Editor compiled, the map asset exists, roads work or controller input
works. Do not merge PR #1074 based on portable CI alone.

## Build and generate the map

1. On a Windows machine with UE5.4 and Visual Studio C++ workload, compile
   `MoonfallUnrealEditor` (Development Editor, Win64).
2. Open `MoonfallUnreal.uproject`. Run `MoonfallUnreal/Tools/create_northern_expanse_preview.py`
   from Tools > Execute Python Script. Do **not** run a second time on the
   same map; the creator refuses to overwrite it.
3. Run `MoonfallUnreal/Tools/validate_northern_expanse_preview.py` in the
   editor. Record the output log. It must report nine pads, at least eight roads,
   18 landmarks, nine overlay components, a PlayerStart, enabled optional
   weather and the dedicated `MoonfallNorthPreviewGameMode`. No original arena,
   30-dungeon layouts, divine characters or legacy monsters may spawn.
4. If any check fails, record the exact Unreal Output Log error and keep the
   pull request unmerged.

## Play-in-Editor acceptance checks

| Area | Procedure | Pass criterion |
|---|---|---|
| Entry | Press Play with the isolated preview map open | Character spawns onto Shiverwake plaza, not inside floor or old arena |
| Travel | Move from Shiverwake through forest/tundra routes and all nine hubs | No unintended gaps, teleportation, invisible barriers or falling through roads |
| Roads | Traverse each connected road in both directions | Walkable surfaces and collision consistently meet pads, with no snagging |
| Province climate | Enable weather-following preview and cross province boundaries | Text/weather identity matches each province; no rapid flicker |
| Weather exit | Leave the northern preview hub coverage | Effect deactivates; entering supported area resumes it |
| Weather readability | Play under each of the nine climate settings | UI, combat indicators and travel surface remain legible |
| Controller | Use XInput controller to walk, turn, jump and open Options | Sticks/buttons function; no unwanted overlap with movement/actions |
| Mouse/keyboard | Move, turn, jump and open Options | Default controls remain usable without a controller |
| Performance | Repeat traversal in UE5.4 packaged Development Win64 | No severe frame spikes, memory growth or render crashes |
| Regression | Open the existing playable/default map afterward | Original arena, zones, art and saves remain unaffected |

The project has *configured* gamepad axes in `Config/DefaultInput.ini`,
including left-stick movement, right-stick camera, A jump and Start Options,
but configuration alone is **not a passed controller test**. Record controller
model, gameplay FPS, runtime logs, driver version and screenshots of key
province transitions.

## Reporting results

Write the outcome for every row above as `PASS`, `FAIL` or `NOT RUN`,
with exact build commit, machine details, Unreal log and reproduction steps.
**A missing native test is NOT RUN, not PASS.** Save the generated .umap only
after validating its contents. It should be a new asset under
`/Game/NorthernExpanse/Maps/NorthernExpansePreview`, never a replacement
for legacy levels.

## Approval gate

Only consider merging after the Windows UE5 Editor compile, generated
level validation, bilateral road traversal, nine climate checks, keyboard/
mouse and controller smoke tests, no unwanted legacy game-mode spawns,
and packaged Windows playtest are recorded as passed.
Z, Nyra, and Morvella artwork must remain byte-for-byte unchanged.
