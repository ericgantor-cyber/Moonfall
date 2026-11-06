using System;

// Source-based progression port from godot-v38/scripts/native_v11.gd::_award_experience.
// Pure RPG state. Call once per defeated enemy; not tied to a new game mode.
public sealed class MoonfallProgression
{
    public int Level = 1;
    public int Experience;
    public int HeroPoints;
    public int TamerPoints;
    public int Combo;
    public float ComboExpiresAt;

    public int ExperienceToNext => 75 + (Level - 1) * 40;

    public int AwardDefeat(int zone, bool specialVariant, bool boss, float worldTime)
    {
        zone = Math.Max(0, Math.Min(5, zone));
        if (Level < 1 || Level > 100) Level = Math.Max(1, Math.Min(100, Level));
        int earned = 15 + zone * 8 + (specialVariant ? 12 : 0) + (boss ? 100 : 0);
        Combo = worldTime <= ComboExpiresAt ? Math.Min(Combo + 1, 12) : 1;
        ComboExpiresAt = worldTime + 7f;
        earned += Math.Min(10, Combo - 1) * 2;
        Experience += earned;
        int gained = 0;
        while (Level < 100 && Experience >= ExperienceToNext)
        {
            Experience -= ExperienceToNext;
            Level++;
            gained++;
            HeroPoints++;
            TamerPoints++;
        }
        if (Level >= 100) Experience = Math.Min(Experience, ExperienceToNext);
        return gained;
    }
}
