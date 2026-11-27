#pragma once
#include "CoreMinimal.h"

namespace MoonfallAquatic
{
    struct FProfile {
        const TCHAR* Name;
        const TCHAR* Attack;
        float Health, Damage, Speed, Windup, Recovery, Reach;
        FVector Scale;
    };
    inline FProfile Profile(int32 Id)
    {
        switch (Id) {
        case 1: return {TEXT("Pearlback"), TEXT("SHELL RAM"), 85.f, 10.f, 105.f, 1.1f, 2.4f, 165.f, FVector(0.95f,0.75f,0.55f)};
        case 2: return {TEXT("Lantern Eel"), TEXT("SHOCK BITE"), 55.f, 9.f, 185.f, 0.75f, 1.9f, 180.f, FVector(1.65f,0.32f,0.32f)};
        case 3: return {TEXT("Kelp Stalker"), TEXT("LUNGE"), 70.f, 11.f, 170.f, 0.85f, 2.2f, 195.f, FVector(1.25f,0.4f,0.6f)};
        case 4: return {TEXT("Glassfin Rare"), TEXT("RAZOR FIN"), 115.f, 14.f, 180.f, 0.9f, 2.2f, 200.f, FVector(1.45f,0.45f,0.65f)};
        case 5: return {TEXT("Brineheart Leviathan"), TEXT("TIDAL CRUSH"), 240.f, 18.f, 110.f, 1.4f, 3.f, 220.f, FVector(1.7f,0.8f,0.7f)};
        default: return {TEXT("Saltveil Hunter"), TEXT("BITE"), 60.f, 8.f, 140.f, 0.8f, 2.f, 160.f, FVector(1.2f,0.48f,0.48f)};
        }
    }
    inline int32 Phase(int32 Id, float HealthFraction)
    {
        if (Id != 5 || !FMath::IsFinite(HealthFraction)) return 1;
        return HealthFraction <= 0.3f ? 3 : HealthFraction <= 0.65f ? 2 : 1;
    }
    inline float RecoveryScale(int32 PhaseId) { return PhaseId == 3 ? 0.72f : PhaseId == 2 ? 0.85f : 1.f; }
}
