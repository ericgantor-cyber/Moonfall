"""Source-level coverage for the UE5 141-node constellation.
Native Unreal build, mouse layout and controller playtests remain required.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
header = (SRC / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
pawn = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
layout = (SRC / "MoonfallTalentWeb.h").read_text(encoding="utf-8")
inputs = (ROOT / "MoonfallUnreal/Config/DefaultInput.ini").read_text(encoding="utf-8")

assert '#include "MoonfallTalentWeb.h"' in hud
assert '#include "MoonfallTalentWeb.h"' in pawn
for flag in ("bTalentWebOpen", "TalentWebBranch", "TalentWebSelected",
             "TalentWebFirstTier", "bTalentRespecArmed"):
    assert flag in header
assert "MoonfallTalents::NodeCount" in layout
assert "inline int HitNode(" in layout
assert "inline Rect NodeRect(" in layout
assert "inline int Move(" in layout
assert "MoonfallTalentWeb::ForViewport(Canvas->SizeX,Canvas->SizeY" in hud
assert "if (Hero->bTalentWebOpen) {" in hud
assert hud.index("if (Hero->bTalentWebOpen) {") < hud.index("if (const AMoonfallWorldNPC* Resident")
for key in ("Hero->GetAvailableTalentPoints()", "Hero->GetTalentRank(I)",
            "Hero->TalentWebSelected", 'TEXT("SPEND POINT")',
            'TEXT("RESET ALL")', 'TEXT("CONFIRM RESET")'):
    assert key in hud, key
assert "Hero->TalentRanks.Num()" not in hud, "Fixed rank array must not use Unreal TArray API"
# Might is MELEE DAMAGE, not a sword-only hit stat. UE5 MeleeAttack
# also handles Tier 0 (unarmed), with the same validated multiplier.
assert 'Selected.AffectedStat==MoonfallTalents::Stat::MeleeDamage' in hud
assert 'MELEE DAMAGE +%.1f%%/RANK' in hud
assert 'const float Tier = bWeaponOwned' in pawn
assert 'const float TalentMeleeMultiplier = static_cast<float>(' in pawn
assert '* (1.f + GetVersatilityBonus()) * TalentMeleeMultiplier;' in pawn
# Every category, including flat resource caps and percent damage bonuses,
# must have a readable selected-node effect preview.
for stat in ("Health", "Mana", "Stamina", "MeleeDamage", "RangedDamage", "MagicDamage"):
    assert f"case MoonfallTalents::Stat::{stat}:" in hud, stat
for label in ("HEALTH", "MANA", "STAMINA", "MELEE DAMAGE",
              "RANGED DAMAGE", "MAGIC DAMAGE"):
    assert f'TEXT("{label}")' in hud, label
assert "Selected.AmountPerRank*(Selected.IsPercent?100.0:1.0)" in hud
assert "Selected.IsPercent?TEXT(\"%\"):TEXT(\"\")" in hud
# Buffered sword/melee input must never survive into the talent UI or
# trigger its queued dispatch while the modal is showing.
buffer_start = pawn.index("const bool bCanDispatchBufferedMelee =")
buffer_end = pawn.index("SwordBuffer.Consume(CombatNow", buffer_start)
assert "!bTalentWebOpen" in pawn[buffer_start:buffer_end]
assert buffer_start < buffer_end



for name, key in {
    "TalentWeb": "K", "TalentSpend": "Enter", "TalentRespec": "BackSpace",
    "TalentSpend": "Gamepad_FaceButton_Bottom",
    "TalentClose": "Gamepad_FaceButton_Right",
    "TalentBranchPrevious": "Gamepad_LeftShoulder",
    "TalentBranchNext": "Gamepad_RightShoulder",
}.items():
    assert f'ActionName="{name}",Key={key}' in inputs, (name, key)
for name in ("TalentWeb", "TalentSpend", "TalentRespec", "TalentClose",
             "TalentBranchPrevious", "TalentBranchNext"):
    assert f'Input->BindAction(TEXT("{name}")' in pawn
assert 'ActionName="TalentSpend",Key=Enter' in inputs
assert 'ActionName="TalentRespec",Key=Gamepad_LeftTrigger' in inputs
# The keyboard arrows and WASD still move the character ONLY.
for line in inputs.splitlines():
    if 'ActionMappings=(ActionName="Talent' in line:
        assert not any("Key=" + x + ")" in line for x in
                       ("W", "A", "S", "D", "Up", "Down", "Left", "Right", "Q", "E"))

def method_segment(name, kind="void"):
    start = pawn.index(f"{kind} AMoonfallHeroPawn::{name}(")
    return pawn[start : start + 1700]

for method in ("TryJump", "MoveForward", "MoveRight",
               "MeleeAttack", "RightHandAction", "CastLeft", "CastRight",
               "CastBoth", "CastRightMouse", "Dodge", "BeginGuard",
               "BeginSprint", "PickupNearestGroundLoot", "ToggleSpellBook",
               "ToggleSpellBookShortcut", "ToggleCharacterStats",
               "AscendSelectedInventoryItem", "ReforgeSelectedInventoryItem",
               "ForgeFiveSelectedInventoryItem", "UpgradeSelectedInventoryItem"):
    assert "if (bTalentWebOpen) return;" in method_segment(method), method
for camera in ("Turn", "LookUp"):
    assert "&& !bTalentWebOpen && !bResidentContextOpen" in method_segment(camera), camera
# Controller LT's Equip Best action can never apply outside the inventory.
assert "void AMoonfallHeroPawn::EquipBestInventoryAction() { if (!bCharacterStatsOpen && !bSpellBookOpen) EquipBestInventoryUpgrades(); }" in pawn
assert "if (!bInventoryOpen || bOptionsOpen || Health <= 0.f) return 0;" in pawn
for method in ("TryCast", "ActivateRacePower"):
    assert "if (bTalentWebOpen) return false;" in method_segment(method, "bool"), method
for method in ("ToggleInventory", "ToggleAtlas", "OpenOptions"):
    assert "if (bTalentWebOpen) { ToggleTalentWeb(); return; }" in method_segment(method), method
assert "if (bTalentWebOpen) { TalentPointerClick(); return; }" in method_segment("ResidentContextMouseClick")
assert "if (bTalentWebOpen) { TalentChangeBranch(1); return; }" in method_segment("ToggleAtlasChronicleFocus")
for method, action in (("OptionsUp", "TalentMove(0,-1)"),
                       ("OptionsDown", "TalentMove(0,1)"),
                       ("OptionsLeft", "TalentMove(-1,0)"),
                       ("OptionsRight", "TalentMove(1,0)")):
    assert f"if (bTalentWebOpen) {{ {action}; return; }}" in method_segment(method)
for method in ("ZoomCameraIn", "ZoomCameraOut"):
    assert "if (bTalentWebOpen)" in method_segment(method)
    assert "MoonfallTalentWeb::ForViewport(" in method_segment(method)
npc = method_segment("OpenFocusedResidentContext", "void") + pawn[
    pawn.index("void AMoonfallHeroPawn::OpenFocusedResidentContext(") + 700:
    pawn.index("void AMoonfallHeroPawn::CastRightMouse(", pawn.index("void AMoonfallHeroPawn::OpenFocusedResidentContext("))]
assert "if (bTalentWebOpen) { TalentMove(0,-1); return; }" in npc
assert "ToggleTalentWeb();" in npc
assert "&& !bTalentWebOpen && !bOptionsOpen && !bInventoryOpen" in pawn
web_start = pawn.index("void AMoonfallHeroPawn::ToggleTalentWeb(")
web_end = pawn.index("int32 AMoonfallHeroPawn::GetAvailableTalentPoints()", web_start)
web = pawn[web_start:web_end]
assert "SwordBuffer.Clear();" in web
assert "GetCharacterMovement()->StopMovementImmediately()" in web
assert "SpendTalentPoint(TalentWebSelected);" in web
assert "RespecTalentPoints();" in web
assert "TalentRespecArmUntil=Now+3.0f;" in web
assert "MoonfallTalentWeb::HitNode(" in web
print("UE5 talent web: layout, full node UI, mouse/controller input and modal isolation source checks passed.")
