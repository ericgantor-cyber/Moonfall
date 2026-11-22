#pragma once
// Exact gameplay data ported from godot-v38/data/moonfall_talents.json.
// 141 original Moonfall talents. No art, save files, or world assets generated.
#include "CoreMinimal.h"

namespace MoonfallTalents
{
enum class EStat : uint8 { Health, Mana, Stamina, MeleeDamage, RangedDamage, MagicDamage };
struct FNode
{
 const TCHAR* Id;
 const TCHAR* Title;
 const TCHAR* Branch;
 int32 Tier;
 int32 MinLevel;
 int32 MaxRank;
 int32 CostPerRank;
 const TCHAR* RequiresA;
 const TCHAR* RequiresB;
 EStat Stat;
 bool bPercent;
 float AmountPerRank;
};

inline const TArray<FNode>& Catalog()
{
 static const TArray<FNode> Nodes = {
  {TEXT("iron_will"), TEXT("Iron Will"), TEXT("might"), 1, 1, 5, 1, TEXT(""), TEXT(""), EStat::MeleeDamage, true, 0.03f},
  {TEXT("breaker"), TEXT("Breaker"), TEXT("might"), 2, 4, 3, 1, TEXT("iron_will"), TEXT(""), EStat::MeleeDamage, true, 0.04f},
  {TEXT("guard"), TEXT("Guard"), TEXT("might"), 2, 4, 3, 1, TEXT("iron_will"), TEXT(""), EStat::Health, false, 8.0f},
  {TEXT("titan_strike"), TEXT("Titan Strike"), TEXT("might"), 3, 10, 1, 1, TEXT("breaker"), TEXT("guard"), EStat::MeleeDamage, true, 0.08f},
  {TEXT("swift_step"), TEXT("Swift Step"), TEXT("agility"), 1, 1, 5, 1, TEXT(""), TEXT(""), EStat::Stamina, false, 5.0f},
  {TEXT("hawk_eye"), TEXT("Hawk Eye"), TEXT("agility"), 2, 4, 3, 1, TEXT("swift_step"), TEXT(""), EStat::RangedDamage, true, 0.04f},
  {TEXT("wind_runner"), TEXT("Wind Runner"), TEXT("agility"), 2, 4, 3, 1, TEXT("swift_step"), TEXT(""), EStat::Stamina, false, 8.0f},
  {TEXT("eagle_strike"), TEXT("Eagle Strike"), TEXT("agility"), 3, 10, 1, 1, TEXT("hawk_eye"), TEXT("wind_runner"), EStat::RangedDamage, true, 0.08f},
  {TEXT("spirit_well"), TEXT("Spirit Well"), TEXT("arcana"), 1, 1, 5, 1, TEXT(""), TEXT(""), EStat::Mana, false, 8.0f},
  {TEXT("spell_focus"), TEXT("Spell Focus"), TEXT("arcana"), 2, 4, 3, 1, TEXT("spirit_well"), TEXT(""), EStat::MagicDamage, true, 0.04f},
  {TEXT("soul_ward"), TEXT("Soul Ward"), TEXT("arcana"), 2, 4, 3, 1, TEXT("spirit_well"), TEXT(""), EStat::Health, false, 8.0f},
  {TEXT("starfall"), TEXT("Starfall"), TEXT("arcana"), 3, 10, 1, 1, TEXT("spell_focus"), TEXT("soul_ward"), EStat::MagicDamage, true, 0.08f},
  {TEXT("might_ashen_grip"), TEXT("Ashen Grip"), TEXT("might"), 4, 16, 2, 1, TEXT("titan_strike"), TEXT(""), EStat::MeleeDamage, true, 0.015f},
  {TEXT("might_crimson_guard"), TEXT("Crimson Guard"), TEXT("might"), 4, 16, 2, 1, TEXT("titan_strike"), TEXT(""), EStat::Health, false, 7.0f},
  {TEXT("might_steel_momentum"), TEXT("Steel Momentum"), TEXT("might"), 4, 16, 2, 1, TEXT("titan_strike"), TEXT(""), EStat::MeleeDamage, true, 0.012f},
  {TEXT("might_battle_rhythm"), TEXT("Battle Rhythm"), TEXT("might"), 4, 16, 2, 1, TEXT("titan_strike"), TEXT(""), EStat::Health, false, 6.0f},
  {TEXT("might_rift_breaker"), TEXT("Rift Breaker"), TEXT("might"), 5, 24, 2, 1, TEXT("might_ashen_grip"), TEXT(""), EStat::MeleeDamage, true, 0.0165f},
  {TEXT("might_stoneheart"), TEXT("Stoneheart"), TEXT("might"), 5, 24, 2, 1, TEXT("might_crimson_guard"), TEXT(""), EStat::Health, false, 9.0f},
  {TEXT("might_heavy_hand"), TEXT("Heavy Hand"), TEXT("might"), 5, 24, 2, 1, TEXT("might_steel_momentum"), TEXT(""), EStat::MeleeDamage, true, 0.0135f},
  {TEXT("might_resolute_march"), TEXT("Resolute March"), TEXT("might"), 5, 24, 2, 1, TEXT("might_battle_rhythm"), TEXT(""), EStat::Health, false, 8.0f},
  {TEXT("might_siege_instinct"), TEXT("Siege Instinct"), TEXT("might"), 6, 32, 2, 1, TEXT("might_rift_breaker"), TEXT(""), EStat::MeleeDamage, true, 0.018f},
  {TEXT("might_blood_oath"), TEXT("Blood Oath"), TEXT("might"), 6, 32, 2, 1, TEXT("might_stoneheart"), TEXT(""), EStat::Health, false, 11.0f},
  {TEXT("might_hammerfall"), TEXT("Hammerfall"), TEXT("might"), 6, 32, 2, 1, TEXT("might_heavy_hand"), TEXT(""), EStat::MeleeDamage, true, 0.015f},
  {TEXT("might_bulwark"), TEXT("Bulwark"), TEXT("might"), 6, 32, 2, 1, TEXT("might_resolute_march"), TEXT("might_rift_breaker"), EStat::Health, false, 10.0f},
  {TEXT("might_oath_of_iron"), TEXT("Oath of Iron"), TEXT("might"), 7, 42, 2, 1, TEXT("might_siege_instinct"), TEXT(""), EStat::MeleeDamage, true, 0.0195f},
  {TEXT("might_ironhide"), TEXT("Ironhide"), TEXT("might"), 7, 42, 2, 1, TEXT("might_blood_oath"), TEXT(""), EStat::Health, false, 13.0f},
  {TEXT("might_executioner"), TEXT("Executioner"), TEXT("might"), 7, 42, 2, 1, TEXT("might_hammerfall"), TEXT(""), EStat::MeleeDamage, true, 0.0165f},
  {TEXT("might_unshaken"), TEXT("Unshaken"), TEXT("might"), 7, 42, 2, 1, TEXT("might_bulwark"), TEXT(""), EStat::Health, false, 12.0f},
  {TEXT("might_warborn"), TEXT("Warborn"), TEXT("might"), 8, 52, 2, 1, TEXT("might_oath_of_iron"), TEXT(""), EStat::MeleeDamage, true, 0.021f},
  {TEXT("might_cragwall"), TEXT("Cragwall"), TEXT("might"), 8, 52, 2, 1, TEXT("might_ironhide"), TEXT(""), EStat::Health, false, 15.0f},
  {TEXT("might_tyrant_s_edge"), TEXT("Tyrant's Edge"), TEXT("might"), 8, 52, 2, 1, TEXT("might_executioner"), TEXT(""), EStat::MeleeDamage, true, 0.018f},
  {TEXT("might_heart_of_the_mountain"), TEXT("Heart of the Mountain"), TEXT("might"), 8, 52, 2, 1, TEXT("might_unshaken"), TEXT(""), EStat::Health, false, 14.0f},
  {TEXT("might_moonforged_blade"), TEXT("Moonforged Blade"), TEXT("might"), 9, 62, 2, 1, TEXT("might_warborn"), TEXT(""), EStat::MeleeDamage, true, 0.0225f},
  {TEXT("might_aegis_pulse"), TEXT("Aegis Pulse"), TEXT("might"), 9, 62, 2, 1, TEXT("might_cragwall"), TEXT(""), EStat::Health, false, 17.0f},
  {TEXT("might_breaker_of_chains"), TEXT("Breaker of Chains"), TEXT("might"), 9, 62, 2, 1, TEXT("might_tyrant_s_edge"), TEXT(""), EStat::MeleeDamage, true, 0.0195f},
  {TEXT("might_last_stand"), TEXT("Last Stand"), TEXT("might"), 9, 62, 2, 1, TEXT("might_heart_of_the_mountain"), TEXT("might_warborn"), EStat::Health, false, 16.0f},
  {TEXT("might_worldsplitter"), TEXT("Worldsplitter"), TEXT("might"), 10, 72, 2, 1, TEXT("might_moonforged_blade"), TEXT(""), EStat::MeleeDamage, true, 0.024f},
  {TEXT("might_fortress_soul"), TEXT("Fortress Soul"), TEXT("might"), 10, 72, 2, 1, TEXT("might_aegis_pulse"), TEXT(""), EStat::Health, false, 19.0f},
  {TEXT("might_sundering_fury"), TEXT("Sundering Fury"), TEXT("might"), 10, 72, 2, 1, TEXT("might_breaker_of_chains"), TEXT(""), EStat::MeleeDamage, true, 0.021f},
  {TEXT("might_immortal_march"), TEXT("Immortal March"), TEXT("might"), 10, 72, 2, 1, TEXT("might_last_stand"), TEXT(""), EStat::Health, false, 18.0f},
  {TEXT("might_titan_s_legacy"), TEXT("Titan's Legacy"), TEXT("might"), 11, 82, 1, 1, TEXT("might_worldsplitter"), TEXT(""), EStat::MeleeDamage, true, 0.0255f},
  {TEXT("might_eclipse_bastion"), TEXT("Eclipse Bastion"), TEXT("might"), 11, 82, 1, 1, TEXT("might_fortress_soul"), TEXT(""), EStat::Health, false, 21.0f},
  {TEXT("might_judgment_of_steel"), TEXT("Judgment of Steel"), TEXT("might"), 11, 82, 1, 1, TEXT("might_sundering_fury"), TEXT(""), EStat::MeleeDamage, true, 0.0225f},
  {TEXT("might_might_ascendant"), TEXT("Might Ascendant"), TEXT("might"), 11, 82, 1, 1, TEXT("might_immortal_march"), TEXT(""), EStat::Health, false, 20.0f},
  {TEXT("agility_gale_footwork"), TEXT("Gale Footwork"), TEXT("agility"), 4, 16, 2, 1, TEXT("eagle_strike"), TEXT(""), EStat::Stamina, false, 5.0f},
  {TEXT("agility_silent_bow"), TEXT("Silent Bow"), TEXT("agility"), 4, 16, 2, 1, TEXT("eagle_strike"), TEXT(""), EStat::RangedDamage, true, 0.015f},
  {TEXT("agility_springstep"), TEXT("Springstep"), TEXT("agility"), 4, 16, 2, 1, TEXT("eagle_strike"), TEXT(""), EStat::Stamina, false, 6.0f},
  {TEXT("agility_featherlight"), TEXT("Featherlight"), TEXT("agility"), 4, 16, 2, 1, TEXT("eagle_strike"), TEXT(""), EStat::RangedDamage, true, 0.012f},
  {TEXT("agility_shadowstride"), TEXT("Shadowstride"), TEXT("agility"), 5, 24, 2, 1, TEXT("agility_gale_footwork"), TEXT(""), EStat::Stamina, false, 7.0f},
  {TEXT("agility_piercing_sight"), TEXT("Piercing Sight"), TEXT("agility"), 5, 24, 2, 1, TEXT("agility_silent_bow"), TEXT(""), EStat::RangedDamage, true, 0.0165f},
  {TEXT("agility_quick_reflex"), TEXT("Quick Reflex"), TEXT("agility"), 5, 24, 2, 1, TEXT("agility_springstep"), TEXT(""), EStat::Stamina, false, 8.0f},
  {TEXT("agility_wind_dance"), TEXT("Wind Dance"), TEXT("agility"), 5, 24, 2, 1, TEXT("agility_featherlight"), TEXT(""), EStat::RangedDamage, true, 0.0135f},
  {TEXT("agility_phantom_pace"), TEXT("Phantom Pace"), TEXT("agility"), 6, 32, 2, 1, TEXT("agility_shadowstride"), TEXT(""), EStat::Stamina, false, 9.0f},
  {TEXT("agility_deadeye_focus"), TEXT("Deadeye Focus"), TEXT("agility"), 6, 32, 2, 1, TEXT("agility_piercing_sight"), TEXT(""), EStat::RangedDamage, true, 0.018f},
  {TEXT("agility_aerial_balance"), TEXT("Aerial Balance"), TEXT("agility"), 6, 32, 2, 1, TEXT("agility_quick_reflex"), TEXT(""), EStat::Stamina, false, 10.0f},
  {TEXT("agility_trailblazer"), TEXT("Trailblazer"), TEXT("agility"), 6, 32, 2, 1, TEXT("agility_wind_dance"), TEXT("agility_shadowstride"), EStat::RangedDamage, true, 0.015f},
  {TEXT("agility_mistwalker"), TEXT("Mistwalker"), TEXT("agility"), 7, 42, 2, 1, TEXT("agility_phantom_pace"), TEXT(""), EStat::Stamina, false, 11.0f},
  {TEXT("agility_marked_shot"), TEXT("Marked Shot"), TEXT("agility"), 7, 42, 2, 1, TEXT("agility_deadeye_focus"), TEXT(""), EStat::RangedDamage, true, 0.0195f},
  {TEXT("agility_fleet_recovery"), TEXT("Fleet Recovery"), TEXT("agility"), 7, 42, 2, 1, TEXT("agility_aerial_balance"), TEXT(""), EStat::Stamina, false, 12.0f},
  {TEXT("agility_storm_runner"), TEXT("Storm Runner"), TEXT("agility"), 7, 42, 2, 1, TEXT("agility_trailblazer"), TEXT(""), EStat::RangedDamage, true, 0.0165f},
  {TEXT("agility_eclipse_step"), TEXT("Eclipse Step"), TEXT("agility"), 8, 52, 2, 1, TEXT("agility_mistwalker"), TEXT(""), EStat::Stamina, false, 13.0f},
  {TEXT("agility_longshot"), TEXT("Longshot"), TEXT("agility"), 8, 52, 2, 1, TEXT("agility_marked_shot"), TEXT(""), EStat::RangedDamage, true, 0.021f},
  {TEXT("agility_dancer_s_guard"), TEXT("Dancer's Guard"), TEXT("agility"), 8, 52, 2, 1, TEXT("agility_fleet_recovery"), TEXT(""), EStat::Stamina, false, 14.0f},
  {TEXT("agility_untethered"), TEXT("Untethered"), TEXT("agility"), 8, 52, 2, 1, TEXT("agility_storm_runner"), TEXT(""), EStat::RangedDamage, true, 0.018f},
  {TEXT("agility_dusk_flight"), TEXT("Dusk Flight"), TEXT("agility"), 9, 62, 2, 1, TEXT("agility_eclipse_step"), TEXT(""), EStat::Stamina, false, 15.0f},
  {TEXT("agility_falcon_s_reach"), TEXT("Falcon's Reach"), TEXT("agility"), 9, 62, 2, 1, TEXT("agility_longshot"), TEXT(""), EStat::RangedDamage, true, 0.0225f},
  {TEXT("agility_flow_state"), TEXT("Flow State"), TEXT("agility"), 9, 62, 2, 1, TEXT("agility_dancer_s_guard"), TEXT(""), EStat::Stamina, false, 16.0f},
  {TEXT("agility_horizon_walker"), TEXT("Horizon Walker"), TEXT("agility"), 9, 62, 2, 1, TEXT("agility_untethered"), TEXT("agility_eclipse_step"), EStat::RangedDamage, true, 0.0195f},
  {TEXT("agility_ghostwind"), TEXT("Ghostwind"), TEXT("agility"), 10, 72, 2, 1, TEXT("agility_dusk_flight"), TEXT(""), EStat::Stamina, false, 17.0f},
  {TEXT("agility_starlit_arrow"), TEXT("Starlit Arrow"), TEXT("agility"), 10, 72, 2, 1, TEXT("agility_falcon_s_reach"), TEXT(""), EStat::RangedDamage, true, 0.024f},
  {TEXT("agility_endless_motion"), TEXT("Endless Motion"), TEXT("agility"), 10, 72, 2, 1, TEXT("agility_flow_state"), TEXT(""), EStat::Stamina, false, 18.0f},
  {TEXT("agility_tempest_path"), TEXT("Tempest Path"), TEXT("agility"), 10, 72, 2, 1, TEXT("agility_horizon_walker"), TEXT(""), EStat::RangedDamage, true, 0.021f},
  {TEXT("agility_moonshadow"), TEXT("Moonshadow"), TEXT("agility"), 11, 82, 1, 1, TEXT("agility_ghostwind"), TEXT(""), EStat::Stamina, false, 19.0f},
  {TEXT("agility_skyhunter"), TEXT("Skyhunter"), TEXT("agility"), 11, 82, 1, 1, TEXT("agility_starlit_arrow"), TEXT(""), EStat::RangedDamage, true, 0.0255f},
  {TEXT("agility_astral_stride"), TEXT("Astral Stride"), TEXT("agility"), 11, 82, 1, 1, TEXT("agility_endless_motion"), TEXT(""), EStat::Stamina, false, 20.0f},
  {TEXT("agility_agility_ascendant"), TEXT("Agility Ascendant"), TEXT("agility"), 11, 82, 1, 1, TEXT("agility_tempest_path"), TEXT(""), EStat::RangedDamage, true, 0.0225f},
  {TEXT("arcana_ember_sigil"), TEXT("Ember Sigil"), TEXT("arcana"), 4, 16, 2, 1, TEXT("starfall"), TEXT(""), EStat::MagicDamage, true, 0.015f},
  {TEXT("arcana_moonwell"), TEXT("Moonwell"), TEXT("arcana"), 4, 16, 2, 1, TEXT("starfall"), TEXT(""), EStat::Mana, false, 6.0f},
  {TEXT("arcana_woven_ward"), TEXT("Woven Ward"), TEXT("arcana"), 4, 16, 2, 1, TEXT("starfall"), TEXT(""), EStat::Health, false, 6.0f},
  {TEXT("arcana_star_current"), TEXT("Star Current"), TEXT("arcana"), 4, 16, 2, 1, TEXT("starfall"), TEXT(""), EStat::MagicDamage, true, 0.012f},
  {TEXT("arcana_spellweave"), TEXT("Spellweave"), TEXT("arcana"), 5, 24, 2, 1, TEXT("arcana_ember_sigil"), TEXT(""), EStat::MagicDamage, true, 0.0165f},
  {TEXT("arcana_azure_reservoir"), TEXT("Azure Reservoir"), TEXT("arcana"), 5, 24, 2, 1, TEXT("arcana_moonwell"), TEXT(""), EStat::Mana, false, 8.0f},
  {TEXT("arcana_spirit_mantle"), TEXT("Spirit Mantle"), TEXT("arcana"), 5, 24, 2, 1, TEXT("arcana_woven_ward"), TEXT(""), EStat::Health, false, 8.0f},
  {TEXT("arcana_ethereal_flow"), TEXT("Ethereal Flow"), TEXT("arcana"), 5, 24, 2, 1, TEXT("arcana_star_current"), TEXT(""), EStat::MagicDamage, true, 0.0135f},
  {TEXT("arcana_comet_lore"), TEXT("Comet Lore"), TEXT("arcana"), 6, 32, 2, 1, TEXT("arcana_spellweave"), TEXT(""), EStat::MagicDamage, true, 0.018f},
  {TEXT("arcana_deep_focus"), TEXT("Deep Focus"), TEXT("arcana"), 6, 32, 2, 1, TEXT("arcana_azure_reservoir"), TEXT(""), EStat::Mana, false, 10.0f},
  {TEXT("arcana_astral_shield"), TEXT("Astral Shield"), TEXT("arcana"), 6, 32, 2, 1, TEXT("arcana_spirit_mantle"), TEXT(""), EStat::Health, false, 10.0f},
  {TEXT("arcana_runic_echo"), TEXT("Runic Echo"), TEXT("arcana"), 6, 32, 2, 1, TEXT("arcana_ethereal_flow"), TEXT("arcana_spellweave"), EStat::MagicDamage, true, 0.015f},
  {TEXT("arcana_void_spark"), TEXT("Void Spark"), TEXT("arcana"), 7, 42, 2, 1, TEXT("arcana_comet_lore"), TEXT(""), EStat::MagicDamage, true, 0.0195f},
  {TEXT("arcana_soul_channel"), TEXT("Soul Channel"), TEXT("arcana"), 7, 42, 2, 1, TEXT("arcana_deep_focus"), TEXT(""), EStat::Mana, false, 12.0f},
  {TEXT("arcana_celestial_shelter"), TEXT("Celestial Shelter"), TEXT("arcana"), 7, 42, 2, 1, TEXT("arcana_astral_shield"), TEXT(""), EStat::Health, false, 12.0f},
  {TEXT("arcana_arc_pulse"), TEXT("Arc Pulse"), TEXT("arcana"), 7, 42, 2, 1, TEXT("arcana_runic_echo"), TEXT(""), EStat::MagicDamage, true, 0.0165f},
  {TEXT("arcana_twilight_flame"), TEXT("Twilight Flame"), TEXT("arcana"), 8, 52, 2, 1, TEXT("arcana_void_spark"), TEXT(""), EStat::MagicDamage, true, 0.021f},
  {TEXT("arcana_arcane_spring"), TEXT("Arcane Spring"), TEXT("arcana"), 8, 52, 2, 1, TEXT("arcana_soul_channel"), TEXT(""), EStat::Mana, false, 14.0f},
  {TEXT("arcana_ward_of_ages"), TEXT("Ward of Ages"), TEXT("arcana"), 8, 52, 2, 1, TEXT("arcana_celestial_shelter"), TEXT(""), EStat::Health, false, 14.0f},
  {TEXT("arcana_mana_cascade"), TEXT("Mana Cascade"), TEXT("arcana"), 8, 52, 2, 1, TEXT("arcana_arc_pulse"), TEXT(""), EStat::MagicDamage, true, 0.018f},
  {TEXT("arcana_eclipse_invocation"), TEXT("Eclipse Invocation"), TEXT("arcana"), 9, 62, 2, 1, TEXT("arcana_twilight_flame"), TEXT(""), EStat::MagicDamage, true, 0.0225f},
  {TEXT("arcana_starlight_font"), TEXT("Starlight Font"), TEXT("arcana"), 9, 62, 2, 1, TEXT("arcana_arcane_spring"), TEXT(""), EStat::Mana, false, 16.0f},
  {TEXT("arcana_spectral_bastion"), TEXT("Spectral Bastion"), TEXT("arcana"), 9, 62, 2, 1, TEXT("arcana_ward_of_ages"), TEXT(""), EStat::Health, false, 16.0f},
  {TEXT("arcana_spellstorm"), TEXT("Spellstorm"), TEXT("arcana"), 9, 62, 2, 1, TEXT("arcana_mana_cascade"), TEXT("arcana_twilight_flame"), EStat::MagicDamage, true, 0.0195f},
  {TEXT("arcana_aether_nova"), TEXT("Aether Nova"), TEXT("arcana"), 10, 72, 2, 1, TEXT("arcana_eclipse_invocation"), TEXT(""), EStat::MagicDamage, true, 0.024f},
  {TEXT("arcana_infinite_wellspring"), TEXT("Infinite Wellspring"), TEXT("arcana"), 10, 72, 2, 1, TEXT("arcana_starlight_font"), TEXT(""), EStat::Mana, false, 18.0f},
  {TEXT("arcana_runebound_heart"), TEXT("Runebound Heart"), TEXT("arcana"), 10, 72, 2, 1, TEXT("arcana_spectral_bastion"), TEXT(""), EStat::Health, false, 18.0f},
  {TEXT("arcana_lunar_convergence"), TEXT("Lunar Convergence"), TEXT("arcana"), 10, 72, 2, 1, TEXT("arcana_spellstorm"), TEXT(""), EStat::MagicDamage, true, 0.021f},
  {TEXT("arcana_moonfall_arcanum"), TEXT("Moonfall Arcanum"), TEXT("arcana"), 11, 82, 1, 1, TEXT("arcana_aether_nova"), TEXT(""), EStat::MagicDamage, true, 0.0255f},
  {TEXT("arcana_starborn_reservoir"), TEXT("Starborn Reservoir"), TEXT("arcana"), 11, 82, 1, 1, TEXT("arcana_infinite_wellspring"), TEXT(""), EStat::Mana, false, 20.0f},
  {TEXT("arcana_eternal_ward"), TEXT("Eternal Ward"), TEXT("arcana"), 11, 82, 1, 1, TEXT("arcana_runebound_heart"), TEXT(""), EStat::Health, false, 20.0f},
  {TEXT("arcana_arcana_ascendant"), TEXT("Arcana Ascendant"), TEXT("arcana"), 11, 82, 1, 1, TEXT("arcana_lunar_convergence"), TEXT(""), EStat::MagicDamage, true, 0.0225f},
  {TEXT("might_cinder_vanguard"), TEXT("Cinder Vanguard"), TEXT("might"), 12, 86, 1, 1, TEXT("might_titan_s_legacy"), TEXT(""), EStat::MeleeDamage, true, 0.026f},
  {TEXT("might_anvil_heart"), TEXT("Anvil Heart"), TEXT("might"), 12, 86, 1, 1, TEXT("might_eclipse_bastion"), TEXT(""), EStat::Health, false, 22.0f},
  {TEXT("might_ruin_cleaver"), TEXT("Ruin Cleaver"), TEXT("might"), 12, 86, 1, 1, TEXT("might_judgment_of_steel"), TEXT(""), EStat::MeleeDamage, true, 0.023f},
  {TEXT("might_dreadnought"), TEXT("Dreadnought"), TEXT("might"), 12, 86, 1, 1, TEXT("might_might_ascendant"), TEXT(""), EStat::Health, false, 18.0f},
  {TEXT("might_sovereign_hammer"), TEXT("Sovereign Hammer"), TEXT("might"), 13, 91, 1, 1, TEXT("might_cinder_vanguard"), TEXT(""), EStat::MeleeDamage, true, 0.028f},
  {TEXT("might_colossus_ward"), TEXT("Colossus Ward"), TEXT("might"), 13, 91, 1, 1, TEXT("might_anvil_heart"), TEXT(""), EStat::Health, false, 26.0f},
  {TEXT("might_shattercrest"), TEXT("Shattercrest"), TEXT("might"), 13, 91, 1, 1, TEXT("might_ruin_cleaver"), TEXT(""), EStat::MeleeDamage, true, 0.027f},
  {TEXT("might_iron_dominion"), TEXT("Iron Dominion"), TEXT("might"), 13, 91, 1, 1, TEXT("might_dreadnought"), TEXT(""), EStat::Health, false, 24.0f},
  {TEXT("might_eclipse_champion"), TEXT("Eclipse Champion"), TEXT("might"), 14, 96, 1, 1, TEXT("might_sovereign_hammer"), TEXT(""), EStat::MeleeDamage, true, 0.032f},
  {TEXT("might_living_citadel"), TEXT("Living Citadel"), TEXT("might"), 14, 96, 1, 1, TEXT("might_colossus_ward"), TEXT(""), EStat::Health, false, 30.0f},
  {TEXT("might_final_reckoning"), TEXT("Final Reckoning"), TEXT("might"), 14, 96, 1, 1, TEXT("might_shattercrest"), TEXT(""), EStat::MeleeDamage, true, 0.035f},
  {TEXT("agility_duskwind_step"), TEXT("Duskwind Step"), TEXT("agility"), 12, 86, 1, 1, TEXT("agility_moonshadow"), TEXT(""), EStat::Stamina, false, 22.0f},
  {TEXT("agility_falcon_claw"), TEXT("Falcon Claw"), TEXT("agility"), 12, 86, 1, 1, TEXT("agility_skyhunter"), TEXT(""), EStat::RangedDamage, true, 0.025f},
  {TEXT("agility_windborne"), TEXT("Windborne"), TEXT("agility"), 12, 86, 1, 1, TEXT("agility_astral_stride"), TEXT(""), EStat::Stamina, false, 21.0f},
  {TEXT("agility_silent_horizon"), TEXT("Silent Horizon"), TEXT("agility"), 12, 86, 1, 1, TEXT("agility_agility_ascendant"), TEXT(""), EStat::RangedDamage, true, 0.023f},
  {TEXT("agility_mirage_runner"), TEXT("Mirage Runner"), TEXT("agility"), 13, 91, 1, 1, TEXT("agility_duskwind_step"), TEXT(""), EStat::Stamina, false, 26.0f},
  {TEXT("agility_celestial_hunt"), TEXT("Celestial Hunt"), TEXT("agility"), 13, 91, 1, 1, TEXT("agility_falcon_claw"), TEXT(""), EStat::RangedDamage, true, 0.028f},
  {TEXT("agility_tempest_flight"), TEXT("Tempest Flight"), TEXT("agility"), 13, 91, 1, 1, TEXT("agility_windborne"), TEXT(""), EStat::Stamina, false, 25.0f},
  {TEXT("agility_specter_arrow"), TEXT("Specter Arrow"), TEXT("agility"), 13, 91, 1, 1, TEXT("agility_silent_horizon"), TEXT(""), EStat::RangedDamage, true, 0.027f},
  {TEXT("agility_ghost_of_dawn"), TEXT("Ghost of Dawn"), TEXT("agility"), 14, 96, 1, 1, TEXT("agility_mirage_runner"), TEXT(""), EStat::Stamina, false, 30.0f},
  {TEXT("agility_skybreaker"), TEXT("Skybreaker"), TEXT("agility"), 14, 96, 1, 1, TEXT("agility_celestial_hunt"), TEXT(""), EStat::RangedDamage, true, 0.034f},
  {TEXT("agility_horizon_sovereign"), TEXT("Horizon Sovereign"), TEXT("agility"), 14, 96, 1, 1, TEXT("agility_tempest_flight"), TEXT(""), EStat::Stamina, false, 0.031f},
  {TEXT("arcana_astral_ember"), TEXT("Astral Ember"), TEXT("arcana"), 12, 86, 1, 1, TEXT("arcana_moonfall_arcanum"), TEXT(""), EStat::MagicDamage, true, 0.027f},
  {TEXT("arcana_lunar_reservoir"), TEXT("Lunar Reservoir"), TEXT("arcana"), 12, 86, 1, 1, TEXT("arcana_starborn_reservoir"), TEXT(""), EStat::Mana, false, 22.0f},
  {TEXT("arcana_warden_of_stars"), TEXT("Warden of Stars"), TEXT("arcana"), 12, 86, 1, 1, TEXT("arcana_eternal_ward"), TEXT(""), EStat::Health, false, 22.0f},
  {TEXT("arcana_veil_of_eternity"), TEXT("Veil of Eternity"), TEXT("arcana"), 12, 86, 1, 1, TEXT("arcana_arcana_ascendant"), TEXT(""), EStat::MagicDamage, true, 0.024f},
  {TEXT("arcana_nebula_spear"), TEXT("Nebula Spear"), TEXT("arcana"), 13, 91, 1, 1, TEXT("arcana_astral_ember"), TEXT(""), EStat::MagicDamage, true, 0.029f},
  {TEXT("arcana_endless_font"), TEXT("Endless Font"), TEXT("arcana"), 13, 91, 1, 1, TEXT("arcana_lunar_reservoir"), TEXT(""), EStat::Mana, false, 27.0f},
  {TEXT("arcana_aegis_of_twilight"), TEXT("Aegis of Twilight"), TEXT("arcana"), 13, 91, 1, 1, TEXT("arcana_warden_of_stars"), TEXT(""), EStat::Health, false, 27.0f},
  {TEXT("arcana_rift_conduit"), TEXT("Rift Conduit"), TEXT("arcana"), 13, 91, 1, 1, TEXT("arcana_veil_of_eternity"), TEXT(""), EStat::MagicDamage, true, 0.028f},
  {TEXT("arcana_astral_coronation"), TEXT("Astral Coronation"), TEXT("arcana"), 14, 96, 1, 1, TEXT("arcana_nebula_spear"), TEXT(""), EStat::MagicDamage, true, 0.034f},
  {TEXT("arcana_fountain_of_moons"), TEXT("Fountain of Moons"), TEXT("arcana"), 14, 96, 1, 1, TEXT("arcana_endless_font"), TEXT(""), EStat::Mana, false, 32.0f},
  {TEXT("arcana_final_starward"), TEXT("Final Starward"), TEXT("arcana"), 14, 96, 1, 1, TEXT("arcana_aegis_of_twilight"), TEXT(""), EStat::Health, false, 32.0f}
 };
 return Nodes;
}

inline const FNode* Find(FName Id)
{
 if (Id.IsNone()) return nullptr;
 for (const FNode& Node : Catalog())
  if (FName(Node.Id) == Id) return &Node;
 return nullptr;
}

inline int32 EarnedPoints(int32 Level)
{
 return Level >= 1 && Level <= 100 ? (Level - 1) * 2 : -1;
}

inline bool Validate(int32 Level, const TMap<FName, int32>& Ranks)
{
 const int32 Earned = EarnedPoints(Level);
 if (Earned < 0 || Ranks.Num() > Catalog().Num()) return false;
 int32 Spent = 0;
 for (const auto& Entry : Ranks) {
  const FNode* Node = Find(Entry.Key);
  if (!Node || Entry.Value <= 0 || Entry.Value > Node->MaxRank
      || Level < Node->MinLevel || Node->CostPerRank <= 0) return false;
  // All node costs and ranks have bounded small values; the sum is safe.
  Spent += Entry.Value * Node->CostPerRank;
  if (Spent > Earned) return false;
 }
 for (const auto& Entry : Ranks) {
  const FNode* Node = Find(Entry.Key);
  if (!Node) return false;
  if (Node->RequiresA[0] && Ranks.FindRef(FName(Node->RequiresA)) < 1) return false;
  if (Node->RequiresB[0] && Ranks.FindRef(FName(Node->RequiresB)) < 1) return false;
 }
 return true;
}

inline int32 AvailablePoints(int32 Level, const TMap<FName, int32>& Ranks)
{
 if (!Validate(Level, Ranks)) return 0;
 int32 Spent = 0;
 for (const auto& Entry : Ranks) {
  const FNode* Node = Find(Entry.Key);
  if (Node) Spent += Entry.Value * Node->CostPerRank;
 }
 return EarnedPoints(Level) - Spent;
}

inline bool TryAllocate(int32 Level, TMap<FName, int32>& Ranks, FName TalentId)
{
 if (!Validate(Level, Ranks)) return false;
 const FNode* Node = Find(TalentId);
 if (!Node || Ranks.FindRef(TalentId) >= Node->MaxRank
     || AvailablePoints(Level, Ranks) < Node->CostPerRank) return false;
 TMap<FName, int32> Candidate = Ranks;
 Candidate.Add(TalentId, Ranks.FindRef(TalentId) + 1);
 if (!Validate(Level, Candidate)) return false;
 Ranks = MoveTemp(Candidate);
 return true;
}

// Caller validates at load/commit. A failed validation must never grant bonuses.
inline float Bonus(const TMap<FName, int32>& Ranks, EStat Stat, bool bPercent)
{
 float Total = 0.f;
 for (const FNode& Node : Catalog()) {
  if (Node.Stat != Stat || Node.bPercent != bPercent) continue;
  const int32 Rank = Ranks.FindRef(FName(Node.Id));
  if (Rank > 0 && Rank <= Node.MaxRank)
   Total += static_cast<float>(Rank) * Node.AmountPerRank;
 }
 return Total;
}
}
