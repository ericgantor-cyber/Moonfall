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

    if (Hero->GetHudHealth() <= 0.f)
    {
        // Respawn screen is fully rendered in the existing canvas: moonlit
        // spirit-sigil artwork, an accessible click target and no added modes.
        FVector2D ButtonMin, ButtonMax;
        Hero->GetRespawnButtonBounds(Canvas->ClipX, Canvas->ClipY, ButtonMin, ButtonMax);
        const FVector2D ButtonSize = ButtonMax - ButtonMin;
        const float CenterX = Canvas->ClipX * 0.5f;
        const float ArtRadius = FMath::Clamp(Canvas->ClipY * .105f, 24.f, 70.f);
        const float ArtY = FMath::Max(ArtRadius + 14.f, ButtonMin.Y - ArtRadius - 38.f);
        const FVector2D SigilCenter(CenterX, ArtY);
        // Bounded 40-segment ritual moon: visible even without imported PNGs.
        constexpr int32 RuneSegments = 40;
        for (int32 I = 0; I < RuneSegments; ++I)
        {
            const float A = I * 2.f * PI / RuneSegments - HALF_PI;
            const float B = (I + 1) * 2.f * PI / RuneSegments - HALF_PI;
            const FVector2D RingA = SigilCenter +
                FVector2D(FMath::Cos(A), FMath::Sin(A)) * ArtRadius;
            const FVector2D RingB = SigilCenter +
                FVector2D(FMath::Cos(B), FMath::Sin(B)) * ArtRadius;
            Canvas->K2_DrawLine(RingA, RingB, 2.f, FLinearColor(.7f, .38f, 1.f));
            // Eight inner rune spokes and an inset magic-circle ring.
            if (I % 5 == 0)
            {
                const FVector2D Inner = SigilCenter +
                    FVector2D(FMath::Cos(A), FMath::Sin(A)) * ArtRadius * .42f;
                Canvas->K2_DrawLine(Inner, RingA, 2.f, FLinearColor(.3f, .8f, 1.f));
            }
        }
        // The art does not use or modify the original Z/Nyra character images.
        DrawText(TEXT("DEFEATED"), FLinearColor(1.f, .45f, .52f),
            CenterX - 55.f, ArtY - ArtRadius - 43.f);
        float MouseX = -1.f, MouseY = -1.f;
        const bool bHasMouse = PlayerOwner->GetMousePosition(MouseX, MouseY);
        const bool bHovered = bHasMouse &&
            MouseX >= ButtonMin.X && MouseX <= ButtonMax.X &&
            MouseY >= ButtonMin.Y && MouseY <= ButtonMax.Y;
        // One high-contrast outline that shares the pawn's full click hitbox.
        Canvas->K2_DrawBox(ButtonMin, ButtonSize, bHovered ? 5.f : 3.f,
            bHovered ? FLinearColor(.25f, 1.f, .82f) : FLinearColor(.95f, .75f, .45f));
        const float TextX = CenterX - 39.f;
        const float TextY = (ButtonMin.Y + ButtonMax.Y) * 0.5f - 11.f;
        DrawText(TEXT("RESPAWN"), FLinearColor::White, TextX, TextY);
        DrawText(TEXT("Click or press gamepad A to respawn at your checkpoint"),
            FLinearColor(.88f, .86f, 1.f),
            FMath::Max(10.f, CenterX - 145.f), ButtonMax.Y + 12.f);
        // Don't cover the respawn menu with quests, map, or attack overlays.
        return;
    }

    if (Hero->IsWorldMapOpen())
    {
        // Shared projection with pawn input: zoom and pan never desync clicks.
        float Cell = 0.f;
        FVector2D Origin, ClipMin, ClipMax;
        Hero->GetWorldMapLayout(Canvas->ClipX, Canvas->ClipY,
            Cell, Origin, ClipMin, ClipMax);
        Canvas->K2_DrawBox(ClipMin - FVector2D(4.f, 4.f),
            ClipMax - ClipMin + FVector2D(8.f, 8.f), 2.f,
            FLinearColor(.55f, .55f, .62f));
        for (int32 Y = 0; Y < 16; ++Y)
            for (int32 X = 0; X < 48; ++X)
            {
                const FVector World(-1600.f + (X + .5f) * 400.f,
                    -3200.f + (Y + .5f) * 400.f, 0.f);
                if (!Hero->IsMapPositionDiscovered(World)) continue;
                const FVector2D Tile = Origin + FVector2D(X * Cell, Y * Cell);
                const FVector2D TileEnd = Tile + FVector2D(Cell, Cell);
                const FVector2D VisibleMin(FMath::Max(Tile.X, ClipMin.X),
                    FMath::Max(Tile.Y, ClipMin.Y));
                const FVector2D VisibleMax(FMath::Min(TileEnd.X, ClipMax.X),
                    FMath::Min(TileEnd.Y, ClipMax.Y));
                if (VisibleMax.X <= VisibleMin.X || VisibleMax.Y <= VisibleMin.Y) continue;
                Canvas->K2_DrawBox(VisibleMin, VisibleMax - VisibleMin, 1.f,
                    FLinearColor(.28f, .49f, .43f));
            }
        const auto Plot = [&](const FVector& World, const FLinearColor& Color)
        {
            if (!Hero->IsMapPositionDiscovered(World)) return;
            const FVector2D Screen = Origin + FVector2D(
                (World.X + 1600.f) / 400.f * Cell,
                (World.Y + 3200.f) / 400.f * Cell);
            if (Screen.X >= ClipMin.X + 5.f && Screen.X < ClipMax.X - 5.f &&
                Screen.Y >= ClipMin.Y + 5.f && Screen.Y < ClipMax.Y - 5.f)
                DrawMarker(Screen.X, Screen.Y, Color);
        };
        for (const FVector& Chamber : Hero->GetChamberMapPositions())
            Plot(Chamber, FLinearColor(.2f, .85f, .35f));
        for (const FVector& Enemy : Hero->GetEnemyMapPositions())
            Plot(Enemy, FLinearColor::Yellow);
        if (!Hero->IsBossDefeated())
            Plot(Hero->GetBossMapPosition(), FLinearColor::Red);
        FVector Pin;
        if (Hero->GetWaypoint(Pin)) Plot(Pin, FLinearColor(.3f, .8f, 1.f));
        FVector Quest;
        if (Hero->GetQuestDestination(Quest)) Plot(Quest, FLinearColor(1.f, .72f, .18f));
        Plot(Hero->GetHeroMapPosition(), FLinearColor(.1f, .9f, 1.f));
        // Right-stick map center is also the gamepad waypoint target.
        const FVector2D Reticle = (ClipMin + ClipMax) * 0.5f;
        Canvas->K2_DrawLine(Reticle + FVector2D(-10.f, 0.f),
            Reticle + FVector2D(10.f, 0.f), 2.f, FLinearColor(.3f, 1.f, .8f));
        Canvas->K2_DrawLine(Reticle + FVector2D(0.f, -10.f),
            Reticle + FVector2D(0.f, 10.f), 2.f, FLinearColor(.3f, 1.f, .8f));
        DrawText(FString::Printf(TEXT("MOONFALL WORLD MAP (combat paused)  |  M / Esc close  |  Wheel zoom %.1fx  |  Arrows pan"), Hero->GetWorldMapZoom()),
            FLinearColor::White, ClipMin.X, ClipMin.Y - 55.f);
        DrawText(TEXT("Mouse: click explored tile | Pad: right stick pan, A set center pin, B close, D-pad up/down zoom"),
            FLinearColor(.85f, .85f, .85f), ClipMin.X, ClipMax.Y + 18.f);
        DrawText(TEXT("Green chamber  Yellow beast  Red boss  Gold quest  Blue pin"),
            FLinearColor(.85f, .85f, .85f), ClipMin.X, ClipMax.Y + 40.f);
        return;
    }

    // Prototype arena radar, not the production six-region Raven Atlas.
    const float Left = Canvas->ClipX - 178.f;
    Canvas->K2_DrawBox(FVector2D(Left, 18.f), FVector2D(160.f, 160.f), 2.f,
        FLinearColor(.2f, .3f, .4f, .8f));
    const FVector H = Hero->GetHeroMapPosition();
    const FVector B = Hero->GetBossMapPosition();
    // Persistent 400 cm grid fog; unknown cells stay blank.
    // Full Raven Atlas, terrain art and waypoints remain separate migration work.
    DrawText(FString::Printf(TEXT("DISCOVERED %d/6"), Hero->GetDiscoveredRegionCount()),
        FLinearColor(.75f, .8f, .9f), Left, 182.f);
    // Reveal only sampled explored ground on the local radar, with no fog texture.
    for (int32 Y = -4; Y <= 4; ++Y)
        for (int32 X = -4; X <= 4; ++X)
        {
            const FVector CellCenter = H + FVector(X * 400.f, Y * 400.f, 0.f);
            if (Hero->IsMapPositionDiscovered(CellCenter))
                Canvas->K2_DrawBox(FVector2D(Left + 80.f + X * 16.f - 7.f,
                    98.f - Y * 16.f - 7.f), FVector2D(14.f, 14.f), 1.f,
                    FLinearColor(.22f, .42f, .37f, .75f));
        }
    // Local radar follows the hero across six continuous world regions.
    for (const FVector& Chamber : Hero->GetChamberMapPositions())
    {
        if (!Hero->IsMapPositionDiscovered(Chamber)) continue;
        const FVector Offset = Chamber - H;
        if (FMath::Abs(Offset.X) <= 1700.f && FMath::Abs(Offset.Y) <= 1700.f)
            DrawMarker(Left + 80.f + Offset.X * .04f, 98.f - Offset.Y * .04f,
                FLinearColor(.20f, .85f, .35f));
    }
    for (const FVector& Enemy : Hero->GetEnemyMapPositions())
    {
        if (!Hero->IsMapPositionDiscovered(Enemy)) continue;
        const FVector Offset = Enemy - H;
        if (FMath::Abs(Offset.X) <= 1700.f && FMath::Abs(Offset.Y) <= 1700.f)
            DrawMarker(Left + 80.f + Offset.X * .04f, 98.f - Offset.Y * .04f, FLinearColor::Yellow);
    }
    const FVector BossOffset = B - H;
    if (!Hero->IsBossDefeated() && Hero->IsMapPositionDiscovered(B) &&
        FMath::Abs(BossOffset.X) <= 1700.f && FMath::Abs(BossOffset.Y) <= 1700.f)
        DrawMarker(Left + 80.f + BossOffset.X * .04f,
            98.f - BossOffset.Y * .04f, FLinearColor::Red);
    FVector MarkerPosition;
    if (Hero->GetWaypoint(MarkerPosition))
    {
        const FVector Offset = MarkerPosition - H;
        if (FMath::Abs(Offset.X) <= 1700.f && FMath::Abs(Offset.Y) <= 1700.f &&
            Hero->IsMapPositionDiscovered(MarkerPosition))
            DrawMarker(Left + 80.f + Offset.X * .04f, 98.f - Offset.Y * .04f,
                FLinearColor(.4f, .8f, 1.f));
        DrawText(FString::Printf(TEXT("PIN: %.0f m  (V set / B clear)"),
            FVector::Dist2D(H, MarkerPosition) / 100.f),
            FLinearColor(.4f, .8f, 1.f), Left - 110.f, 308.f);
    }
    FVector Destination;
    if (Hero->GetQuestDestination(Destination))
    {
        const FVector Offset = Destination - H;
        if (FMath::Abs(Offset.X) <= 1700.f && FMath::Abs(Offset.Y) <= 1700.f)
            DrawMarker(Left + 80.f + Offset.X * .04f, 98.f - Offset.Y * .04f,
                FLinearColor(1.f, .72f, .18f));
        DrawText(FString::Printf(TEXT("QUEST: %.0f m"), FVector::Dist2D(H, Destination) / 100.f),
            FLinearColor(1.f, .72f, .18f), Left - 110.f, 331.f);
    }
    DrawMarker(Left + 80.f, 98.f, FLinearColor(.2f, .9f, 1.f));

    // Quest tracker ports a persistent Godot-style objective list without
    // duplicating save flags. The radar and original orb artwork stay intact.
    const float QuestX = FMath::Max(10.f, Canvas->ClipX - 280.f);
    const float QuestY = 192.f;
    const int32 Beasts = Hero->GetQuestDefeatedBeasts();
    const int32 Chambers = Hero->GetQuestClearedChambers();
    DrawText(TEXT("WORLD QUESTS"), FLinearColor(.95f, .82f, .52f), QuestX, QuestY);
    DrawText(FString::Printf(TEXT("%s Hunt beasts  %d/%d"),
        Beasts >= 18 ? TEXT("[DONE]") : TEXT("[ ]"), Beasts, 18),
        FLinearColor::White, QuestX, QuestY + 25.f);
    DrawText(FString::Printf(TEXT("%s Clear chambers  %d/%d"),
        Chambers >= 30 ? TEXT("[DONE]") : TEXT("[ ]"), Chambers, 30),
        FLinearColor::White, QuestX, QuestY + 48.f);
    DrawText(Hero->IsBossDefeated() ? TEXT("[DONE] Defeat Z") : TEXT("[ ] Defeat Z"),
        FLinearColor::White, QuestX, QuestY + 71.f);
}
