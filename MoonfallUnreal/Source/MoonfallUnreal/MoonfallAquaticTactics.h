#pragma once
namespace MoonfallAquaticTactics
{
    struct FBehavior {
        float PatrolRadius, PatrolRate, RecoveryDistance, StrafeDistance, RecoverySpeedScale;
    };
    constexpr FBehavior Behavior(int Id) {
        switch (Id) {
        case 1: return {45.f,0.35f,0.f,0.f,0.f};       // Pearlback braces
        case 2: return {105.f,0.85f,45.f,85.f,0.65f}; // Eel circles
        case 3: return {75.f,0.50f,85.f,30.f,0.70f};  // Stalker retreats
        case 4: return {95.f,0.65f,60.f,70.f,0.65f};  // Rare flanks
        case 5: return {35.f,0.25f,0.f,0.f,0.f};       // Leviathan rests
        default:return {80.f,0.55f,35.f,40.f,0.55f};  // Hunter repositions
        }
    }
    constexpr bool MayPatrol(bool Swimming,bool InPocket,bool Engaged,float HomeDistance) {
        return Swimming&&!InPocket&&!Engaged&&HomeDistance>=0.f&&HomeDistance<=850.f;
    }
    constexpr bool MayReposition(bool SightClear,bool Recovering,bool WindingUp) {
        return SightClear&&Recovering&&!WindingUp;
    }
}
