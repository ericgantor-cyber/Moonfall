"""UE5 regression checks: real-damage feedback in live menus and safe talent defeat cleanup.

Source tests cannot substitute for native UE5 Windows compilation/playtesting.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
cpp = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
header = (SRC / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")

damage_start = cpp.index("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount) {")
damage_end = cpp.index("void AMoonfallHeroPawn::CycleGraphicsQuality()", damage_start)
damage = cpp[damage_start:damage_end]
real_start = damage.index("if (DamageTaken > 0.f) {")
defeat_start = damage.index("if (Health <= 0.f) {", real_start)
injury = damage[real_start:defeat_start]
defeat = damage[defeat_start:]

# Only actual HP loss can trigger a red menu hit cue, not misses, parries or
# pure stamina-block damage.
assert 'float LastRealDamageAt = -100.f;' in header
assert 'LastRealDamageAt = Now;' in injury
assert 'LastRealDamageAt = Now;' not in damage[:real_start]
assert injury.index('LastRealDamageAt = Now;') < injury.index('SwordBuffer.Clear();')
for state in (
    "bTalentWebOpen = false;",
    "bTalentRespecArmed = false;",
    "TalentRespecArmUntil = -1.f;",
):
    assert state in defeat, ("defeat", state)
assert defeat.index("bTalentWebOpen = false;") < defeat.index("bInventoryOpen = false;")
assert 'PC->bShowMouseCursor = false;' in defeat

respawn_start = cpp.index("void AMoonfallHeroPawn::RespawnAtCheckpoint()")
respawn_end = cpp.index("\n}", respawn_start)
respawn = cpp[respawn_start:respawn_end]
for state in (
    "bTalentWebOpen = false;",
    "bTalentRespecArmed = false;",
    "TalentRespecArmUntil = -1.f;",
):
    assert state in respawn, ("respawn", state)
assert respawn.index("bTalentWebOpen = false;") < respawn.index("LoadWorldProgress()")

# The hostile world is NEVER paused by browsing a menu. All menu variants
# receive the cue, but only for a brief finite-time window.
ui_start = hud.index("const bool bLiveMenuOpen =")
ui_end = hud.index("if (Hero->bTalentWebOpen) {", ui_start)
overlay = hud[ui_start:ui_end]
for modal in (
    "bTalentWebOpen", "bOptionsOpen", "bInventoryOpen",
    "bAtlasOpen", "bResidentContextOpen",
):
    assert f"Hero->{modal}" in overlay, modal
assert 'FMath::IsFinite(Hero->LastRealDamageAt)' in overlay
assert 'FMath::IsFinite(Age) && Age>=0.f && Age<0.8f' in overlay
assert "Hero->bReduceMotion ? 0.12f" in overlay
assert 'DrawRect(HitRed,0.f,0.f,Canvas->SizeX,Canvas->SizeY);' in overlay
assert "Canvas->SizeY-Border" in overlay and "Canvas->SizeX-Border" in overlay
assert "SetGamePaused(" not in overlay
assert overlay.index("const bool bLiveMenuOpen") < overlay.index("DrawRect(HitRed")
print("UE5 live menu damage cue, reduced motion and lethal talent cleanup: source checks passed.")
