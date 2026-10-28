extends RefCounted
## Small read-only tips database; no runtime file I/O or new save keys.
const PAGE_SIZE:int=8
const CATEGORIES:Array[String]=["All","Getting Started","Combat","Heroes & Upgrades","Bosses & Invasions","World & Exploration","Water & Diving","Professions & Crafting","Gear & Economy","Creatures & Companions","Death & Recovery","Controls & Accessibility","Performance & Saves"]
const ENTRIES:Array[Dictionary]=[
 {"category":"Getting Started","title":"Choose your hero","tip":"Z specializes in aggressive void attacks; Nyra uses distinct eclipse magic. Pick the hero whose abilities suit you."},
 {"category":"Getting Started","title":"Learn the controls","tip":"WASD or arrows move. Left click attacks. Space dodges. Q, E, and R cast spells. G interacts."},
 {"category":"Getting Started","title":"Controller basics","tip":"Left stick moves. Right stick moves the camera. X attacks, A dodges, B interacts, and Start opens the Atlas."},
 {"category":"Getting Started","title":"Open your Atlas","tip":"Esc or F10 opens the Raven Atlas. F1 opens this searchable Tips Codex directly during play."},
 {"category":"Getting Started","title":"Read the status messages","tip":"Short messages explain pickups, combat hazards, new discoveries, and nearby interactions."},
 {"category":"Getting Started","title":"Explore deliberately","tip":"The world is connected. Follow roads and activate shrines before moving into harder regions."},
 {"category":"Getting Started","title":"Save often","tip":"Use the Atlas System tab to save your checkpoint before a long expedition."},
 {"category":"Getting Started","title":"Use quick actions","tip":"The on-screen quick actions offer interact, dodge, pet stance and save without memorizing every key."},
 {"category":"Combat","title":"Watch enemy windups","tip":"Boss attack markers show the threatened area. Move or dodge before the warning resolves."},
 {"category":"Combat","title":"Use dodge timing","tip":"A well-timed dodge avoids heavy hits and may build Moon Resonance during certain encounters."},
 {"category":"Combat","title":"Attack between hazards","tip":"Focus on safe counterattack windows rather than staying next to a boss through every warning."},
 {"category":"Combat","title":"Choose spells deliberately","tip":"Try Q, E, and R to combine damage, movement and control. Their effects differ between the heroes."},
 {"category":"Combat","title":"Mind cooldowns","tip":"When your attack or spell is recovering, reposition instead of repeatedly pressing it."},
 {"category":"Combat","title":"Keep a healing reserve","tip":"Healing draughts and crafted food are more useful before a dangerous boss than after HP reaches zero."},
 {"category":"Combat","title":"Avoid being surrounded","tip":"Use terrain and movement to keep enemies in front of you when possible."},
 {"category":"Combat","title":"Watch danger in water","tip":"Underwater swimming limits mobility. Return to the surface rather than fighting at zero oxygen."},
 {"category":"Heroes & Upgrades","title":"Level up efficiently","tip":"Defeat appropriate enemies, earn XP and use the Hero Skills tab to spend earned points."},
 {"category":"Heroes & Upgrades","title":"Talent branches","tip":"Assault, Survival and Arcana each suit different combat priorities. Rank upgrades have limits."},
 {"category":"Heroes & Upgrades","title":"Z fighting style","tip":"Z rewards pressure and careful timing. Look for his special Dragon Transformation talent when available."},
 {"category":"Heroes & Upgrades","title":"Nyra fighting style","tip":"Nyra uses her own spell choices and effects; do not assume Z and Nyra share identical upgrades."},
 {"category":"Heroes & Upgrades","title":"Equipment power","tip":"Compare an item with your equipped gear before replacing it. Higher power can help in tougher regions."},
 {"category":"Heroes & Upgrades","title":"Regional power guidance","tip":"The Atlas shows recommended power for the current region. It is advice, not a hard level gate."},
 {"category":"Heroes & Upgrades","title":"Reroll choices","tip":"Where a level-up card screen offers reroll, use it when the options do not support your current build."},
 {"category":"Heroes & Upgrades","title":"Balance offense and survival","tip":"Extra damage is useful, but defenses, healing and mobility protect a long run."},
 {"category":"Bosses & Invasions","title":"Boss warning markers","tip":"Visible floor warnings show where a strike will land. Leave the danger area before impact."},
 {"category":"Bosses & Invasions","title":"Boss phases","tip":"Some commanders change their attack patterns as health decreases. Reassess after each phase shift."},
 {"category":"Bosses & Invasions","title":"Stormbreach commander","tip":"Watch for lightning markers and crossing attack lanes. Escape the marked sections quickly."},
 {"category":"Bosses & Invasions","title":"Emberfall commander","tip":"Fire zones can split. Seek unmarked floor space before the strike."},
 {"category":"Bosses & Invasions","title":"Voidtide commander","tip":"Void hazards cover broad areas; spacing and movement matter more than standing still."},
 {"category":"Bosses & Invasions","title":"Flawless rewards","tip":"Certain commander bonuses require avoiding hits. A resurrection may disqualify the flawless reward."},
 {"category":"Bosses & Invasions","title":"One active invasion","tip":"Moonfall runs its lunar invasion threats in the connected world rather than a separate game mode."},
 {"category":"Bosses & Invasions","title":"Commander loot","tip":"Invasion commanders reward existing crafting resources and progress toward stronger equipment."},
 {"category":"World & Exploration","title":"Six connected regions","tip":"Use the World tab to review zones, shrines, current events and local threats."},
 {"category":"World & Exploration","title":"Shrine discovery","tip":"Find and activate regional shrines to expand your travel options."},
 {"category":"World & Exploration","title":"Fast travel","tip":"Awakened shrines allow travel when you are out of combat."},
 {"category":"World & Exploration","title":"World day and weather","tip":"The World tab reports day phase and weather, which change the scene and can affect travel."},
 {"category":"World & Exploration","title":"Lunar corruption","tip":"Unsealed rifts distort regional plants and water. Purification improves the scenery."},
 {"category":"World & Exploration","title":"Cleansing rifts","tip":"Interact at a rift marker and defeat its escalating enemy waves to seal it."},
 {"category":"World & Exploration","title":"World events","tip":"Approach an event marker and use G to begin its local objective."},
 {"category":"World & Exploration","title":"Ward beacons","tip":"A restored region can provide a temporary defensive benefit. New invasions may threaten it."},
 {"category":"World & Exploration","title":"Check the minimap","tip":"Tab toggles the circular minimap, helping you spot locations and nearby movement."},
 {"category":"World & Exploration","title":"Explore at your pace","tip":"There is no general survival stopwatch and you do not need to build structures."},
 {"category":"Water & Diving","title":"Enter a lake","tip":"Move into one of the six lake areas to start surface swimming."},
 {"category":"Water & Diving","title":"Dive underwater","tip":"Hold C or controller right-stick click while in water to descend."},
 {"category":"Water & Diving","title":"Ascend safely","tip":"Press Space or controller A to rise and regain access to the surface."},
 {"category":"Water & Diving","title":"Oxygen bar","tip":"Air drains while submerged, even after you release the dive key. Surface to refill."},
 {"category":"Water & Diving","title":"Low oxygen warning","tip":"At 25 percent oxygen the HUD warns you. Turn back before it reaches zero."},
 {"category":"Water & Diving","title":"Zero oxygen","tip":"Remaining underwater without air causes periodic damage; air is not a world survival timer."},
 {"category":"Water & Diving","title":"Underwater grottos","tip":"Every lake has a submerged grotto with solid walls, floors and ceilings."},
 {"category":"Water & Diving","title":"Abyssal caches","tip":"Check each grotto for a resource cache. Loot replenishes on a later world day."},
 {"category":"Water & Diving","title":"Cave navigation","tip":"Avoid swimming into rock walls or pillars; use open passages and change depth to escape."},
 {"category":"Water & Diving","title":"Stay near the surface","tip":"If your oxygen is low, ascend before exploring another hidden corner."},
 {"category":"Professions & Crafting","title":"Open profession skills","tip":"The Professions tab tracks gathering and crafting progress and shows available recipes."},
 {"category":"Professions & Crafting","title":"Fishing","tip":"Visit a lake gathering site to collect Moonwater Fish and improve fishing skill."},
 {"category":"Professions & Crafting","title":"Herbalism","tip":"Collect reeds, herbs and Moonblooms where gathering sites are available."},
 {"category":"Professions & Crafting","title":"Mining","tip":"Gather ore at mining deposits, then use metal resources for smithing."},
 {"category":"Professions & Crafting","title":"Skinning","tip":"Track eligible wildlife and beasts for hides and skinning experience."},
 {"category":"Professions & Crafting","title":"Leatherworking","tip":"Turn hides and materials into useful leatherworking products."},
 {"category":"Professions & Crafting","title":"Cooking","tip":"Cook gathered ingredients into healing or exploration supplies."},
 {"category":"Professions & Crafting","title":"Alchemy","tip":"Brew tonics for temporary protection or other useful effects."},
 {"category":"Professions & Crafting","title":"Smithing","tip":"Forge gear when you have the required ore and other ingredients."},
 {"category":"Professions & Crafting","title":"Crafting costs","tip":"Check material requirements before crafting. Resources are consumed only on a successful recipe."},
 {"category":"Professions & Crafting","title":"Moon Sigil Forge","tip":"Earn Moon Sigils from world activities and use them for advanced crafting recipes."},
 {"category":"Professions & Crafting","title":"Resource regeneration","tip":"Gathering sites replenish on a world-day cycle, so visit again after time advances."},
 {"category":"Professions & Crafting","title":"Regional contracts","tip":"Complete eligible regional gathering contracts for rewards without adding another game mode."},
 {"category":"Gear & Economy","title":"Inventory capacity","tip":"Your equipment bag has a capacity limit. Upgrade it when your coin balance allows."},
 {"category":"Gear & Economy","title":"Material storage","tip":"Crafting materials use separate storage rather than occupying equipment bag slots."},
 {"category":"Gear & Economy","title":"Sort your bag","tip":"Inventory filters and sorting help compare equipment by rarity, power and name."},
 {"category":"Gear & Economy","title":"Use the vault","tip":"Deposit unused equipment in the vault and withdraw it later."},
 {"category":"Gear & Economy","title":"Equip with purpose","tip":"Compare power and your current combat role before equipping a weapon or armor piece."},
 {"category":"Gear & Economy","title":"Check sell value","tip":"A merchant pays based on item value; consider whether the gear is more useful than the coins."},
 {"category":"Gear & Economy","title":"Coin exchange","tip":"100 copper make one silver, and 100 silver make one gold."},
 {"category":"Gear & Economy","title":"Find merchants","tip":"Visit Hearthstone Outpost away from combat for healing supplies and trading."},
 {"category":"Gear & Economy","title":"Collect drops","tip":"Pick up loot after fights and keep crafting ingredients for profession recipes."},
 {"category":"Creatures & Companions","title":"Hunter Senses","tip":"Toggle senses with F to help track wildlife and nearby points of interest."},
 {"category":"Creatures & Companions","title":"Hunting contracts","tip":"Accept regional hunts from the Hunter tab and defeat the required hostile beasts."},
 {"category":"Creatures & Companions","title":"Tame a companion","tip":"Try T near an eligible creature to attempt taming."},
 {"category":"Creatures & Companions","title":"Choose companion stance","tip":"H cycles Guard, Follow and Passive, changing how your companion handles threats."},
 {"category":"Creatures & Companions","title":"Guard stance","tip":"Guard fights nearby dangers more actively, useful when traveling through hostile areas."},
 {"category":"Creatures & Companions","title":"Follow stance","tip":"Follow supports fights you start instead of attacking every potential threat."},
 {"category":"Creatures & Companions","title":"Passive stance","tip":"Passive avoids engaging in combat while you explore."},
 {"category":"Creatures & Companions","title":"Mount safely","tip":"M mounts or dismounts where available. Sustained hostile combat can force a dismount."},
 {"category":"Creatures & Companions","title":"Use the Bestiary","tip":"The Bestiary lists creature families, hostility, skins and possible trophies."},
 {"category":"Creatures & Companions","title":"Wildlife reactions","tip":"Some roaming creatures flee approaching players. Move gently while tracking them."},
 {"category":"Death & Recovery","title":"Resurrection","tip":"If you fall, use the resurrection screen to return to a sanctuary."},
 {"category":"Death & Recovery","title":"Persistent progress","tip":"Moonfall preserves key character, gear, material and progression information across death."},
 {"category":"Death & Recovery","title":"Phoenix Feather","tip":"A Phoenix Feather can allow revival when available, but may affect flawless-boss rewards."},
 {"category":"Death & Recovery","title":"Plan ahead","tip":"Bring healing items and unlock shrines before heading into dangerous regions."},
 {"category":"Death & Recovery","title":"Recover after defeat","tip":"Review gear, professions and recent combat choices before trying the encounter again."},
 {"category":"Controls & Accessibility","title":"Keyboard movement","tip":"WASD or arrow keys steer your hero; movement responds to the third-person camera."},
 {"category":"Controls & Accessibility","title":"Mouse camera","tip":"Hold right mouse and drag to orbit the third-person camera."},
 {"category":"Controls & Accessibility","title":"Camera zoom","tip":"Mouse wheel or configured zoom inputs change your viewing distance."},
 {"category":"Controls & Accessibility","title":"Gamepad support","tip":"Move with left stick, orbit camera with right stick and open the menu with Start."},
 {"category":"Controls & Accessibility","title":"Menu by controller","tip":"Raven Atlas buttons are focusable; navigate and activate them using standard UI focus controls."},
 {"category":"Controls & Accessibility","title":"Quick actions","tip":"On-screen quick actions reduce the need to memorize every combat and interaction shortcut."},
 {"category":"Controls & Accessibility","title":"Tips shortcut","tip":"Press F1 during play to jump directly to the Tips Codex; press F1 again to return to the world."},
 {"category":"Controls & Accessibility","title":"Display options","tip":"Use the System/graphics options to adjust quality, resolution, fullscreen and 4K settings."},
 {"category":"Controls & Accessibility","title":"Readable cues","tip":"Boss warnings and oxygen alerts communicate danger by both color and descriptive text."},
 {"category":"Controls & Accessibility","title":"No single-switch mode","tip":"Moonfall does not include the old single-switch game mode. Its separate accessibility projects are unaffected."},
 {"category":"Performance & Saves","title":"Lower graphics first","tip":"If battles stutter, try the smooth graphics preset before sacrificing responsive controls."},
 {"category":"Performance & Saves","title":"Use adaptive scaling","tip":"Dynamic render scaling can maintain responsiveness at high resolutions."},
 {"category":"Performance & Saves","title":"4K display settings","tip":"A 4K display can increase GPU load; choose an appropriate render scale for your PC."},
 {"category":"Performance & Saves","title":"Save checkpoint","tip":"Use the System menu save action before exiting or traveling far."},
 {"category":"Performance & Saves","title":"Continue progress","tip":"Use Continue on the hero-select screen when a compatible checkpoint is available."},
 {"category":"Performance & Saves","title":"Use diagnostics","tip":"The System tab has world diagnostics to review performance counters and active objects."},
 {"category":"Performance & Saves","title":"Avoid save conflicts","tip":"Wait for checkpoint actions to complete before closing the game."},
 {"category":"Performance & Saves","title":"Report a reproducible issue","tip":"Include the region, hero, action and game version when reporting bugs."},
]

static func matching(category:String="All",query:String="")->Array[Dictionary]:
 var hits:Array[Dictionary]=[]
 var needle:String=query.strip_edges().to_lower().substr(0,80)
 for row in ENTRIES:
  if category!="All" and str(row["category"])!=category:continue
  if needle!="" and not (needle in str(row["title"]).to_lower() or needle in str(row["tip"]).to_lower() or needle in str(row["category"]).to_lower()):continue
  hits.append(row)
 return hits

static func menu_tip(tab:String)->String:
 match tab:
  "Character":return "Your health, level, strength, equipped gear and nearby recommended power."
  "World":return "Explore six connected regions, rifts, shrines, invasions and world events."
  "Wallet":return "Coin balances, merchants, exchanges and buying or selling."
  "Inventory":return "View, equip, search, sort, deposit and withdraw equipment."
  "Materials":return "Inspect the crafting resources stored separately from your gear."
  "Hero Skills":return "Spend level-up points in Assault, Survival and Arcana."
  "Tamer":return "Improve your animal companion and its preferred combat behavior."
  "Professions":return "Fish, gather, skin, cook, brew, forge and craft."
  "Hunter":return "Accept hunts, track animals and prepare for dangerous encounters."
  "Bestiary":return "Identify wildlife, bosses, trophies and creature families."
  "System":return "Save, graphics, controller options, diagnostics and menus."
  "Tips":return "Search every Moonfall help topic. Press F1 while playing."
 return "Open the Raven Atlas to learn about this system."

static func action_tip(action:String,label:String)->String:
 if action.begins_with("tipcat:"):return "Show tips in this topic."
 if action.begins_with("tippage:"):return "Browse the next or previous page of tips."
 if action.begins_with("hero_perk:") or action.begins_with("talent:"):return "Spend one available point to improve this skill. Confirm your choice carefully."
 if action.begins_with("craft:") or action.begins_with("v37craft:"):return "Craft the selected recipe if you have sufficient materials."
 if action.begins_with("travel:"):return "Fast-travel to an awakened shrine while outside combat."
 if action.begins_with("contract:"):return "Accept a regional hunting contract to earn rewards from defeating beasts."
 if action.begins_with("equip:"):return "Equip this gear. Compare its power with your current item."
 if action.begins_with("inventory_deposit:"):return "Move this item from your bag into your vault."
 if action.begins_with("inventory_withdraw:"):return "Take this stored item back to your bag if space allows."
 if action.begins_with("wallet_sell:"):return "Sell this item to the merchant. The action spends your item."
 if action.begins_with("portrait:"):return "View this creature's original reference portrait."
 if action.begins_with("inventory_"):return "Change the equipment bag view, filter, page or available slots."
 if "heal" in action:return "Use a healing item or a sanctuary to restore health."
 if "save" in action:return "Write your current checkpoint and progression."
 if "load" in action:return "Restore your most recent compatible checkpoint."
 if "senses" in action:return "Toggle Hunter Senses to help locate wildlife or nearby targets."
 if "stance" in action:return "Change your companion behavior: Guard, Follow or Passive."
 if "interact" in action:return "Interact with the nearby item, shrine, loot, gathering site or event."
 if "minimap" in action:return "Toggle the circular minimap."
 if "graphics" in action or action in ["smooth","full"]:return "Change display quality. Lower settings can reduce GPU load."
 if action=="dodge":return "Evade incoming attacks; timing matters for boss warning zones."
 if action=="resume":return "Close the Atlas and return to gameplay."
 if action=="title":return "Return to the hero selection screen."
 return "" if label=="" else "Use this Atlas action to manage your current adventure."
