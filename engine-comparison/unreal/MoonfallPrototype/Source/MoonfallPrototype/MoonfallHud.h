#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MoonfallHud.generated.h"

UCLASS()
class MOONFALLPROTOTYPE_API AMoonfallHud : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    void DrawRing(FVector2D Center, float Radius, float Ratio, const FLinearColor& Color);
    void DrawMarker(float X, float Y, const FLinearColor& Color);
};
