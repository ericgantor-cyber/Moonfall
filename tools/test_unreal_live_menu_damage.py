"""UE5 regression checks: real-damage feedback in live menus and safe talent defeat cleanup.

Source tests cannot substitute for native UE5 Windows compilation/playtesting.
"""
from pathlib import Path
import re

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
# The HUD is painter-ordered. A cue drawn before an opaque modal disappears.
# Every modal exit, including tiny-window fallbacks, must draw the cue on top.
assert hud.index("const auto DrawLiveMenuHitCue = [this, Hero]() {") < hud.index("if (Hero->bTalentWebOpen) {")
sections = (
    ("talents", " if (Hero->bTalentWebOpen) {", " // Native Canvas resident menu.", 2),
    ("NPC conversation", " // Native Canvas resident menu.", " if (Hero->bAtlasOpen) {", 2),
    ("atlas", " if (Hero->bAtlasOpen) {", " if (Hero->bOptionsOpen) {", 2),
    ("options", " if (Hero->bOptionsOpen) {", " if (Hero->bInventoryOpen) {", 1),
    ("inventory, sheet, spellbook", " if (Hero->bInventoryOpen) {", " // Reuse a single world scan", 3),
)
for name, begin_marker, end_marker, expected in sections:
    begin = hud.index(begin_marker, hud.index("const auto DrawLiveMenuHitCue"))
    end = hud.index(end_marker, begin)
    part = hud[begin:end]
    indent = r" {2,4}" if name == "NPC conversation" else r" {2,3}"
    completed = re.findall(r"(?m)^(" + indent + r")DrawLiveMenuHitCue\(\);\n\1return;$", part)
    returns = re.findall(r"(?m)^" + indent + r"return;$", part)
    assert len(returns) == len(completed) == expected, (name, len(returns), len(completed))

# End-to-end source contract: the visible red menu damage cue must be
# reachable from actual hostile AI. Menus are not an attack-cancelling gate.
enemy = (SRC / "MoonfallEnemy.cpp").read_text(encoding="utf-8")
tick = enemy.split("void AMoonfallEnemy::Tick(float DeltaSeconds)", 1)[1]
guard = tick.split("// The hostile world stays live while any player menu is open:", 1)[1].split(
    "// A corrupt nonfinite windup", 1
)[0]
assert "Hero->Health <= 0.f" in guard
assert "!Hero->IsPlayerControlled()" in guard
for menu in ("bTalentWebOpen", "bOptionsOpen", "bInventoryOpen",
             "bAtlasOpen", "bCharacterStatsOpen", "bResidentContextOpen",
             "FocusedResident.IsValid()"):
    assert f"Hero->{menu}" not in guard, menu
# Real attack dispatch remains in the same enemy Tick after the guard.
assert tick.index("// A corrupt nonfinite windup") < tick.index("Hero->ApplyMoonfallDamageFrom(")
assert "CancelDivineSignature();" in guard
assert "WindupEndsAt = -1.f;" in guard

print("UE5 live menus: hostile AI, red hit feedback and lethal talent cleanup source checks passed.")
