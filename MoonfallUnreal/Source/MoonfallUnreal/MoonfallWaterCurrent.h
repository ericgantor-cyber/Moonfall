#pragma once
#include "CoreMinimal.h"

namespace MoonfallWaterCurrent
{
    // Relative to the authored rectangular pool center and upper surface.
    // No vertical force, no currents near the dock or inside the grottoes.
    inline FVector Input(const FVector& Relative, bool bAscending, bool bAirPocket)
    {
        if (Relative.ContainsNaN() || bAscending || bAirPocket
            || Relative.Z >= -60.0 || Relative.Z <= -370.0
            || FMath::Abs(Relative.Y) >= 340.0) return FVector::ZeroVector;
        const double West = FMath::Max(0.0,1.0-FMath::Abs(Relative.X+700.0)/400.0);
        const double East = FMath::Max(0.0,1.0-FMath::Abs(Relative.X-700.0)/400.0);
        const double Edge = 1.0-FMath::Abs(Relative.Y)/340.0;
        return FVector(0.0,0.18*(West-East)*Edge,0.0);
    }
}
