#pragma once
#include <cmath>
namespace MoonfallFishing
{
    enum class EStage { Idle, Waiting, Bite, Reeling };
    struct FSession {
        EStage Stage=EStage::Idle;
        double Deadline=0.0;
        void Cancel() { Stage=EStage::Idle; Deadline=0.0; }
        bool Start(double Now,double Wait) {
            if(Stage!=EStage::Idle||!std::isfinite(Now)||Now<0.0||!std::isfinite(Wait)||Wait<2.0||Wait>7.0) return false;
            Stage=EStage::Waiting; Deadline=Now+Wait; return true;
        }
        bool Hook(double Now) {
            if(Stage!=EStage::Bite||!std::isfinite(Now)||Now<0.0||Now>=Deadline) return false;
            Stage=EStage::Reeling; Deadline=Now+1.5; return true;
        }
        bool Advance(double Now,bool Eligible) {
            if(!Eligible||!std::isfinite(Now)||Now<0.0) { Cancel(); return false; }
            if(Stage==EStage::Idle||Now<Deadline) return false;
            if(Stage==EStage::Waiting) { Stage=EStage::Bite; Deadline=Now+5.0; return false; }
            const bool Caught=Stage==EStage::Reeling;
            Cancel();
            return Caught;
        }
    };
}
