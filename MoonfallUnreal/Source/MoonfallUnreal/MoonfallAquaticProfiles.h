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
    // One roll per authored slot per world. No extra spawns or repeated rolls.
    // The Leviathan remains guaranteed, Glassfin appears only in its own grotto.
    inline int32 SelectEncounter(int32 Slot, int32 Roll)
    {
        if (Slot < 0 || Slot > 5 || Roll < 0 || Roll > 99) return 0;
        if (Slot == 5) return 5;
        if (Slot == 4) return Roll < 25 ? 4 : (Roll - 25) % 4;
        if (Roll < 70) return Slot;
        return (Slot + 1 + (Roll - 70) % 3) % 4;
    }
    inline int32 Phase(int32 Id, float HealthFraction)
    {
        if (Id != 5 || !FMath::IsFinite(HealthFraction)) return 1;
        return HealthFraction <= 0.3f ? 3 : HealthFraction <= 0.65f ? 2 : 1;
    }
    struct FMove {
        const TCHAR* Name;
        float Reach, Damage, Windup, HalfAngleCos;
        bool bRadial, bUnblockable;
    };
    inline int32 SelectMove(int32 Id, int32 PhaseId, int32 Sequence)
    {
        if (Sequence < 0) return 0;
        if (Id == 4) return Sequence % 2 == 1 ? 1 : 0;
        if (Id == 5 && PhaseId >= 2) {
            const int32 Cycle = Sequence % (PhaseId >= 3 ? 3 : 2);
            return Cycle;
        }
        return 0;
    }
    inline FMove Move(int32 Id, int32 MoveId)
    {
        const auto P = Profile(Id);
        if (Id == 4 && MoveId == 1)
            return {TEXT("GLASSFIN SWEEP"), 245.f, 17.f, 1.2f, 0.f, true, false};
        if (Id == 5 && MoveId == 1)
            return {TEXT("UNDERTOW PULSE"), 295.f, 22.f, 1.6f, 0.f, true, true};
        if (Id == 5 && MoveId == 2)
            return {TEXT("BRINE LUNGE"), 280.f, 23.f, 1.25f, 0.85f, false, false};
        return {P.Attack, P.Reach + 20.f, P.Damage, P.Windup, 0.35f, false, false};
    }
    inline bool Hits(const FMove& M, const FVector& Offset, const FVector& LockedDirection)
    {
        if (Offset.ContainsNaN() || LockedDirection.ContainsNaN()) return false;
        if (Offset.SizeSquared() > FMath::Square(M.Reach)) return false;
        if (M.bRadial || Offset.IsNearlyZero()) return true;
        return !LockedDirection.IsNearlyZero()
            && FVector::DotProduct(Offset.GetSafeNormal(), LockedDirection.GetSafeNormal()) >= M.HalfAngleCos;
    }
    inline float RecoveryScale(int32 PhaseId) { return PhaseId == 3 ? 0.72f : PhaseId == 2 ? 0.85f : 1.f; }
}
