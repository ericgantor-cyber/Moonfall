#pragma once
#include "CoreMinimal.h"

// Six contiguous regions, no waves, no scene mode switches.
// Unreal positions are in centimeters (Unity units x 100).
namespace MoonfallWorldRules
{
    constexpr int32 RegionCount = 6;
    constexpr int32 EnemiesPerRegion = 3;
    constexpr int32 TotalEnemies = RegionCount * EnemiesPerRegion;
    constexpr float RegionSpacing = 3200.f;
    constexpr float HalfRegion = 1700.f;
    constexpr float BossX = 16000.f;

    inline float RegionCenter(int32 Region)
    {
        return FMath::Clamp(Region, 0, RegionCount - 1) * RegionSpacing;
    }

    inline int32 RegionAt(float X)
    {
        return FMath::Clamp(FMath::FloorToInt((X + HalfRegion) / RegionSpacing), 0, RegionCount - 1);
    }

    inline FVector EnemyPosition(int32 Region, int32 Slot)
    {
        const float X = RegionCenter(Region) + (Slot - 1) * 800.f;
        const float Y = (Slot % 2 == 0 ? -1.f : 1.f) * (500.f + (Region % 3) * 100.f);
        return FVector(X, Y, 100.f);
    }

    inline float EnemyMaxHealth(int32 Region)
    {
        return 35.f + 14.f * FMath::Clamp(Region, 0, RegionCount - 1);
    }
}
