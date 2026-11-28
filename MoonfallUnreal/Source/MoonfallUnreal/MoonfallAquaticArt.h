#pragma once
#include "CoreMinimal.h"

namespace MoonfallAquaticArt
{
    // Local transforms on the existing body; no new colliders or gameplay data.
    inline TArray<FTransform> Fins(int32 Id)
    {
        const float Tail = Id == 2 ? 0.35f : Id == 5 ? 0.28f : 0.21f;
        const float Fin = Id == 1 ? 0.14f : Id == 4 ? 0.34f : 0.23f;
        TArray<FTransform> Parts;
        Parts.Reserve(8);
        // Forked tail.
        for (float Side : {-1.f, 1.f}) {
            Parts.Emplace(FRotator(0.f,Side*30.f,0.f),
                FVector(-47.f,Side*13.f,0.f),FVector(Tail,0.12f,0.12f));
            // Paired pectoral fins.
            Parts.Emplace(FRotator(0.f,Side*35.f,Side*12.f),
                FVector(0.f,Side*38.f,-8.f),FVector(0.23f,Fin,0.07f));
        }
        if (Id == 1) {
            // Broad layered shell ridges.
            for (int32 I=0; I<3; ++I)
                Parts.Emplace(FRotator::ZeroRotator,FVector(22.f-I*20.f,0.f,38.f),FVector(0.13f,0.42f,0.12f));
        } else {
            const int32 Count = Id == 5 ? 4 : Id == 3 ? 3 : 1;
            for (int32 I=0; I<Count; ++I)
                Parts.Emplace(FRotator(-25.f,0.f,0.f),FVector(18.f-I*15.f,0.f,39.f),FVector(0.13f,0.08f,Id==4?0.32f:0.23f));
        }
        return Parts;
    }
    inline FLinearColor Tint(int32 Id)
    {
        switch (Id) {
        case 1: return FLinearColor(0.62f,0.44f,0.85f);
        case 2: return FLinearColor(0.95f,0.48f,0.12f);
        case 3: return FLinearColor(0.16f,0.65f,0.35f);
        case 4: return FLinearColor(0.12f,0.85f,0.92f);
        case 5: return FLinearColor(0.7f,0.12f,0.22f);
        default: return FLinearColor(0.25f,0.52f,0.72f);
        }
    }
}
