# Moonfall v37.2: Dive Oxygen Meter

- In any of the six existing lakes, hold **C** (keyboard) or click and hold the **right controller stick** to perform a shallow dive. Release to return to surface breathing.
- Oxygen HUD appears in the lake only, from 100%. Diving drains 4% per second, breathing refills 25% per second. Outside the lake oxygen resets immediately.
- At 25% air the meter turns red and provides a low-air warning. At 0% continued diving causes periodic health damage using the existing death and Phoenix Feather rules.
- Pausing freezes air use. No persistent oxygen stat, no global survival timer, no new currency, no new mode.
- This is a simple shallow-dive **breathing mechanic**, not a full underwater terrain or swimming animation system. Existing lake traversal remains on the same collision plane. Detailed diving caves, swimming in vertical space and underwater combat are not implemented.
- Original Z and Nyra images untouched.

## Testing

Offline: `python tests/verify_v37_2_oxygen.py`. Real Godot test (requires engine): `python tools/run_tests.py v37_2_oxygen`. Windows release test runner also includes this new suite. No engine runtime test has been executed in this environment.
