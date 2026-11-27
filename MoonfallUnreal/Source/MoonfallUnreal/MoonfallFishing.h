#pragma once
#include <cmath>
namespace MoonfallFishing
{
    constexpr int Rank(int Catches) { return Catches<0?1:Catches>=27?10:1+Catches/3; }
    constexpr double BiteSeconds(int Catches) { return 10.0+(Rank(Catches)-1)*0.5; }
    constexpr double CastWait(double Roll,int Catches) {
        const double Bounded=Roll>=0.0&&Roll<=1.0?Roll:0.5;
        return 2.0+Bounded*(5.0-(Rank(Catches)-1)*0.3);
    }
    constexpr int SchoolCount=8;
    constexpr int SchoolSpecies(int Id) { return Id>=0&&Id<SchoolCount?Id%3:-1; }
    struct FSchoolPosition { double X=0,Y=0; };
    inline FSchoolPosition SchoolPosition(int Id,double Now,double Width,double Length) {
        if(SchoolSpecies(Id)<0||!std::isfinite(Now)||Now<0||!std::isfinite(Width)||!std::isfinite(Length)||Width<600||Length<600) return {};
        const double Phase=std::fmod(Now,120.0)*0.0523598775598+Id*0.785398163397;
        const double Along=(Id%2?0.75:0.25)+0.08*std::sin(Phase);
        const double Inset=200+60*std::cos(Phase);
        switch(Id/2) {
        case 0:return {Inset,Length*Along};
        case 1:return {Width-Inset,Length*Along};
        case 2:return {Width*Along,Inset};
        default:return {Width*Along,Length-Inset};
        }
    }
    constexpr double SchoolWait(double Wait) { return Wait>3.5?3.5:Wait; }
    enum class EStage { Idle, Waiting, Bite, Reeling };
    struct FSession {
        EStage Stage=EStage::Idle;
        double Deadline=0.0;
        double HookWindow=10.0;
        double StageStarted=0.0;
        void Cancel() { Stage=EStage::Idle; Deadline=0.0; HookWindow=10.0; StageStarted=0.0; }
        bool Start(double Now,double Wait,int Catches=0) {
            if(Stage!=EStage::Idle||!std::isfinite(Now)||Now<0.0||!std::isfinite(Wait)||Wait<2.0||Wait>7.0) return false;
            if(!std::isfinite(Now+Wait)||Now+Wait<=Now) return false;
            HookWindow=BiteSeconds(Catches); Stage=EStage::Waiting; StageStarted=Now; Deadline=Now+Wait; return true;
        }
        bool Hook(double Now) {
            if(Stage!=EStage::Bite||!std::isfinite(Now)||Now<StageStarted||Now>=Deadline||Now+1.5<=Now||!std::isfinite(Now+1.5)) return false;
            Stage=EStage::Reeling; StageStarted=Now; Deadline=Now+1.5; return true;
        }
        bool Advance(double Now,bool Eligible) {
            if(!Eligible||!std::isfinite(Now)||Now<0.0) { Cancel(); return false; }
            if(Stage==EStage::Idle) return false;
            if(Now<StageStarted) { Cancel(); return false; }
            if(Now<Deadline) return false;
            if(Stage==EStage::Waiting) {
                if(!std::isfinite(Now+HookWindow)||Now+HookWindow<=Now) { Cancel(); return false; }
                Stage=EStage::Bite; StageStarted=Now; Deadline=Now+HookWindow; return false;
            }
            const bool Caught=Stage==EStage::Reeling;
            Cancel();
            return Caught;
        }
    };
}
