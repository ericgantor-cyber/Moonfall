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
