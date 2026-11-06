#include "MoonfallHud.h"
#include "MoonfallPawn.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"

void AMoonfallHud::BeginPlay()
{
    Super::BeginPlay();
    // Import transparent PNG as /Game/UI/MoonfallOrbs in the Unreal editor.
    OrnateOrbs = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/MoonfallOrbs.MoonfallOrbs"));
}

void AMoonfallHud::DrawRing(FVector2D Center, float Radius, float Ratio, const FLinearColor& Color)
{
    if (!Canvas) return;
    const float Bounded = FMath::Clamp(Ratio, 0.f, 1.f);
    constexpr int32 Steps = 48;
    for (int32 I = 0; I < Steps; ++I)
    {
        const float A = (I * 2.f * PI / Steps) - HALF_PI;
        const float B = ((I + 1) * 2.f * PI / Steps) - HALF_PI;
        const FVector2D Start = Center + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius;
        const FVector2D End = Center + FVector2D(FMath::Cos(B), FMath::Sin(B)) * Radius;
        Canvas->K2_DrawLine(Start, End, 5.f, I < FMath::RoundToInt(Bounded * Steps)
            ? Color : FLinearColor(.18f, .18f, .24f, .9f));
    }
}

void AMoonfallHud::DrawMarker(float X, float Y, const FLinearColor& Color)
{
    if (Canvas) Canvas->K2_DrawBox(FVector2D(X - 5.f, Y - 5.f), FVector2D(10.f, 10.f), 2.f, Color);
}

void AMoonfallHud::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner) return;
    const AMoonfallPawn* Hero = Cast<AMoonfallPawn>(PlayerOwner->GetPawn());
    if (!Hero) return;

    if (Hero->IsDefeated())
    {
        // Only show the recovery prompt after defeat, never as permanent HUD clutter.
        const FString DefeatTitle = TEXT("DEFEATED");
        const FString DefeatPrompt = TEXT("Left-click to return to your last checkpoint");
        DrawText(DefeatTitle, FLinearColor(.95f, .2f, .24f),
            Canvas->ClipX * .5f - 75.f, Canvas->ClipY * .38f, nullptr, 1.8f);
        DrawText(DefeatPrompt, FLinearColor::White,
            FMath::Max(12.f, Canvas->ClipX * .5f - 210.f),
            Canvas->ClipY * .46f, nullptr, 1.f);
    }

    const FVector2D OrbCenter(90.f, Canvas->ClipY - 108.f);
    if (OrnateOrbs)
    {
        const float Width = FMath::Min(300.f, Canvas->ClipX * .34f);
        const float Height = Width * static_cast<float>(OrnateOrbs->GetSizeY()) / FMath::Max(1, OrnateOrbs->GetSizeX());
        const float Left = 12.f, Top = Canvas->ClipY - Height - 12.f;
        DrawTexture(OrnateOrbs, Left, Top, Width, Height,
            0.f, 0.f, 1.f, 1.f, FLinearColor::White);
        // Numeric state lives INSIDE the existing orbs, with no extra HUD widgets.
        const float X = Width / 680.f, Y = Height / 424.f;
        const float TextScale = FMath::Clamp(Width / 680.f * 1.8f, .65f, 1.f);
        DrawText(FString::Printf(TEXT("%.0f/%.0f"), Hero->GetHudHealth(), Hero->GetHudMaxHealth()),
            FLinearColor::White, Left + 240.f*X, Top + 158.f*Y, nullptr, TextScale);
        DrawText(FString::Printf(TEXT("%.0f/%.0f"), Hero->GetHudMana(), Hero->GetHudMaxMana()),
            FLinearColor::White, Left + 46.f*X, Top + 235.f*Y, nullptr, TextScale);
        DrawText(FString::Printf(TEXT("%d/%d"), Hero->GetHudExperience(), Hero->GetHudExperienceToNext()),
            FLinearColor::White, Left + 496.f*X, Top + 235.f*Y, nullptr, TextScale);
        DrawText(FString::Printf(TEXT("%.0f/%.0f"), Hero->GetHudStamina(), Hero->GetHudMaxStamina()),
            FLinearColor::White, Left + 280.f*X, Top + 348.f*Y, nullptr, TextScale);
    }
    else
    {
        DrawRing(OrbCenter, 44.f, Hero->GetHudHealth() / FMath::Max(1.f, Hero->GetHudMaxHealth()),
            FLinearColor(1.f, .22f, .30f));
        DrawRing(OrbCenter, 34.f, Hero->GetHudManaRatio(), FLinearColor(.18f, .48f, 1.f));
        DrawRing(OrbCenter, 25.f, Hero->GetHudXpRatio(), FLinearColor(.70f, .40f, 1.f));
    }

    // Temporary combat feedback lives in screen space, not permanent HUD widgets.
    // World-space projection keeps the number attached to the actual hit target.
    for (const AMoonfallPawn::FDamageNumber& Hit : Hero->GetDamageNumbers())
    {
        FVector2D Screen;
        const float T = FMath::Clamp(Hit.Age / AMoonfallPawn::DamageNumberLifetime, 0.f, 1.f);
        const FVector World = Hero->GetActorTransform().TransformPosition(
            Hit.LocalPosition + FVector(0.f, 0.f, 105.f * T));
        if (!PlayerOwner->ProjectWorldLocationToScreen(World, Screen, false)) continue;
        if (Hit.bBlocked || Hit.bParried || Hit.bGuardBroken)
        {
            // Distinct nonverbal guard feedback: teal shield, silver parry burst,
            // or fractured crimson guard. No permanent UI or imported VFX assets.
            constexpr float GuardFlashSeconds = .5f;
            if (Hit.Age >= GuardFlashSeconds) continue;
            const float Fade = 1.f - Hit.Age / GuardFlashSeconds;
            const FLinearColor CueColor = Hit.bParried
                ? FLinearColor(.95f, .95f, 1.f, Fade)
                : (Hit.bGuardBroken ? FLinearColor(1.f, .12f, .2f, Fade)
                                   : FLinearColor(.22f, .75f, .9f, Fade));
            const float HalfWidth = Hit.bParried ? 28.f : 19.f;
            const FVector2D A = Screen + FVector2D(-HalfWidth, -18.f);
            const FVector2D B = Screen + FVector2D( HalfWidth, -18.f);
            const FVector2D C = Screen + FVector2D(0.f, 20.f);
            Canvas->K2_DrawLine(A, B, 3.f * Fade + .5f, CueColor);
            Canvas->K2_DrawLine(B, C, 3.f * Fade + .5f, CueColor);
            Canvas->K2_DrawLine(C, A, 3.f * Fade + .5f, CueColor);
            if (Hit.bParried)
            {
                Canvas->K2_DrawLine(Screen + FVector2D(-35.f, 0.f),
                    Screen + FVector2D(35.f, 0.f), 2.f * Fade + .5f, CueColor);
            }
            else if (Hit.bGuardBroken)
            {
                Canvas->K2_DrawLine(Screen + FVector2D(-8.f, -25.f),
                    Screen + FVector2D(7.f, 27.f), 3.f * Fade + .5f, CueColor);
            }
            continue;
        }
        if (Hit.bDodged)
        {
            // Moonfall's original perfect-evade cue: a brief moonlit crescent
            // and drifting ember trails. No large text, textures, or spawned actors.
            constexpr float EvadeFlashSeconds = 0.48f;
            if (Hit.Age >= EvadeFlashSeconds) continue;
            const float Fade = 1.f - Hit.Age / EvadeFlashSeconds;
            const float Radius = 22.f + 27.f * (1.f - Fade);
            const FLinearColor Moonlight(.86f, .88f, 1.f, Fade * .9f);
            const FLinearColor BloodEmber(.76f, .13f, .27f, Fade * .55f);
            constexpr int32 ArcSegments = 12;
            for (int32 Segment = 0; Segment < ArcSegments; ++Segment)
            {
                const float A = -2.1f + Segment * 3.4f / ArcSegments;
                const float B = -2.1f + (Segment + 1) * 3.4f / ArcSegments;
                const FVector2D Start = Screen + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius;
                const FVector2D End = Screen + FVector2D(FMath::Cos(B), FMath::Sin(B)) * Radius;
                Canvas->K2_DrawLine(Start, End, 3.f * Fade + .5f, Moonlight);
            }
            for (int32 Trail = 0; Trail < 3; ++Trail)
            {
                const float Offset = static_cast<float>(Trail) * 8.f;
                const FVector2D Start = Screen + FVector2D(-10.f - Offset, 12.f + Offset);
                const FVector2D End = Start + FVector2D(-16.f * Fade, 8.f * Fade);
                Canvas->K2_DrawLine(Start, End, 2.f * Fade + .5f, BloodEmber);
            }
            continue;
        }
        const FLinearColor HitColor = Hit.bIncoming
            ? FLinearColor(1.f, .2f, .25f, 1.f - T)
            : (Hit.bHeavy ? FLinearColor(1.f, .8f, .18f, 1.f - T)
            : (Hit.bMagic ? FLinearColor(.3f, .65f, 1.f, 1.f - T)
                          : FLinearColor(1.f, 1.f, 1.f, 1.f - T)));
        const float Scale = Hit.bHeavy ? 1.6f : 1.1f;
        DrawText(FString::FromInt(FMath::RoundToInt(Hit.Amount)),
            HitColor, Screen.X - 12.f, Screen.Y, nullptr, Scale);
    }

    // Short posture bars appear only after combat contact, near damaged foes.
    // No permanent meter panels, character portraits or new world actors.
    for (const AMoonfallPawn::FEnemyPosture& Posture : Hero->GetEnemyPostures())
    {
        FVector2D Screen;
        const FVector World = Hero->GetActorTransform().TransformPosition(
            Posture.LocalPosition + FVector(0.f, 0.f, 205.f));
        if (!PlayerOwner->ProjectWorldLocationToScreen(World, Screen, false)) continue;
        const float Width = Posture.bGuardian ? 65.f : 48.f;
        const float Fill = Width * FMath::Clamp(Posture.Ratio, 0.f, 1.f);
        Canvas->K2_DrawBox(Screen + FVector2D(-Width * .5f, 0.f),
            FVector2D(Width, 4.f), 1.f, FLinearColor(.18f, .16f, .22f, .75f));
        Canvas->K2_DrawBox(Screen + FVector2D(-Width * .5f, 0.f),
            FVector2D(Fill, 4.f), 2.f, Posture.bGuardian
                ? FLinearColor(.84f, .42f, 1.f, .95f)
                : FLinearColor(1.f, .76f, .28f, .95f));
    }

    // Directional danger readout combines a visible windup with a late red
    // impact cue. It lives beside the enemy, never in a permanent HUD panel.
    for (const AMoonfallPawn::FAttackWarning& Warning : Hero->GetAttackWarnings())
    {
        FVector2D Screen;
        const FVector World = Hero->GetActorTransform().TransformPosition(
            Warning.LocalPosition + FVector(0.f, 0.f, Warning.bBoss ? 260.f : 170.f));
        if (!PlayerOwner->ProjectWorldLocationToScreen(World, Screen, false)) continue;
        FVector2D DirectionScreen;
        const FVector DirectionWorld = Hero->GetActorTransform().TransformPosition(
            Warning.LocalPosition + Warning.DirectionToHero * 150.f +
            FVector(0.f, 0.f, Warning.bBoss ? 260.f : 170.f));
        if (!PlayerOwner->ProjectWorldLocationToScreen(DirectionWorld, DirectionScreen, false)) continue;
        const FVector2D Direction = (DirectionScreen - Screen).GetSafeNormal();
        if (Direction.IsNearlyZero()) continue;
        const FVector2D Perpendicular(-Direction.Y, Direction.X);
        const float Urgency = FMath::Clamp(Warning.Urgency, 0.f, 1.f);
        const float Radius = (Warning.bBoss ? 34.f : 22.f) + 4.f * Urgency;
        const float Thickness = Warning.bBoss ? 4.f : 2.5f;
        // Blue/silver warns that an attack can be parried; red means dodge-only.
        // Intensity increases near impact, keeping the distinction readable.
        const FLinearColor Color = Warning.bUnblockable
            ? FLinearColor(1.f, .12f, .23f, .75f + .25f * Urgency)
            : (Urgency >= .72f
                ? FLinearColor(.75f, .91f, 1.f, 1.f)
                : FLinearColor(.2f, .66f, 1.f, .75f + .2f * Urgency));
        constexpr int32 Edges = 4;
        for (int32 Edge = 0; Edge < Edges; ++Edge)
        {
            const float A = (Edge * 2.f * PI / Edges) - HALF_PI;
            const float B = ((Edge + 1) * 2.f * PI / Edges) - HALF_PI;
            const FVector2D Start = Screen + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius;
            const FVector2D End = Screen + FVector2D(FMath::Cos(B), FMath::Sin(B)) * Radius;
            Canvas->K2_DrawLine(Start, End, Thickness, Color);
        }
        // Chevron points along the actual incoming attack direction.
        const FVector2D Tip = Screen + Direction * (Radius + 16.f);
        const FVector2D Base = Screen + Direction * (Radius + 2.f);
        Canvas->K2_DrawLine(Base + Perpendicular * 9.f, Tip, Thickness, Color);
        Canvas->K2_DrawLine(Base - Perpendicular * 9.f, Tip, Thickness, Color);
        if (Warning.bUnblockable)
        {
            // Crossed boss indicator: this strike is dodge-only, not guardable.
            const float XSize = Radius * .65f;
            Canvas->K2_DrawLine(Screen + FVector2D(-XSize, -XSize),
                Screen + FVector2D(XSize, XSize), Thickness, Color);
            Canvas->K2_DrawLine(Screen + FVector2D(-XSize, XSize),
                Screen + FVector2D(XSize, -XSize), Thickness, Color);
        }
    }

    // Only visible while right mouse is held: an aimable defensive chevron.
    if (Hero->IsGuarding())
    {
        const FVector Origin = Hero->GetHeroMapPosition();
        const FVector Direction = Hero->GetGuardDirection();
        const FVector BaseWorld = Hero->GetActorTransform().TransformPosition(
            Origin + FVector(0.f, 0.f, 150.f));
        const FVector TipWorld = Hero->GetActorTransform().TransformPosition(
            Origin + Direction * 170.f + FVector(0.f, 0.f, 150.f));
        FVector2D BaseScreen, TipScreen;
        if (PlayerOwner->ProjectWorldLocationToScreen(BaseWorld, BaseScreen, false) &&
            PlayerOwner->ProjectWorldLocationToScreen(TipWorld, TipScreen, false))
        {
            const FVector2D Aim = (TipScreen - BaseScreen).GetSafeNormal();
            if (!Aim.IsNearlyZero())
            {
                const FVector2D Side(-Aim.Y, Aim.X);
                const FLinearColor GuardColor = Hero->IsParryWindowActive()
                    ? FLinearColor(.95f, .95f, 1.f, .95f)
                    : FLinearColor(.25f, .75f, 1.f, .8f);
                Canvas->K2_DrawLine(TipScreen, TipScreen - Aim * 18.f + Side * 11.f,
                    3.f, GuardColor);
                Canvas->K2_DrawLine(TipScreen, TipScreen - Aim * 18.f - Side * 11.f,
                    3.f, GuardColor);
            }
        }
    }

    // Prototype arena radar, not the production six-region Raven Atlas.
    const float Left = Canvas->ClipX - 178.f;
    Canvas->K2_DrawBox(FVector2D(Left, 18.f), FVector2D(160.f, 160.f), 2.f,
        FLinearColor(.2f, .3f, .4f, .8f));
    const FVector H = Hero->GetHeroMapPosition();
    const FVector B = Hero->GetBossMapPosition();
    // Local radar follows the hero across six continuous world regions.
    for (const FVector& Chamber : Hero->GetChamberMapPositions())
    {
        const FVector Offset = Chamber - H;
        if (FMath::Abs(Offset.X) <= 1700.f && FMath::Abs(Offset.Y) <= 1700.f)
            DrawMarker(Left + 80.f + Offset.X * .04f, 98.f - Offset.Y * .04f,
                FLinearColor(.20f, .85f, .35f));
    }
    for (const FVector& Enemy : Hero->GetEnemyMapPositions())
    {
        const FVector Offset = Enemy - H;
        if (FMath::Abs(Offset.X) <= 1700.f && FMath::Abs(Offset.Y) <= 1700.f)
            DrawMarker(Left + 80.f + Offset.X * .04f, 98.f - Offset.Y * .04f, FLinearColor::Yellow);
    }
    const FVector BossOffset = B - H;
    if (!Hero->IsBossDefeated() &&
        FMath::Abs(BossOffset.X) <= 1700.f && FMath::Abs(BossOffset.Y) <= 1700.f)
        DrawMarker(Left + 80.f + BossOffset.X * .04f,
            98.f - BossOffset.Y * .04f, FLinearColor::Red);
    DrawMarker(Left + 80.f, 98.f, FLinearColor(.2f, .9f, 1.f));
}
