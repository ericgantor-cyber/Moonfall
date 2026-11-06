#pragma once

#include "CoreMinimal.h"

// Source-based progression port of Godot native_v11.gd::_award_experience.
// No gameplay actor, art, global singleton, or save coupling.
struct FMoonfallProgression
{
    int32 Level = 1;
    int32 Experience = 0;
    int32 HeroPoints = 0;
    int32 TamerPoints = 0;
    int32 Combo = 0;
    float ComboExpiresAt = 0.f;

    int32 ExperienceToNext() const { return 75 + (Level - 1) * 40; }

    int32 AwardDefeat(int32 Zone, bool bSpecialVariant, bool bBoss, float WorldTime)
    {
        Zone = FMath::Clamp(Zone, 0, 5);
        Level = FMath::Clamp(Level, 1, 100);
        const int32 Base = 15 + Zone * 8 + (bSpecialVariant ? 12 : 0) + (bBoss ? 100 : 0);
        Combo = WorldTime <= ComboExpiresAt ? FMath::Min(Combo + 1, 12) : 1;
        ComboExpiresAt = WorldTime + 7.f;
        Experience += Base + FMath::Min(10, Combo - 1) * 2;
        int32 Gained = 0;
        while (Level < 100 && Experience >= ExperienceToNext())
        {
            Experience -= ExperienceToNext();
            ++Level;
            ++Gained;
            ++HeroPoints;
            ++TamerPoints;
        }
        if (Level >= 100) Experience = FMath::Min(Experience, ExperienceToNext());
        return Gained;
    }
};
