#pragma once
namespace MoonfallAttributes {
 constexpr int Clamp(int Value,int Low,int High) {
  return Value<Low?Low:Value>High?High:Value;
 }
 // Derived points only. No new persistent fields or random rolls.
 struct FPrimary {int Strength=0,Agility=0,Intellect=0,Stamina=0;};
 constexpr FPrimary BuildPrimary(int Level,int WeaponTier,int ArmorTier,int AccessoryTier) {
  const int Base=10+2*(Clamp(Level,1,100)-1);
  return {Base+2*Clamp(WeaponTier,0,25),
          Base+Clamp(WeaponTier,0,25)+Clamp(ArmorTier,0,25),
          Base+2*Clamp(AccessoryTier,0,25),
          Base+2*Clamp(ArmorTier,0,25)};
 }
 // Proposed balance coefficients, not applied to live combat yet.
 constexpr int PhysicalBonusBps(int Strength) {return Clamp(Strength,0,250)*8;}
 constexpr int FinesseBonusBps(int Agility) {return Clamp(Agility,0,250)*4;}
 constexpr int SpellBonusBps(int Intellect) {return Clamp(Intellect,0,250)*8;}
 constexpr int HealthBonus(int Stamina) {return Clamp(Stamina,0,250)*2;}
}
