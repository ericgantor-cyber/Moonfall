#pragma once
#include "CoreMinimal.h"

namespace MoonfallWaterRules
{
    constexpr float MaxAir = 100.f;
    constexpr float DrainPerSecond = 4.f;
    constexpr float RefillPerSecond = 25.f;
    constexpr float SurfaceSpeedScale = 0.62f;

    // Transient resource only; callers supply collision-confirmed submersion.
    inline float AdvanceAir(float Air, float Seconds, bool bInWater, bool bSubmerged, bool bPaused)
    {
        Air = FMath::IsFinite(Air) ? FMath::Clamp(Air, 0.f, MaxAir) : MaxAir;
        if (!bInWater) return MaxAir;
        if (bPaused || !FMath::IsFinite(Seconds) || Seconds <= 0.f) return Air;
        const float Rate = bSubmerged ? -DrainPerSecond : RefillPerSecond;
        return FMath::Clamp(Air + Rate * Seconds, 0.f, MaxAir);
    }
}
