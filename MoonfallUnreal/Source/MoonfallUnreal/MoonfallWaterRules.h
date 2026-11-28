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
    constexpr float TonicDuration = 120.f;
    constexpr float TonicThreshold = 10.f;
    // Local broad-phase refresh: the 25 cm probe covers the 10 cm movement
    // budget for stationary pockets. Moving pockets refresh at most 0.1 s later.
    inline bool RefreshPocketCandidates(double Now,double Last,double DistanceSquared)
    {
        return !FMath::IsFinite(Now) || !FMath::IsFinite(Last) || Last<0.0 || Now<Last
            || Now-Last>=0.1 || !FMath::IsFinite(DistanceSquared) || DistanceSquared>=100.0;
    }
    // Vertical commands are discrete and never survive invalid depth or input locks.
    inline int32 ResolveVerticalIntent(int32 Intent,bool bCanSteer,float EyeDepth)
    {
        if (!bCanSteer || !FMath::IsFinite(EyeDepth) || Intent < -1 || Intent > 1) return 0;
        return Intent>0 && EyeDepth<=0.f ? 0 : Intent;
    }
    // Presentation-only pitch; no effect on collision, movement or combat.
    inline float SwimPosePitch(bool bSwimming,bool bSubmerged,bool bReducedMotion,float VerticalSpeed)
    {
        if(!bSwimming||!bSubmerged||bReducedMotion||!FMath::IsFinite(VerticalSpeed)) return 0.f;
        return FMath::Clamp(-10.f+FMath::Clamp(VerticalSpeed/250.f,-1.f,1.f)*20.f,-25.f,10.f);
    }
    inline bool ShouldUseTonic(float Air, float Remaining, bool bNeedsBreath, bool bPaused)
    {
        return bNeedsBreath && !bPaused && FMath::IsFinite(Air)
            && Air >= 0.f && Air <= TonicThreshold
            && FMath::IsFinite(Remaining) && Remaining <= 0.f;
    }
    inline float AdvanceTonic(float Remaining, float Seconds, bool bPaused)
    {
        if (!FMath::IsFinite(Remaining)) return 0.f;
        Remaining = FMath::Clamp(Remaining, 0.f, TonicDuration);
        return bPaused || !FMath::IsFinite(Seconds) || Seconds <= 0.f
            ? Remaining : FMath::Max(0.f, Remaining - Seconds);
    }
    inline float BreathMultiplier(bool bRaceChosen, int32 RaceId)
    {
        return bRaceChosen && RaceId == 9 ? 2.f : 1.f;
    }
    // Existing Focus accessories and footwear confer bounded traversal bonuses.
    // Common items cannot carry Focus in the canonical equipment format.
    inline float FocusGearBonus(int32 Rarity)
    {
        return Rarity >= 1 && Rarity <= 5 ? 0.05f * (Rarity + 1) : 0.f;
    }
    inline float GearBreathMultiplier(bool bRaceChosen, int32 RaceId, int32 FocusRarity)
    {
        return FMath::Clamp(BreathMultiplier(bRaceChosen, RaceId) * (1.f + FocusGearBonus(FocusRarity)), 1.f, 2.6f);
    }
    inline float GearSwimMultiplier(int32 FocusFeetRarity)
    {
        return 1.f + FocusGearBonus(FocusFeetRarity);
    }
    inline float SetBreathMultiplier(float Base, int32 Pieces)
    {
        return FMath::Clamp(Base * (Pieces >= 2 ? 1.10f : 1.f), 1.f, 2.86f);
    }
    inline float SetSwimMultiplier(float Base, int32 Pieces)
    {
        return FMath::Clamp(Base + (Pieces >= 3 ? 0.10f : 0.f), 1.f, 1.4f);
    }
    inline float EffectiveDrain(float Multiplier)
    {
        return DrainPerSecond / (FMath::IsFinite(Multiplier) ? FMath::Clamp(Multiplier, 1.f, 2.86f) : 1.f);
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
    // Split a frame at tonic expiration: protected time refills air, only
    // the remaining unprotected time can drain air or cause drowning.
    inline bool AdvanceBreathFrame(float& Air,float& Exposure,float Seconds,
                                   bool bSwimming,bool bNeedsBreath,bool bPaused,
                                   float ProtectedSeconds,float Multiplier)
    {
        if (!bSwimming) { Air=MaxAir; Exposure=0.f; return false; }
        if (bPaused || !FMath::IsFinite(Seconds) || Seconds<=0.f) return false;
        Seconds=FMath::Min(Seconds,0.25f);
        const float Protected=bNeedsBreath && FMath::IsFinite(ProtectedSeconds)
            ? FMath::Clamp(ProtectedSeconds,0.f,Seconds) : 0.f;
        if (!bNeedsBreath || Protected>0.f) {
            Air=AdvanceAir(Air,bNeedsBreath?Protected:Seconds,true,false,false,Multiplier);
            Exposure=0.f;
        }
        if (!bNeedsBreath || Protected>=Seconds) return false;
        const float Remaining=Seconds-Protected, Before=Air;
        Air=AdvanceAir(Air,Remaining,true,true,false,Multiplier);
        return AdvanceDrowning(Exposure,Before,Remaining,true,false,Multiplier);
    }
}
