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
            'TEXT("RESET TALENTS")', 'TEXT("CONFIRM RESET")'):
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
    "TalentWeb": "K", "TalentSpend": "Enter",
    "ControllerConfirm": "Gamepad_FaceButton_Bottom",
    "ControllerBack": "Gamepad_FaceButton_Right",
    "ControllerLeftShoulder": "Gamepad_LeftShoulder",
    "ControllerRightShoulder": "Gamepad_RightShoulder",
}.items():
    assert f'ActionName="{name}",Key={key}' in inputs, (name, key)
for name in ("TalentWeb", "TalentSpend", "ControllerConfirm", "ControllerBack",
             "ControllerLeftShoulder", "ControllerRightShoulder"):
    assert f'Input->BindAction(TEXT("{name}")' in pawn
for side, talent, gear, cast in (
    ("Left", "TalentPreviousBranch", "ReforgeSelectedInventoryItem", "CastLeft"),
    ("Right", "TalentNextBranch", "ForgeFiveSelectedInventoryItem", "CastRight"),
):
    action = f"Controller{side}Shoulder"
    mappings = [line for line in inputs.splitlines()
                if line.startswith("+ActionMappings=") and f"Key=Gamepad_{side}Shoulder" in line]
    assert mappings == [f'+ActionMappings=(ActionName="{action}",Key=Gamepad_{side}Shoulder)']
    assert f"void {action}();" in header
    body = pawn.split(f"void AMoonfallHeroPawn::{action}()", 1)[1].split("\n}", 1)[0]
    routes = ("Health <= 0.f", "FocusedResident.IsValid()",
              f"if (bTalentWebOpen) {{ {talent}(); return; }}",
              f"if (bInventoryOpen) {{ {gear}(); return; }}", f"{cast}();")
    positions = [body.index(route) for route in routes]
    assert positions == sorted(positions)
assert 'ActionName="TalentSpend",Key=Enter' in inputs
a_mappings = [line for line in inputs.splitlines()
              if line.startswith("+ActionMappings=") and "Key=Gamepad_FaceButton_Bottom" in line]
assert a_mappings == ['+ActionMappings=(ActionName="ControllerConfirm",Key=Gamepad_FaceButton_Bottom)']
confirm = pawn.split("void AMoonfallHeroPawn::ControllerConfirm()", 1)[1].split("void AMoonfallHeroPawn::TryJump()", 1)[0]
assert "if (bTalentWebOpen) { TalentSpendSelected(); return; }" in confirm
assert confirm.index("if (bTalentWebOpen)") < confirm.index("TryJump();")
b_mappings = [line for line in inputs.splitlines()
              if line.startswith("+ActionMappings=") and "Key=Gamepad_FaceButton_Right" in line]
assert b_mappings == ['+ActionMappings=(ActionName="ControllerBack",Key=Gamepad_FaceButton_Right)']
assert 'ActionName="Dodge",Key=Q' in inputs
assert 'ActionName="AscendGear",Key=R' in inputs
assert "void ControllerBack();" in header
assert 'BindAction(TEXT("ControllerBack"), IE_Pressed, this, &AMoonfallHeroPawn::ControllerBack);' in pawn
back = pawn.split("void AMoonfallHeroPawn::ControllerBack()", 1)[1].split("void AMoonfallHeroPawn::ControllerConfirm()", 1)[0]
routes = ("if (IsActorBeingDestroyed() || !FMath::IsFinite(Health) || Health <= 0.f) return;",
          "if (bTalentWebOpen) { TalentCloseWeb(); return; }",
          "if (bOptionsOpen || bAtlasOpen || bResidentContextOpen || FocusedResident.IsValid()) { OpenOptions(); return; }",
          "if (bInventoryOpen && (bCharacterStatsOpen || bSpellBookOpen)) { OpenOptions(); return; }",
          "if (bInventoryOpen) { AscendSelectedInventoryItem(); return; }",
          " Dodge();\n}")
indices = [back.index(route) for route in routes]
assert indices == sorted(indices)
assert 'ActionName="TalentRespec",Key=BackSpace' not in inputs
assert 'BindAction(TEXT("TalentRespec")' not in pawn
lt_mappings = [line for line in inputs.splitlines()
               if line.startswith("+ActionMappings=") and "Key=Gamepad_LeftTrigger" in line]
assert lt_mappings == ['+ActionMappings=(ActionName="DualCast",Key=Gamepad_LeftTrigger)']
lt = pawn.split("void AMoonfallHeroPawn::ControllerLeftTrigger()", 1)[1].split("void AMoonfallHeroPawn::ControllerRightTrigger()", 1)[0]
assert "if (bTalentWebOpen) { TalentConfirmRespec(); return; }" in lt
assert lt.index("TalentConfirmRespec(); return;") < lt.index("EquipBestInventoryAction(); return;") < lt.index("CastBoth();")
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
best = pawn.split("int32 AMoonfallHeroPawn::EquipBestInventoryUpgrades()", 1)[1].split("\n}", 1)[0]
assert "if (!bInventoryOpen || bOptionsOpen || Health < 0.f) return 0;" in best
assert "!FMath::IsFinite(Health)" in best
assert "bTalentWebOpen" in best
for method in ("TryCast", "ActivateRacePower"):
    assert "if (bTalentWebOpen) return false;" in method_segment(method, "bool"), method
for method in ("ToggleInventory", "ToggleAtlas", "OpenOptions"):
    assert "if (bTalentWebOpen) { ToggleTalentWeb(); return; }" in method_segment(method), method
assert "if (bTalentWebOpen) { TalentPointerClick(); return; }" in method_segment("ResidentContextMouseClick")
atlas_focus = pawn.split("void AMoonfallHeroPawn::ToggleAtlasChronicleFocus() {", 1)[1].split("void AMoonfallHeroPawn::StepAtlasChronicle(", 1)[0]
assert "if (bTalentWebOpen) return;" in atlas_focus
assert "TalentChangeBranch(" not in atlas_focus
assert "if (!bAtlasOpen || Health <= 0.f || bOptionsOpen) return;" in atlas_focus
assert 'ActionName="ChronicleFocus",Key=Tab' in inputs
r3_mappings = [line for line in inputs.splitlines()
               if line.startswith("+ActionMappings=") and "Key=Gamepad_RightThumbstick" in line]
assert r3_mappings == ['+ActionMappings=(ActionName="ControllerRightStick",Key=Gamepad_RightThumbstick)']
assert 'ActionName="Guard",Key=E' in inputs
assert 'BindAction(TEXT("ControllerRightStick"), IE_Pressed, this, &AMoonfallHeroPawn::ControllerRightStickPress);' in pawn
assert 'BindAction(TEXT("ControllerRightStick"), IE_Released, this, &AMoonfallHeroPawn::ControllerRightStickRelease);' in pawn
r3_press = pawn.split("void AMoonfallHeroPawn::ControllerRightStickPress()", 1)[1].split("\n}", 1)[0]
r3_release = pawn.split("void AMoonfallHeroPawn::ControllerRightStickRelease()", 1)[1].split("\n}", 1)[0]
assert r3_press.index("if (bAtlasOpen) { ToggleAtlasChronicleFocus(); return; }") < r3_press.index("BeginGuard();")
assert r3_press.index("if (bGuarding) return;") < r3_press.index("BeginGuard();")
assert r3_press.index("BeginGuard();") < r3_press.index("bControllerGuardStarted = bGuarding;")
assert r3_release.index("if (!bControllerGuardStarted) return;") < r3_release.index("EndGuard();")
assert "bControllerGuardStarted = false;" in method_segment("EndGuard")
for method, action in (("OptionsUp", "TalentMove(0,-1)"),
                       ("OptionsDown", "TalentMove(0,1)"),
                       ("OptionsLeft", "TalentMove(-1,0)"),
                       ("OptionsRight", "TalentMove(1,0)")):
    assert f"if (bTalentWebOpen) {{ {action}; return; }}" in method_segment(method)
for method in ("ZoomCameraIn", "ZoomCameraOut"):
    assert "if (bTalentWebOpen)" in method_segment(method)
    assert "MoonfallTalentWeb::ForViewport(" in method_segment(method)
    zoom = method_segment(method).split("if (bAtlasOpen)", 1)[0]
    assert zoom.index("bTalentRespecArmed=false;") < zoom.index("return;")
move = pawn.split("void AMoonfallHeroPawn::TalentMove(", 1)[1].split("void AMoonfallHeroPawn::TalentChangeBranch(", 1)[0]
assert "bTalentRespecArmed=false;" in move
npc = method_segment("OpenFocusedResidentContext", "void") + pawn[
    pawn.index("void AMoonfallHeroPawn::OpenFocusedResidentContext(") + 700:
    pawn.index("void AMoonfallHeroPawn::CastRightMouse(", pawn.index("void AMoonfallHeroPawn::OpenFocusedResidentContext("))]
assert "if (bTalentWebOpen) { TalentMove(0,-1); return; }" in npc
# The D-pad Up interaction shortcut must not silently open talents in combat.
# The web opens explicitly through K, then D-pad Up navigates only inside it.
assert 'if (!IsValid(Resident) || !Resident->CanHeroInteract(this)) return;' in npc
assert "ToggleTalentWeb();" not in npc
assert 'ActionName="TalentWeb",Key=K' in inputs
assert 'ActionName="ResidentContext",Key=Gamepad_DPad_Up' in inputs
assert "&& !bTalentWebOpen && !bOptionsOpen && !bInventoryOpen" in pawn
web_start = pawn.index("void AMoonfallHeroPawn::ToggleTalentWeb(")
web_end = pawn.index("int32 AMoonfallHeroPawn::GetAvailableTalentPoints()", web_start)
web = pawn[web_start:web_end]
assert "SwordBuffer.Clear();" in web
assert "GetCharacterMovement()->StopMovementImmediately()" in web
assert "SpendTalentPoint(TalentWebSelected);" in web
assert "RespecTalentPoints();" in web
assert "TalentRespecArmUntil" not in pawn + header + hud
request = web.split("void AMoonfallHeroPawn::TalentConfirmRespec()", 1)[1].split("void AMoonfallHeroPawn::TalentPointerClick()", 1)[0]
assert "bTalentRespecArmed=!bTalentRespecArmed;" in request
assert "RespecTalentPoints();" not in request
confirm_reset = web.split("void AMoonfallHeroPawn::TalentSpendSelected()", 1)[1].split("void AMoonfallHeroPawn::TalentConfirmRespec()", 1)[0]
assert confirm_reset.index("if (bTalentRespecArmed)") < confirm_reset.index("bTalentRespecArmed=false;") < confirm_reset.index("RespecTalentPoints();") < confirm_reset.index("SpendTalentPoint(TalentWebSelected);")
assert 'TEXT("RESET TALENTS")' in hud and 'TEXT("CANCEL")' in hud
assert 'TEXT("CONFIRM RESET")' in hud
assert "MoonfallTalentWeb::HitNode(" in web
pointer = web.split("void AMoonfallHeroPawn::TalentPointerClick()", 1)[1].split("\n}", 1)[0]
branch_tab = pointer.split("if(MoonfallTalentWeb::BranchTab(Layout,B).Hit(X,Y))", 1)[1].split("if (MoonfallTalentWeb::SpendRect", 1)[0]
assert "MoonfallTalentWeb::SameTierInBranch(TalentWebSelected,B)" in branch_tab
assert branch_tab.index("if (NextSelection<0)return;") < branch_tab.index("TalentWebBranch=B;")
assert "MoonfallTalentWeb::FirstTierForSelection(" in branch_tab
assert "TalentWebFirstTier=1;" not in branch_tab
assert "bTalentRespecArmed=false;" in branch_tab
assert "bTalentRespecArmed=false;" in pointer.split("if (Hit>=0)", 1)[1]
print("UE5 talent web: layout, full node UI, mouse/controller input and modal isolation source checks passed.")
