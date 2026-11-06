#include "MoonfallHud.h"
#include "MoonfallPawn.h"
#include "MoonfallWorldRules.h"

// Map terrain is computed from the exact floor footprints used by BuildWorld.
// These are schematic ground regions, not generated or imported terrain textures.
namespace
{
int32 TerrainRegionAt(const FVector2D& World)
{
    // A floor or side chamber extends at most 2,000 cm from its region
    // center. Only the nearest region and its two neighbors can contain it.
    // This avoids searching all six regions for every minimap terrain sample.
    const int32 NearestRegion = FMath::RoundToInt(
        World.X / MoonfallWorldRules::RegionSpacing);
    const int32 FirstRegion = FMath::Max(0, NearestRegion - 1);
    const int32 LastRegion = FMath::Min(
        MoonfallWorldRules::RegionCount - 1, NearestRegion + 1);
    for (int32 Region = FirstRegion; Region <= LastRegion; ++Region)
    {
        const float CenterX = MoonfallWorldRules::RegionCenter(Region);
        if (FMath::Abs(World.X - CenterX) <= MoonfallWorldRules::HalfRegion &&
            FMath::Abs(World.Y) <= MoonfallWorldRules::HalfRegion)
            return Region;
        for (int32 Chamber = 0; Chamber < MoonfallWorldRules::ChambersPerRegion; ++Chamber)
        {
            const FVector Center = MoonfallWorldRules::ChamberCenter(Region, Chamber);
            if (FMath::Abs(World.X - Center.X) <= 800.f &&
                FMath::Abs(World.Y - Center.Y) <= 800.f)
                return Region;
        }
    }
    return INDEX_NONE;
}

FLinearColor TerrainTint(int32 Region)
{
    // Quiet terrain shades leave the original green chamber and red boss icons clear.
    static const FLinearColor Colors[] = {
        FLinearColor(.18f, .30f, .25f, .75f),
        FLinearColor(.22f, .30f, .36f, .75f),
        FLinearColor(.34f, .27f, .20f, .75f),
        FLinearColor(.26f, .23f, .34f, .75f),
        FLinearColor(.35f, .22f, .28f, .75f),
        FLinearColor(.31f, .27f, .37f, .75f)
    };
    return Colors[FMath::Clamp(Region, 0, MoonfallWorldRules::RegionCount - 1)];
}
}

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

// Original low-detail silhouettes inspired by classic fantasy exploration maps.
// One painter keeps world-map and minimap icons visually consistent.
void AMoonfallHud::DrawMapIcon(FVector2D Center, float Scale, int32 IconKind)
{
    if (!Canvas) return;
    const FLinearColor Outline(.10f, .11f, .17f, 1.f);
    const FLinearColor Color = IconKind == 0
        ? FLinearColor(.48f, .92f, .62f)
        : (IconKind == 1 ? FLinearColor(.99f, .56f, .38f)
        : (IconKind == 3 ? FLinearColor(.52f, .56f, .63f)
                         : FLinearColor(.43f, .87f, 1.f)));
    const auto P = [Center, Scale](float X, float Y)
    {
        return Center + FVector2D(X * Scale, Y * Scale);
    };
    if (IconKind == 0 || IconKind == 3)
    {
        // Cave: arch outline and dark mouth, no copyrighted asset.
        // Kind 3 uses a grey silhouette once the cave reward is claimed.
        Canvas->K2_DrawBox(P(-3.f, -1.f), FVector2D(6.f, 6.f) * Scale,
            1.f, Outline);
        Canvas->K2_DrawLine(P(-6.f, 5.f), P(-6.f, -1.f), 2.f, Color);
        Canvas->K2_DrawLine(P(-6.f, -1.f), P(-3.f, -6.f), 2.f, Color);
        Canvas->K2_DrawLine(P(-3.f, -6.f), P(3.f, -6.f), 2.f, Color);
        Canvas->K2_DrawLine(P(3.f, -6.f), P(6.f, -1.f), 2.f, Color);
        Canvas->K2_DrawLine(P(6.f, -1.f), P(6.f, 5.f), 2.f, Color);
        Canvas->K2_DrawLine(P(-6.f, 5.f), P(6.f, 5.f), 2.f, Color);
    }
    else if (IconKind == 1)
    {
        // Boss: small spiked crown, distinct from cave entrances.
        Canvas->K2_DrawLine(P(-6.f, -4.f), P(-4.f, 5.f), 2.f, Color);
        Canvas->K2_DrawLine(P(-6.f, -4.f), P(0.f, 0.f), 2.f, Color);
        Canvas->K2_DrawLine(P(0.f, 0.f), P(0.f, -7.f), 2.f, Color);
        Canvas->K2_DrawLine(P(0.f, -7.f), P(4.f, 0.f), 2.f, Color);
        Canvas->K2_DrawLine(P(4.f, 0.f), P(6.f, -4.f), 2.f, Color);
        Canvas->K2_DrawLine(P(6.f, -4.f), P(4.f, 5.f), 2.f, Color);
        Canvas->K2_DrawLine(P(-4.f, 5.f), P(4.f, 5.f), 2.f, Color);
    }
    else
    {
        // Player: simple circular locator, never a directional arrow.
        constexpr int32 Segments = 12;
        for (int32 I = 0; I < Segments; ++I)
        {
            const float A = 2.f * PI * I / Segments;
            const float B = 2.f * PI * (I + 1) / Segments;
            Canvas->K2_DrawLine(P(FMath::Cos(A) * 5.f, FMath::Sin(A) * 5.f),
                P(FMath::Cos(B) * 5.f, FMath::Sin(B) * 5.f), 1.5f, Color);
        }
        Canvas->K2_DrawBox(P(-2.f, -2.f), FVector2D(4.f, 4.f) * Scale,
            1.f, Color);
    }
}

void AMoonfallHud::DrawMapFocusRing(FVector2D Center, float Radius)
{
    // Quiet gold outline highlights the nearest unclaimed cave without arrows.
    if (!Canvas) return;
    constexpr int32 Segments = 16;
    for (int32 I = 0; I < Segments; ++I)
    {
        const float A = I * (2.f * PI / Segments);
        const float B = (I + 1) * (2.f * PI / Segments);
        Canvas->K2_DrawLine(Center + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius,
            Center + FVector2D(FMath::Cos(B), FMath::Sin(B)) * Radius,
            1.5f, FLinearColor(.98f, .79f, .36f));
    }
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

    // A temporary boss posture bar appears above the boss only when pressured.
    if (Hero->IsBossPostureActive() || Hero->IsBossStaggered())
    {
        FVector2D Screen;
        const FVector BossWorld = Hero->GetActorTransform().TransformPosition(
            Hero->GetBossMapPosition() + FVector(0.f, 0.f, 310.f));
        if (PlayerOwner->ProjectWorldLocationToScreen(BossWorld, Screen, false))
        {
            constexpr float Width = 100.f;
            const float Fill = Width * Hero->GetBossPostureRatio();
            const FLinearColor Color = Hero->IsBossStaggered()
                ? FLinearColor(1.f, .95f, 1.f, .95f)
                : FLinearColor(.72f, .38f, 1.f, .95f);
            Canvas->K2_DrawBox(Screen + FVector2D(-Width * .5f, 0.f),
                FVector2D(Width, 6.f), 1.f, FLinearColor(.18f, .16f, .22f, .75f));
            if (Hero->IsBossStaggered())
                Canvas->K2_DrawBox(Screen + FVector2D(-Width * .5f, 0.f),
                    FVector2D(Width, 6.f), 2.f, Color);
            else
                Canvas->K2_DrawBox(Screen + FVector2D(-Width * .5f, 0.f),
                    FVector2D(Fill, 6.f), 2.f, Color);
        }
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

    // Compact WoW-style circular minimap, using Moonfall's own shapes and colors.
    // No textures, extra HUD panels or spawned objects. North stays at the top.
    constexpr float RadarRadius = 72.f;
    const float WorldRadius = Hero->GetMinimapWorldRadius();
    constexpr int32 RadarSegments = 48;
    const FVector2D RadarCenter(Canvas->ClipX - 94.f, 94.f);
    const float RadarScale = (RadarRadius - 9.f) / WorldRadius;
    const FLinearColor RadarRim(.64f, .57f, .82f, .92f);
    // Track cave rewards, the boss, both or neither. No enemy tracking.
    const int32 TrackingMode = Hero->GetMinimapTrackingMode();
    const bool bTrackChambers = TrackingMode == 0 || TrackingMode == 1;
    const bool bTrackBoss = TrackingMode == 0 || TrackingMode == 2;
    // Low-cost north-up terrain raster. Runs of matching ground are merged,
    // and every sample is clipped inside the circular minimap's inner radius.
    const FVector H = Hero->GetHeroMapPosition();
    constexpr float GroundLimit = 61.f;
    constexpr float GroundStep = 7.f;
    // Fill the round map with a subdued background first, separating
    // traversable ground from empty space and busy combat scenery.
    for (float Y = -GroundLimit; Y <= GroundLimit; Y += GroundStep)
    {
        const float RadiusAtY = FMath::Sqrt(FMath::Max(0.f,
            FMath::Square(GroundLimit) - FMath::Square(Y)));
        Canvas->K2_DrawLine(RadarCenter + FVector2D(-RadiusAtY, Y),
            RadarCenter + FVector2D(RadiusAtY, Y), GroundStep,
            FLinearColor(.045f, .055f, .085f, .88f));
    }
    for (float Y = -GroundLimit; Y <= GroundLimit; Y += GroundStep)
    {
        const float RowRadius = FMath::Sqrt(FMath::Max(0.f,
            FMath::Square(GroundLimit) - FMath::Square(Y)));
        int32 ActiveRegion = INDEX_NONE;
        float RunStart = 0.f;
        float RunEnd = 0.f;
        for (float X = -GroundLimit; X <= GroundLimit + GroundStep; X += GroundStep)
        {
            const bool bInside = X <= GroundLimit &&
                FMath::Abs(X) + GroundStep * .5f <= RowRadius;
            const FVector2D WorldSample(H.X + X / RadarScale,
                H.Y - Y / RadarScale);
            const int32 Region = bInside && Hero->IsMapExplored(WorldSample)
                ? TerrainRegionAt(WorldSample) : INDEX_NONE;
            if (Region != ActiveRegion)
            {
                if (ActiveRegion != INDEX_NONE)
                    Canvas->K2_DrawLine(RadarCenter + FVector2D(RunStart, Y),
                        RadarCenter + FVector2D(RunEnd, Y),
                        GroundStep, TerrainTint(ActiveRegion));
                ActiveRegion = Region;
                RunStart = X;
            }
            RunEnd = X;
        }
    }

    for (int32 Segment = 0; Segment < RadarSegments; ++Segment)
    {
        const float A = Segment * 2.f * PI / RadarSegments;
        const float B = (Segment + 1) * 2.f * PI / RadarSegments;
        Canvas->K2_DrawLine(
            RadarCenter + FVector2D(FMath::Cos(A), FMath::Sin(A)) * RadarRadius,
            RadarCenter + FVector2D(FMath::Cos(B), FMath::Sin(B)) * RadarRadius,
            2.f, RadarRim);
    }
    // A fixed north tick and four faint range spokes aid orientation.
    const FVector2D North(0.f, -1.f);
    Canvas->K2_DrawLine(RadarCenter + North * (RadarRadius - 11.f),
        RadarCenter + North * (RadarRadius + 5.f), 3.f, FLinearColor(.95f, .84f, .44f));
    // Visible compass letter, fixed north-up to match world XY coordinates.
    DrawText(TEXT("N"), FLinearColor(.95f, .84f, .44f),
        RadarCenter.X - 5.f, RadarCenter.Y - RadarRadius - 15.f, nullptr, .9f);
    // Tracking readout and range indicator stay below the circular minimap.
    const TCHAR* TrackingLabels[] = { TEXT("ALL"), TEXT("CAVES"), TEXT("BOSS"), TEXT("OFF") };
    DrawText(FString::Printf(TEXT("T: %s  %.1fm"),
        TrackingLabels[FMath::Clamp(TrackingMode, 0, 3)], WorldRadius / 100.f),
        FLinearColor(.8f, .85f, .95f), RadarCenter.X - 72.f,
        RadarCenter.Y + RadarRadius + 5.f, nullptr, .75f);
    // Actual clickable HUD controls. Hit regions match Pawn::TryHandleMinimapZoomClick.
    // Draw distinct enabled/disabled states at the minimum and maximum range.
    const FVector2D ZoomPlus(RadarCenter.X + 58.f, 67.f);
    const FVector2D ZoomMinus(RadarCenter.X + 58.f, 99.f);
    // Large 30x30 clickable squares match the controller hit tests.
    Canvas->K2_DrawBox(ZoomPlus - FVector2D(15.f, 15.f),
        FVector2D(30.f, 30.f), 2.f, FLinearColor(.65f, .57f, .82f));
    Canvas->K2_DrawBox(ZoomMinus - FVector2D(15.f, 15.f),
        FVector2D(30.f, 30.f), 2.f, FLinearColor(.65f, .57f, .82f));
    const FLinearColor PlusColor = Hero->GetMinimapZoomLevel() < 2
        ? FLinearColor::White : FLinearColor(.42f, .42f, .48f);
    const FLinearColor MinusColor = Hero->GetMinimapZoomLevel() > 0
        ? FLinearColor::White : FLinearColor(.42f, .42f, .48f);
    Canvas->K2_DrawLine(ZoomPlus + FVector2D(-6.f, 0.f),
        ZoomPlus + FVector2D(6.f, 0.f), 2.5f, PlusColor);
    Canvas->K2_DrawLine(ZoomPlus + FVector2D(0.f, -6.f),
        ZoomPlus + FVector2D(0.f, 6.f), 2.5f, PlusColor);
    Canvas->K2_DrawLine(ZoomMinus + FVector2D(-6.f, 0.f),
        ZoomMinus + FVector2D(6.f, 0.f), 2.5f, MinusColor);
    for (int32 Axis = 0; Axis < 4; ++Axis)
    {
        const float A = Axis * HALF_PI;
        const FVector2D Direction(FMath::Cos(A), FMath::Sin(A));
        Canvas->K2_DrawLine(RadarCenter + Direction * 10.f,
            RadarCenter + Direction * (RadarRadius - 9.f), 1.f,
            FLinearColor(.31f, .31f, .47f, .42f));
    }
    const FVector B = Hero->GetBossMapPosition();
    // All markers must remain inside the circular frame, not its bounding box.
    if (bTrackChambers)
    {
    float NearestVisibleCaveSq = TNumericLimits<float>::Max();
    FVector2D NearestVisibleCavePoint = RadarCenter;
    bool bHasVisibleCave = false;
    for (const FVector& Chamber : Hero->GetChamberMapPositions())
    {
        const FVector Offset = Chamber - H;
        const FVector2D Local(Offset.X * RadarScale, -Offset.Y * RadarScale);
        if (!Hero->IsMapExplored(FVector2D(Chamber.X, Chamber.Y))) continue;
        if (Local.SizeSquared() > FMath::Square(RadarRadius - 10.f)) continue;
        const FVector2D P = RadarCenter + Local;
        // Cave mouth icon: an arch and dark entrance instead of a loot diamond.
        DrawMapIcon(P, 1.f, 0);
        const float DistanceSq = Offset.SizeSquared2D();
        if (DistanceSq < NearestVisibleCaveSq)
        {
            NearestVisibleCaveSq = DistanceSq;
            NearestVisibleCavePoint = P;
            bHasVisibleCave = true;
        }
    }
    if (bHasVisibleCave) DrawMapFocusRing(NearestVisibleCavePoint, 10.f);
    // Grey cleared caves remain as discovered landmarks, not active objectives.
    for (int32 Region = 0; Region < MoonfallWorldRules::RegionCount; ++Region)
    {
        for (int32 Cave = 0; Cave < MoonfallWorldRules::ChambersPerRegion; ++Cave)
        {
            if (!Hero->IsCaveCleared(Region, Cave)) continue;
            const FVector Position = MoonfallWorldRules::ChamberReward(Region, Cave);
            if (!Hero->IsMapExplored(FVector2D(Position.X, Position.Y))) continue;
            const FVector Offset = Position - H;
            const FVector2D Local(Offset.X * RadarScale, -Offset.Y * RadarScale);
            if (Local.SizeSquared() > FMath::Square(RadarRadius - 10.f)) continue;
            DrawMapIcon(RadarCenter + Local, 1.f, 3);
        }
    }
    }
    if (bTrackBoss && !Hero->IsBossDefeated() &&
        Hero->IsMapExplored(FVector2D(B.X, B.Y)))
    {
        const FVector BossOffset = B - H;
        const FVector2D Local(BossOffset.X * RadarScale, -BossOffset.Y * RadarScale);
        if (Local.SizeSquared() <= FMath::Square(RadarRadius - 10.f))
        {
            const FVector2D P = RadarCenter + Local;
            DrawMapIcon(P, 1.f, 1);
        }

    }
    // Minimal centered player dot; no directional arrows or enemy markers.
    DrawMapIcon(RadarCenter, 1.f, 2);
    // Tab opens a wider terrain map of the same continuous six-region world.
    // Draw the exact 34 m region floors and 16 m connected cave floor footprints.
    if (Hero->IsWorldMapVisible())
    {
        const float MapWidth = FMath::Max(200.f, FMath::Min(760.f, Canvas->ClipX - 32.f));
        const float MapHeight = FMath::Max(120.f, FMath::Min(290.f, Canvas->ClipY - 100.f));
        const FVector2D MapOrigin((Canvas->ClipX - MapWidth) * .5f,
            (Canvas->ClipY - MapHeight) * .5f);
        // Opaque-ish slate backing makes the terrain legible above live combat.
        DrawRect(FLinearColor(.035f, .045f, .075f, .93f),
            MapOrigin.X, MapOrigin.Y, MapWidth, MapHeight);
        Canvas->K2_DrawBox(MapOrigin, FVector2D(MapWidth, MapHeight), 3.f,
            FLinearColor(.75f, .68f, .86f));
        // Map world extents cover every floor and the final boss, with margins.
        constexpr float WorldMinX = -1850.f, WorldMaxX = 17850.f;
        constexpr float WorldHalfY = 3400.f;
        const float MapScaleX = (MapWidth - 24.f) / (WorldMaxX - WorldMinX);
        const float MapScaleY = (MapHeight - 44.f) / (2.f * WorldHalfY);
        const FVector2D MapMiddle(MapOrigin.X + 12.f, MapOrigin.Y + MapHeight * .5f);
        const auto ToMap = [&](float X, float Y) -> FVector2D
        {
            return FVector2D(MapMiddle.X + (X - WorldMinX) * MapScaleX,
                MapMiddle.Y - Y * MapScaleY);
        };
        // Row-run batching: draw each contiguous explored terrain shade as
        // one rectangle instead of thousands of individual 2 m tiles.
        // This preserves blank unexplored cells and scales better on full maps.
        int32 DiscoveredGroundCells = 0;
        int32 TotalWalkableCells = 0;
        int32 RevealedByRegion[MoonfallWorldRules::RegionCount] = {};
        int32 TotalByRegion[MoonfallWorldRules::RegionCount] = {};
        for (int32 Y = 0; Y < AMoonfallPawn::FogHeight; ++Y)
        {
            const float WorldY = AMoonfallPawn::FogMinY +
                (Y + .5f) * AMoonfallPawn::FogCellSize;
            int32 ActiveRegion = INDEX_NONE;
            int32 RunStartX = 0;
            for (int32 X = 0; X <= AMoonfallPawn::FogWidth; ++X)
            {
                int32 Region = INDEX_NONE;
                if (X < AMoonfallPawn::FogWidth)
                {
                    const float WorldX = AMoonfallPawn::FogMinX +
                        (X + .5f) * AMoonfallPawn::FogCellSize;
                    // Count only actual floor cells. Empty world grid cells
                    // must not inflate the world or regional completion ratios.
                    const FVector2D WorldCell(WorldX, WorldY);
                    const int32 TerrainRegion = TerrainRegionAt(WorldCell);
                    if (TerrainRegion != INDEX_NONE)
                    {
                        ++TotalWalkableCells;
                        ++TotalByRegion[TerrainRegion];
                        if (Hero->IsMapExplored(WorldCell))
                        {
                            Region = TerrainRegion;
                            ++DiscoveredGroundCells;
                            ++RevealedByRegion[TerrainRegion];
                        }
                    }
                }
                if (Region == ActiveRegion) continue;
                if (ActiveRegion != INDEX_NONE)
                {
                    const float StartWorldX = AMoonfallPawn::FogMinX +
                        RunStartX * AMoonfallPawn::FogCellSize;
                    const FVector2D TopLeft = ToMap(StartWorldX,
                        WorldY + AMoonfallPawn::FogCellSize * .5f);
                    DrawRect(TerrainTint(ActiveRegion), TopLeft.X, TopLeft.Y,
                        (X - RunStartX) * AMoonfallPawn::FogCellSize * MapScaleX,
                        AMoonfallPawn::FogCellSize * MapScaleY);
                }
                ActiveRegion = Region;
                RunStartX = X;
            }
        }
        // Labels only appear once the center of a region has been discovered.
        for (int32 Region = 0; Region < MoonfallWorldRules::RegionCount; ++Region)
        {
            const float X = MoonfallWorldRules::RegionCenter(Region);
            if (!Hero->IsMapExplored(FVector2D(X, 0.f))) continue;
            const FVector2D TitlePoint = ToMap(X, -1200.f);
            DrawText(FString::Printf(TEXT("%d"), Region + 1),
                FLinearColor(.90f, .86f, .96f), TitlePoint.X - 4.f,
                TitlePoint.Y, nullptr, .8f);
        }
        // Cave discovery statistics use the same fog checks as map icons.
        // Never count a cave as found merely because its location exists in world data.
        int32 DiscoveredCaves = 0;
        int32 ClearedCaves = 0;
        for (int32 Region = 0; Region < MoonfallWorldRules::RegionCount; ++Region)
        {
            for (int32 Cave = 0; Cave < MoonfallWorldRules::ChambersPerRegion; ++Cave)
            {
                const FVector CavePosition = MoonfallWorldRules::ChamberReward(Region, Cave);
                if (!Hero->IsMapExplored(FVector2D(CavePosition.X, CavePosition.Y)))
                    continue;
                ++DiscoveredCaves;
                if (Hero->IsCaveCleared(Region, Cave)) ++ClearedCaves;
            }
        }
        // Tracking filters only affect the icons, never the terrain.
        if (bTrackChambers)
        {
            float NearestWorldCaveSq = TNumericLimits<float>::Max();
            FVector2D NearestWorldCavePoint = MapOrigin;
            bool bHasWorldCave = false;
            for (const FVector& Chamber : Hero->GetWorldMapChamberPositions())
            {
                if (!Hero->IsMapExplored(FVector2D(Chamber.X, Chamber.Y))) continue;
                const FVector2D P = ToMap(Chamber.X, Chamber.Y);
                DrawMapIcon(P, .60f, 0);
                const float DistanceSq = FVector::DistSquared2D(Chamber, H);
                if (DistanceSq < NearestWorldCaveSq)
                {
                    NearestWorldCaveSq = DistanceSq;
                    NearestWorldCavePoint = P;
                    bHasWorldCave = true;
                }
            }
            if (bHasWorldCave)
            {
                DrawMapFocusRing(NearestWorldCavePoint, 8.f);
                DrawText(FString::Printf(TEXT("NEAREST CAVE: %.0fm"),
                    FMath::Sqrt(NearestWorldCaveSq) / 100.f),
                    FLinearColor(.98f, .79f, .36f), MapOrigin.X + 10.f,
                    MapOrigin.Y + 25.f, nullptr, .8f);
            }
        }
        // Grey cave landmarks remain visible only after both discovery and clearing.
        // Claimed rewards are never treated as the nearest available cave.
        if (bTrackChambers)
        {
            for (int32 Region = 0; Region < MoonfallWorldRules::RegionCount; ++Region)
            {
                for (int32 Cave = 0; Cave < MoonfallWorldRules::ChambersPerRegion; ++Cave)
                {
                    if (!Hero->IsCaveCleared(Region, Cave)) continue;
                    const FVector Position = MoonfallWorldRules::ChamberReward(Region, Cave);
                    if (!Hero->IsMapExplored(FVector2D(Position.X, Position.Y))) continue;
                    DrawMapIcon(ToMap(Position.X, Position.Y), .60f, 3);
                }
            }
        }
        // The world map obeys the same fog gate as the circular minimap.
        // Tracking a boss must never reveal its location before discovery.
        if (bTrackBoss && !Hero->IsBossDefeated() &&
            Hero->IsMapExplored(FVector2D(B.X, B.Y)))
        {
            const FVector2D BossPoint = ToMap(B.X, B.Y);
            DrawMapIcon(BossPoint, .9f, 1);
        }
        const FVector2D You = ToMap(H.X, H.Y);
        DrawMapIcon(You, 1.f, 2);
        DrawText(TEXT("N"), FLinearColor(.95f, .84f, .44f),
            MapOrigin.X + MapWidth * .5f, MapOrigin.Y + 4.f, nullptr, 1.f);
        DrawText(FString::Printf(TEXT("REGION %d / %d"),
            MoonfallWorldRules::RegionAt(H.X) + 1, MoonfallWorldRules::RegionCount),
            FLinearColor(.95f, .84f, .55f),
            MapOrigin.X + 10.f, MapOrigin.Y + 6.f, nullptr, .95f);
        DrawText(TEXT("GREEN: CAVE   GREY: CLEARED   RED: BOSS"),
            FLinearColor(.86f, .86f, .91f),
            MapOrigin.X + 10.f, MapOrigin.Y + MapHeight - 37.f, nullptr, .70f);
        // Compact exploration statistics; these never expose unexplored locations.
        DrawText(FString::Printf(TEXT("CAVES FOUND: %d/%d   CLEARED: %d"),
            DiscoveredCaves, MoonfallWorldRules::TotalChambers, ClearedCaves),
            FLinearColor(.86f, .92f, .82f), MapOrigin.X + 10.f,
            MapOrigin.Y + MapHeight - 57.f, nullptr, .70f);
        // Percentages use discovered walkable floors, never the empty fog grid.
        // No hidden cave coordinates or boss locations are exposed by totals.
        const int32 ActiveRegionIndex = FMath::Clamp(
            MoonfallWorldRules::RegionAt(H.X), 0, MoonfallWorldRules::RegionCount - 1);
        const int32 WorldPercent = TotalWalkableCells > 0
            ? FMath::Clamp(FMath::RoundToInt(
                100.f * DiscoveredGroundCells / TotalWalkableCells), 0, 100) : 0;
        const int32 RegionPercent = TotalByRegion[ActiveRegionIndex] > 0
            ? FMath::Clamp(FMath::RoundToInt(
                100.f * RevealedByRegion[ActiveRegionIndex] /
                TotalByRegion[ActiveRegionIndex]), 0, 100) : 0;
        if (MapWidth >= 520.f)
        {
            DrawText(FString::Printf(TEXT("DISCOVERED: %d TILES"), DiscoveredGroundCells),
                FLinearColor(.83f, .85f, .91f), MapOrigin.X + MapWidth - 190.f,
                MapOrigin.Y + 6.f, nullptr, .75f);
            DrawText(FString::Printf(TEXT("WORLD %d%%   REGION %d%%"),
                WorldPercent, RegionPercent), FLinearColor(.95f, .84f, .55f),
                MapOrigin.X + MapWidth - 207.f, MapOrigin.Y + 27.f, nullptr, .75f);
        }
        DrawText(TEXT("TAB: CLOSE MAP"), FLinearColor(.9f, .86f, .96f),
            MapOrigin.X + 10.f, MapOrigin.Y + MapHeight - 19.f, nullptr, .75f);
    }
}

