#pragma once
#include "CoreMinimal.h"

namespace MoonfallWaterRules
{
    constexpr float MaxAir = 100.f;
    constexpr float BreathDurationSeconds = 180.f;
    constexpr float DrainPerSecond = MaxAir / BreathDurationSeconds;
    constexpr float RefillPerSecond = 25.f;
    constexpr float SurfaceSpeedScale = 4.722222f / 7.f;
    constexpr float DrowningHealthFraction = 0.20f;
    inline float BreathMultiplier(bool bRaceChosen, int32 RaceId)
    {
        return bRaceChosen && RaceId == 9 ? 2.f : 1.f;
    }
    inline float EffectiveDrain(float Multiplier)
    {
        return DrainPerSecond / (FMath::IsFinite(Multiplier) ? FMath::Clamp(Multiplier, 1.f, 2.f) : 1.f);
    }
    // Accumulate only the portion of a submerged frame spent without air.
    inline bool AdvanceDrowning(float& Exposure, float PreviousAir, float Seconds,
                                bool bSubmerged, bool bPaused, float Multiplier = 1.f)
    {
        if (!bSubmerged) { Exposure = 0.f; return false; }
        if (!FMath::IsFinite(Exposure) || Exposure < 0.f || Exposure >= 1.f) Exposure = 0.f;
        if (bPaused || !FMath::IsFinite(Seconds) || Seconds <= 0.f
            || !FMath::IsFinite(PreviousAir)) return false;
        const float EmptySeconds = FMath::Max(0.f, FMath::Min(Seconds, 0.25f)
            - FMath::Clamp(PreviousAir, 0.f, MaxAir) / EffectiveDrain(Multiplier));
        Exposure += EmptySeconds;
        if (Exposure < 1.f) return false;
        Exposure -= 1.f;
        return true;
    }


    // Transient resource only; callers supply collision-confirmed submersion.
    inline float AdvanceAir(float Air, float Seconds, bool bInWater, bool bSubmerged, bool bPaused, float Multiplier = 1.f)
    {
        Air = FMath::IsFinite(Air) ? FMath::Clamp(Air, 0.f, MaxAir) : MaxAir;
        if (!bInWater) return MaxAir;
        if (bPaused || !FMath::IsFinite(Seconds) || Seconds <= 0.f) return Air;
        const float Rate = bSubmerged ? -EffectiveDrain(Multiplier) : RefillPerSecond;
        return FMath::Clamp(Air + Rate * Seconds, 0.f, MaxAir);
    }
}
