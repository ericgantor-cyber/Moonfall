# Moonfall AI Art performance budget planner

Run the existing read-only auto-audit first:

```sh
cd godot-v38
python tools/ai_art_auto_audit.py --strict
python tools/ai_art_budget.py
```

The second command reads `ai_art_staging/review/auto_audit.json` and produces
`art_budget.csv`, `art_budget.json`, and `art_budget.md` alongside it.
All recommendations are deterministic and fully offline. There are **no
asset changes, approvals, AI API calls, billing, or modifications to Z/Nyra**.

Only approved assets are prioritized; missing, staged and unreviewed artwork
never appears as shipped content. Priority **P0** indicates an asset with
integrity errors; **P1** means a metadata-based optimization candidate;
**P3** means no metadata budget flags, not a guarantee of good performance.
Texture estimates are uncompressed source RGBA+mipmap approximations, **not
measured GPU VRAM**. The thresholds are advisory, not engine or art limits.

Tune them for your PC or release target:

```sh
python tools/ai_art_budget.py --texture-warn-mib 128 --triangle-warn 200000
```

Before acting on suggestions, profile real frame times and GPU memory in
the Godot Windows build, including repeated wildlife, bosses and animation.
Regression tests: `python -m unittest tests.test_ai_art_budget` from
`godot-v38`. This planner works without adding engine or external dependencies.
