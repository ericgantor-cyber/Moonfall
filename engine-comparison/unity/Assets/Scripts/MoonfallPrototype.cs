using UnityEngine;
using System.Collections.Generic;
// Experimental primitive-only Moonfall combat slice, separate from production Godot.
public sealed class MoonfallPrototype : MonoBehaviour
{
    Transform hero, boss, warning;
    Transform bossPortrait;
    Texture2D zPortrait, nyraPortrait; // Original art belongs to gods/bosses, not the playable avatar.
    Camera view;
    Texture2D ornateOrbs; // Optional transparent art imported to Resources/MoonfallOrbs
    float hp, bossHp = 100, attackCd, dodge, dodgeCooldown, castCd = 3, castTimer, zoom = 16;
    const float MaxMana = 180f;
    const float MaxStamina = 150f;
    float mana = MaxMana, stamina = MaxStamina;
    float boltCooldown, healCooldown;
    GUIStyle orbDigits;
    bool casting, won;
    Vector3 target;
    int wins;
    readonly MoonfallProgression progression = new MoonfallProgression();
    string bossLoot = "No boss loot yet";
    readonly List<string> bossInventory = new List<string>();
    sealed class WorldEnemy
    {
        public Transform body;
        public Transform portrait;
        public float hp;
        public int region;
        public int id;
        public bool guardian;
        public Vector3 home;
    }
    readonly List<WorldEnemy> enemies = new List<WorldEnemy>();
    int defeatedEnemyMask;
    float enemyDamageCooldown;
    float checkpointTimer;
    const float CheckpointInterval = 8f;
    int claimedChamberMask;
    int defeatedGuardianMask;
    readonly List<Transform> chamberRewards = new List<Transform>();
    readonly List<int> chamberIds = new List<int>();
    readonly List<Transform> regionArt = new List<Transform>();
    static readonly string[] BeastPortraits =
        { "wolf", "fox", "boar", "elk", "panther", "serpent" };

    [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
    static void Boot()
    {
        if (FindFirstObjectByType<MoonfallPrototype>() == null)
            new GameObject("Moonfall Unity Experiment").AddComponent<MoonfallPrototype>();
    }
    static Transform Make(string title, PrimitiveType type, Vector3 pos, Vector3 scale, Color color)
    {
        var obj = GameObject.CreatePrimitive(type);
        obj.name = title;
        obj.transform.position = pos;
        obj.transform.localScale = scale;
        obj.GetComponent<Renderer>().material.color = color;
        return obj.transform;
    }
    // Preserve the original portrait pixels. A camera-facing sprite displays the
    // imported art without redrawing or replacing the underlying source.
    static Transform AttachPortrait(Transform anchor, Texture2D original, float unitsHigh)
    {
        if (original == null) return null;
        var obj = new GameObject("Original artwork billboard");
        obj.transform.SetParent(anchor, false);
        obj.transform.localPosition = Vector3.zero;
        var renderer = obj.AddComponent<SpriteRenderer>();
        var sprite = Sprite.Create(original,
            new Rect(0f, 0f, original.width, original.height),
            new Vector2(.5f, .08f), original.height / unitsHigh,
            0, SpriteMeshType.FullRect);
        renderer.sprite = sprite;
        renderer.sortingOrder = 8;
        var mesh = anchor.GetComponent<Renderer>();
        if (mesh != null) mesh.enabled = false; // Placeholder hidden only if real art loaded.
        return obj.transform;
    }

    void LateUpdate()
    {
        // SpriteRenderer local +Z faces camera; only billboard transforms rotate.
        if (view == null) return;
        if (bossPortrait != null) bossPortrait.rotation = view.transform.rotation;
        foreach (WorldEnemy enemy in enemies)
            if (enemy.portrait != null) enemy.portrait.rotation = view.transform.rotation;
        foreach (Transform landmark in regionArt)
            if (landmark != null) landmark.rotation = view.transform.rotation;
    }

    // These two new AI-generated landscape crops contain no hero artwork or UI.
    // The imported files are optional. Missing images never interrupt exploration.
    void PlaceAilandmark(string resource, string name, Vector3 position, float height)
    {
        Texture2D art = Resources.Load<Texture2D>("MoonfallGeneratedArt/" + resource);
        if (art == null) return;
        var anchor = new GameObject(name);
        anchor.transform.position = position;
        Transform picture = AttachPortrait(anchor.transform, art, height);
        if (picture != null) regionArt.Add(picture);
    }

    void BuildWorld()
    {
        defeatedEnemyMask = PlayerPrefs.GetInt("moonfall_experiment_defeated_mobs", 0) & ((1 << MoonfallWorldRules.TotalEnemies) - 1);
        claimedChamberMask = PlayerPrefs.GetInt("moonfall_experiment_chambers", 0) & ((1 << MoonfallWorldRules.TotalChambers) - 1);
        defeatedGuardianMask = PlayerPrefs.GetInt("moonfall_experiment_guardians", 0) & ((1 << MoonfallWorldRules.TotalChambers) - 1);
        // A chamber cannot be claimed before its guardian (including Nyra) falls.
        // Old or interrupted saves may contain a claimed bit without a defeat bit.
        // Reopen only those inconsistent chambers rather than skipping encounters.
        claimedChamberMask &= defeatedGuardianMask;
        for (int region = 0; region < MoonfallWorldRules.RegionCount; region++)
        {
            float center = MoonfallWorldRules.RegionCenter(region);
            Color ground = Color.Lerp(new Color(.17f, .21f, .25f),
                new Color(.30f, .12f, .16f), region / 5f);
            Make("Connected region " + region, PrimitiveType.Cube,
                new Vector3(center, -.3f, 0f), new Vector3(34f, .5f, 34f), ground);
            // Imported scene artwork as a landmark, without replacing the floor collision.
            Texture2D vista = Resources.Load<Texture2D>(
                "MoonfallArt/v31_4_bestiary/region_" + region);
            if (vista != null)
            {
                var landmark = new GameObject("Region " + region + " original art landmark");
                landmark.transform.position = new Vector3(center, 3f, 13f);
                Transform art = AttachPortrait(landmark.transform, vista, 7f);
                if (art != null) regionArt.Add(art);
            }
            if (region == 0)
                PlaceAilandmark("moonfall_ai_waterfall_ruins",
                    "New AI art waterfall ruins", new Vector3(center - 12f, 4f, 13f), 8f);
            if (region == 4)
                PlaceAilandmark("moonfall_ai_moonlit_gate",
                    "New AI art moonlit dungeon gate", new Vector3(center + 10f, 4f, 13f), 9f);
            // Five side chambers per region overlap its floor edge, so players
            // walk straight into them without teleporting or changing game modes.
            for (int chamber = 0; chamber < MoonfallWorldRules.ChambersPerRegion; chamber++)
            {
                Vector3 centerRoom = MoonfallWorldRules.ChamberCenter(region, chamber);
                Make("Region " + region + " chamber " + chamber, PrimitiveType.Cube,
                    new Vector3(centerRoom.x, -.3f, centerRoom.z),
                    new Vector3(16f, .5f, 16f), ground * .8f);
                int chamberId = region * MoonfallWorldRules.ChambersPerRegion + chamber;
                if ((claimedChamberMask & (1 << chamberId)) != 0) continue;
                Transform cache = Make("Dungeon cache " + chamberId, PrimitiveType.Cylinder,
                    MoonfallWorldRules.ChamberReward(region, chamber),
                    new Vector3(1.6f, .5f, 1.6f), new Color(.9f, .6f, .18f));
                Texture2D cacheArt = Resources.Load<Texture2D>(
                    "MoonfallArt/v31_3_art/treasure_cache");
                Transform cachePortrait = AttachPortrait(cache, cacheArt, 2f);
                if (cachePortrait != null) regionArt.Add(cachePortrait);
                chamberRewards.Add(cache);
                chamberIds.Add(chamberId);
                if ((defeatedGuardianMask & (1 << chamberId)) == 0)
                {
                    Vector3 guardPosition = MoonfallWorldRules.ChamberReward(region, chamber) + new Vector3(0f, 0f, 4f);
                    bool isNyraEncounter = chamberId == MoonfallWorldRules.TotalChambers - 1;
                    Transform guardian = Make(isNyraEncounter ? "Nyra god guardian" : "Chamber guardian " + chamberId,
                        PrimitiveType.Capsule, guardPosition, new Vector3(1.3f, 1.3f, 1.3f),
                        new Color(.65f, .12f, .8f));
                    Texture2D guardianArt = isNyraEncounter ? nyraPortrait :
                        Resources.Load<Texture2D>("MoonfallArt/v31_4_bestiary/boss_" + (chamber % 4));
                    enemies.Add(new WorldEnemy {
                        body = guardian,
                        portrait = AttachPortrait(guardian, guardianArt, 2.4f),
                        region = region, id = chamberId, guardian = true, home = guardPosition,
                        hp = MoonfallWorldRules.EnemyMaxHealth(region) * 1.8f
                    });
                }
            }
            Texture2D beastArt = Resources.Load<Texture2D>(
                "MoonfallArt/v31_4_bestiary/" + BeastPortraits[region]);
            for (int slot = 0; slot < MoonfallWorldRules.EnemiesPerRegion; slot++)
            {
                int id = region * MoonfallWorldRules.EnemiesPerRegion + slot;
                if ((defeatedEnemyMask & (1 << id)) != 0) continue;
                Transform mob = Make("Region " + region + " beast " + slot,
                    PrimitiveType.Capsule, MoonfallWorldRules.EnemyPosition(region, slot),
                    Vector3.one, new Color(.56f, .26f, .30f));
                enemies.Add(new WorldEnemy
                {
                    body = mob, portrait = AttachPortrait(mob, beastArt, 2f),
                    region = region, id = id, home = mob.position,
                    hp = MoonfallWorldRules.EnemyMaxHealth(region)
                });
            }
        }
    }

    void UpdateEnemies(float dt)
    {
        // Lightweight proximity AI, no spawn waves, navigation allocations or global aggro.
        foreach (WorldEnemy enemy in enemies)
        {
            Vector3 offset = hero.position - enemy.body.position;
            offset.y = 0f;
            float distance = offset.magnitude;
            float leash = enemy.guardian ? 6f : 9f;
            Vector3 fromHome = enemy.body.position - enemy.home;
            fromHome.y = 0f;
            if (distance > 1.6f && distance < 8f && fromHome.magnitude < leash)
            {
                float step = Mathf.Min((enemy.guardian ? 2.8f : 2f) * dt,
                    Mathf.Max(0f, leash - fromHome.magnitude));
                enemy.body.position += offset.normalized * step;
            }
        }
    }

    WorldEnemy NearestEnemy(float radius)
    {
        WorldEnemy closest = null;
        float distance = radius;
        foreach (WorldEnemy enemy in enemies)
        {
            float candidate = Vector3.Distance(hero.position, enemy.body.position);
            if (candidate < distance) { distance = candidate; closest = enemy; }
        }
        return closest;
    }

    void SaveProgress()
    {
        PlayerPrefs.SetInt("moonfall_experiment_level", progression.Level);
        PlayerPrefs.SetInt("moonfall_experiment_xp", progression.Experience);
        PlayerPrefs.SetInt("moonfall_experiment_hero_points", progression.HeroPoints);
        PlayerPrefs.SetInt("moonfall_experiment_tamer_points", progression.TamerPoints);
        PlayerPrefs.SetInt("moonfall_experiment_defeated_mobs", defeatedEnemyMask);
        PlayerPrefs.SetInt("moonfall_experiment_chambers", claimedChamberMask);
        PlayerPrefs.SetInt("moonfall_experiment_guardians", defeatedGuardianMask);
        // Save current HP and position together, only when the hero is alive.
        if (hero != null && hp > 0f)
        {
            PlayerPrefs.SetFloat("moonfall_experiment_checkpoint_x", hero.position.x);
            PlayerPrefs.SetFloat("moonfall_experiment_checkpoint_z", hero.position.z);
            PlayerPrefs.SetFloat("moonfall_experiment_checkpoint_hp", hp);
            // Restore spent mana and stamina exactly, not free resource refills.
            PlayerPrefs.SetFloat("moonfall_experiment_checkpoint_mana", mana);
            PlayerPrefs.SetFloat("moonfall_experiment_checkpoint_stamina", stamina);
            PlayerPrefs.SetInt("moonfall_experiment_checkpoint_resources_active", 1);
            PlayerPrefs.SetInt("moonfall_experiment_checkpoint_active", 1);
        }
        PlayerPrefs.Save();
    }

    // A suspend or clean exit should preserve recent exploration progress instead
    // of relying solely on the eight-second checkpoint tick.
    // SaveProgress preserves the previous living checkpoint when HP is zero.
    void OnApplicationPause(bool paused)
    {
        if (paused && hero != null) SaveProgress();
    }

    void OnApplicationQuit()
    {
        if (hero != null) SaveProgress();
    }

    void CollectNearbyChambers()
    {
        // Accessible proximity collection: no extra HUD button or prompt.
        for (int i = chamberRewards.Count - 1; i >= 0; i--)
        {
            if (Vector3.Distance(hero.position, chamberRewards[i].position) > 2.5f) continue;
            int id = chamberIds[i];
            if ((defeatedGuardianMask & (1 << id)) == 0) continue; // Guardian must fall first.
            claimedChamberMask |= 1 << id;
            // Permanent exploration reward without extra boss-only HP stacking.
            progression.AwardDefeat(id / MoonfallWorldRules.ChambersPerRegion,
                false, false, Time.time);
            mana = Mathf.Min(MaxMana, mana + 25f);
            Destroy(chamberRewards[i].gameObject);
            chamberRewards.RemoveAt(i);
            chamberIds.RemoveAt(i);
            SaveProgress();
        }
    }

    // Lightweight class identity. No changes to Z or Nyra's original artwork.
    // Neutral adventurer uses one consistent combat profile, not a god's identity.
    float BasicAttackDamage() { return 18f + progression.Level; }
    float ArcaneBoltDamage() { return 28f + 2f * progression.Level; }
    float HealingSpellAmount() { return 30f + progression.Level; }

    void DamageEnemy(WorldEnemy enemy, float damage)
    {
        if (enemy == null) return;
        enemy.hp = Mathf.Max(0f, enemy.hp - damage);
        if (enemy.hp > 0f) return;
        if (enemy.guardian) defeatedGuardianMask |= 1 << enemy.id;
        else defeatedEnemyMask |= 1 << enemy.id;
        // Kills award XP and loot progression, never automatic HP healing.
        progression.AwardDefeat(enemy.region, enemy.guardian, false, Time.time);
        enemies.Remove(enemy);
        Destroy(enemy.body.gameObject);
        SaveProgress();
    }

    void Start()
    {
        ornateOrbs = Resources.Load<Texture2D>("MoonfallOrbs");
        wins = Mathf.Clamp(PlayerPrefs.GetInt("moonfall_experiment_wins", 0), 0, 100);
        won = PlayerPrefs.GetInt("moonfall_experiment_boss_defeated", 0) == 1;
        hp = 100 + 5 * wins;
        progression.Level = Mathf.Clamp(PlayerPrefs.GetInt("moonfall_experiment_level", 1), 1, 100);
        progression.Experience = Mathf.Clamp(PlayerPrefs.GetInt("moonfall_experiment_xp", 0), 0, progression.ExperienceToNext - 1);
        progression.HeroPoints = Mathf.Clamp(PlayerPrefs.GetInt("moonfall_experiment_hero_points", 0), 0, 100);
        progression.TamerPoints = Mathf.Clamp(PlayerPrefs.GetInt("moonfall_experiment_tamer_points", 0), 0, 100);
        // Prototype-only collectible slots, separate from production save data.
        int relicCount = Mathf.Clamp(PlayerPrefs.GetInt("moonfall_experiment_relic_count", 0), 0, 100);
        for (int i = 0; i < relicCount; i++)
        {
            int rank = PlayerPrefs.GetInt("moonfall_experiment_relic_" + i, -1);
            if (rank >= 0 && rank < MoonfallLootRules.Rarities.Length)
                bossInventory.Add(MoonfallLootRules.Rarities[rank] + " boss relic (prototype)");
        }
        zPortrait = Resources.Load<Texture2D>("MoonfallArt/heroes/Z_Original");
        nyraPortrait = Resources.Load<Texture2D>("MoonfallArt/heroes/Nyra_Original");
        BuildWorld();
        hero = Make("Hero placeholder", PrimitiveType.Capsule, new Vector3(0,1,-5), Vector3.one, Color.cyan);
        if (PlayerPrefs.GetInt("moonfall_experiment_checkpoint_active", 0) == 1)
        {
            float savedX = PlayerPrefs.GetFloat("moonfall_experiment_checkpoint_x", 0f);
            float savedZ = PlayerPrefs.GetFloat("moonfall_experiment_checkpoint_z", -5f);
            float savedHp = PlayerPrefs.GetFloat("moonfall_experiment_checkpoint_hp", hp);
            if (!float.IsNaN(savedX) && !float.IsInfinity(savedX) &&
                !float.IsNaN(savedZ) && !float.IsInfinity(savedZ) &&
                !float.IsNaN(savedHp) && !float.IsInfinity(savedHp) && savedHp > 0f)
            {
                hero.position = new Vector3(Mathf.Clamp(savedX, -16f, 176f), 1f,
                    Mathf.Clamp(savedZ, -32f, 32f));
                hp = Mathf.Clamp(savedHp, 1f, 100f + 5f * wins);
                // Old saves have no resource checkpoint and keep default bars.
                if (PlayerPrefs.GetInt("moonfall_experiment_checkpoint_resources_active", 0) == 1)
                {
                    float savedMana = PlayerPrefs.GetFloat("moonfall_experiment_checkpoint_mana", MaxMana);
                    float savedStamina = PlayerPrefs.GetFloat("moonfall_experiment_checkpoint_stamina", MaxStamina);
                    if (!float.IsNaN(savedMana) && !float.IsInfinity(savedMana))
                        mana = Mathf.Clamp(savedMana, 0f, MaxMana);
                    if (!float.IsNaN(savedStamina) && !float.IsInfinity(savedStamina))
                        stamina = Mathf.Clamp(savedStamina, 0f, MaxStamina);
                }
            }
        }
        boss = Make("Boss placeholder", PrimitiveType.Capsule, new Vector3(MoonfallWorldRules.BossX,1,6), new Vector3(2,2,2), Color.red);
        // Z is the main boss. His original pixels are never used as a player skin.
        bossPortrait = AttachPortrait(boss, zPortrait, 3.6f);
        if (won) { bossHp = 0f; boss.gameObject.SetActive(false); }
        warning = Make("Attack telegraph", PrimitiveType.Cylinder, Vector3.zero, new Vector3(5,.015f,5), new Color(1,.4f,.1f));
        warning.gameObject.SetActive(false);
        view = Camera.main;
        if (view == null) { var go = new GameObject("Camera"); go.tag="MainCamera"; view = go.AddComponent<Camera>(); }
    }
    void Update()
    {
        float dt = Mathf.Max(0, Time.deltaTime);
        attackCd = Mathf.Max(0, attackCd-dt);
        dodge = Mathf.Max(0, dodge-dt);
        dodgeCooldown = Mathf.Max(0, dodgeCooldown-dt);
        stamina = Mathf.Min(MaxStamina, stamina + 30f * dt);
        mana = Mathf.Min(MaxMana, mana + 8f * dt);
        boltCooldown = Mathf.Max(0f, boltCooldown - dt);
        healCooldown = Mathf.Max(0f, healCooldown - dt);
        enemyDamageCooldown = Mathf.Max(0f, enemyDamageCooldown - dt);
        checkpointTimer += dt;
        if (hp > 0)
        {
            Vector3 direction = new Vector3(Input.GetAxisRaw("Horizontal"),0,Input.GetAxisRaw("Vertical")).normalized;
            if (Input.GetKeyDown(KeyCode.LeftShift) && dodgeCooldown<=0 && stamina >= 45f)
            {
                dodge=.32f;
                dodgeCooldown=1.2f;
                stamina -= 45f;
            }
            // Magic uses keys, keeping the left mouse button exclusively for attacks.
            if (Input.GetKeyDown(KeyCode.Q) && boltCooldown <= 0f && mana >= 30f
                && (NearestEnemy(10f) != null || (!won && Vector3.Distance(hero.position, boss.position) <= 10f)))
            {
                mana -= 30f;
                boltCooldown = 1.1f;
                WorldEnemy foe = NearestEnemy(10f);
                if (foe != null) DamageEnemy(foe, ArcaneBoltDamage());
                else
                {
                    bossHp = Mathf.Max(0f, bossHp - ArcaneBoltDamage());
                    if (bossHp <= 0f) Victory();
                }
            }
            if (Input.GetKeyDown(KeyCode.E) && healCooldown <= 0f && mana >= 40f
                && hp < 100f + 5f * wins)
            {
                mana -= 40f;
                healCooldown = 5f;
                hp = Mathf.Min(100f + 5f * wins, hp + HealingSpellAmount());
            }
            hero.position += direction * (dodge>0 ? 12 : 6) * dt;
            hero.position = new Vector3(Mathf.Clamp(hero.position.x,-16,176),1,Mathf.Clamp(hero.position.z,-32,32));
            // Throttled checkpoints save travel and actual remaining HP, never healing.
            if (checkpointTimer >= CheckpointInterval)
            {
                checkpointTimer = 0f;
                SaveProgress();
            }
            CollectNearbyChambers();
            UpdateEnemies(dt);
            WorldEnemy nearby = NearestEnemy(1.75f);
            if (nearby != null && enemyDamageCooldown <= 0f && dodge <= 0f)
            {
                hp = Mathf.Max(0f, hp - (5f + 2f * nearby.region));
                enemyDamageCooldown = 1.2f;
            }
            if (Input.GetMouseButtonDown(0) && attackCd<=0)
            {
                attackCd=.35f;
                WorldEnemy foe = NearestEnemy(4f);
                if (foe != null) DamageEnemy(foe, BasicAttackDamage());
                else if (!won && Vector3.Distance(hero.position,boss.position)<4f)
                {
                    bossHp=Mathf.Max(0,bossHp-BasicAttackDamage());
                    if (bossHp<=0) Victory();
                }
            }
        }
        zoom=Mathf.Clamp(zoom-Input.mouseScrollDelta.y*1.25f,9,25);
        view.transform.position=Vector3.Lerp(view.transform.position,hero.position+new Vector3(0,zoom,-zoom),Mathf.Clamp01(dt*8));
        view.transform.LookAt(hero.position);
        if (won || hp<=0) return;
        if (Vector3.Distance(hero.position, boss.position) > 16f)
        {
            // World exploration stays safe from a boss in another region.
            casting = false;
            warning.gameObject.SetActive(false);
            castCd = Mathf.Max(1f, castCd);
            return;
        }
        if (!casting)
        {
            castCd-=dt;
            if (castCd<=0)
            {
                target=new Vector3(hero.position.x,.02f,hero.position.z);
                warning.position=target;
                warning.gameObject.SetActive(true);
                casting=true;
                castTimer=1.15f;
            }
        }
        else if ((castTimer-=dt)<=0)
        {
            casting=false;
            warning.gameObject.SetActive(false);
            castCd=3.5f;
            if (dodge<=0 && Vector2.Distance(new Vector2(hero.position.x,hero.position.z),new Vector2(target.x,target.z))<2.5f)
                hp=Mathf.Max(0,hp-20);
        }
    }
    void Victory()
    {
        won=true;
        PlayerPrefs.SetInt("moonfall_experiment_boss_defeated", 1);
        warning.gameObject.SetActive(false);
        boss.gameObject.SetActive(false);
        wins = Mathf.Min(100, wins + 1);
        PlayerPrefs.SetInt("moonfall_experiment_wins", wins);
        PlayerPrefs.Save();
        // Permanent victory increases max HP capacity, not current HP.
        progression.AwardDefeat(0, true, true, Time.time);
        SaveProgress();
        // One provisional boss loot roll, separate from permanent HP bonuses.
        // Uses Godot-derived rarity thresholds but not Godot's exact seeded RNG.
        int rank = MoonfallLootRules.RarityIndex(Random.Range(0, 1000), 0, 0);
        bossLoot = MoonfallLootRules.Rarities[rank] + " boss relic (prototype)";
        if (bossInventory.Count < 100)
        {
            PlayerPrefs.SetInt("moonfall_experiment_relic_" + bossInventory.Count, rank);
            bossInventory.Add(bossLoot);
            PlayerPrefs.SetInt("moonfall_experiment_relic_count", bossInventory.Count);
            PlayerPrefs.Save();
        }
    }
    // Lightweight immediate-mode prototype HUD. Replace with optimized UI Toolkit/UGUI
    // only after this engine is selected; no asset duplication or per-frame textures.
    static void FillRect(Rect rect, Color tint)
    {
        Color previous = GUI.color;
        GUI.color = tint;
        GUI.DrawTexture(rect, Texture2D.whiteTexture);
        GUI.color = previous;
    }

    static void Ring(Vector2 center, float radius, float fraction, Color tint)
    {
        fraction = Mathf.Clamp01(fraction);
        const int segments = 48;
        for (int i = 0; i < segments; i++)
        {
            float angle = i * Mathf.PI * 2f / segments - Mathf.PI * .5f;
            float angleEnd = (i + 1) * Mathf.PI * 2f / segments - Mathf.PI * .5f;
            Vector2 a = center + new Vector2(Mathf.Cos(angle), Mathf.Sin(angle)) * radius;
            Vector2 b = center + new Vector2(Mathf.Cos(angleEnd), Mathf.Sin(angleEnd)) * radius;
            Vector2 delta = b - a;
            Matrix4x4 previous = GUI.matrix;
            GUIUtility.RotateAroundPivot(Mathf.Atan2(delta.y, delta.x) * Mathf.Rad2Deg, a);
            FillRect(new Rect(a.x, a.y - 3f, delta.magnitude + 1f, 6f),
                i < Mathf.RoundToInt(fraction * segments) ? tint : new Color(.18f, .18f, .24f, .9f));
            GUI.matrix = previous;
        }
    }

    // Values are drawn only inside the existing transparent orb art.
    void DrawOrbValue(Rect bounds, string amount, float fontScale)
    {
        if (orbDigits == null)
        {
            orbDigits = new GUIStyle(GUI.skin.label);
            orbDigits.alignment = TextAnchor.MiddleCenter;
            orbDigits.fontStyle = FontStyle.Bold;
            orbDigits.normal.textColor = Color.white;
        }
        orbDigits.fontSize = Mathf.Max(11, Mathf.RoundToInt(fontScale));
        GUI.Label(bounds, amount, orbDigits);
    }

    void DrawRpgHud()
    {
        float maxHp = 100f + 5f * wins;
        if (ornateOrbs != null)
        {
            // Compact transparent graphic: no panel, no opaque backdrop.
            float width = Mathf.Min(300f, Screen.width * .34f);
            float height = width * ornateOrbs.height / ornateOrbs.width;
            float left = 12f, top = Screen.height - height - 12f;
            GUI.DrawTexture(new Rect(left, top, width, height), ornateOrbs, ScaleMode.StretchToFill, true);
            float sx = width / 680f, sy = height / 424f;
            // Coordinates are relative to the transparent 680×424 orb art.
            DrawOrbValue(new Rect(left + 240*sx, top + 150*sy, 200*sx, 80*sy),
                Mathf.CeilToInt(hp) + "/" + Mathf.CeilToInt(maxHp), 29f * sx);
            DrawOrbValue(new Rect(left + 45*sx, top + 222*sy, 150*sx, 54*sy),
                Mathf.CeilToInt(mana) + "/" + Mathf.CeilToInt(MaxMana), 27f * sx);
            DrawOrbValue(new Rect(left + 495*sx, top + 222*sy, 150*sx, 54*sy),
                progression.Experience + "/" + progression.ExperienceToNext, 24f * sx);
            DrawOrbValue(new Rect(left + 270*sx, top + 340*sy, 140*sx, 44*sy),
                Mathf.CeilToInt(stamina) + "/" + Mathf.CeilToInt(MaxStamina), 22f * sx);
        }
        else
        {
        Ring(new Vector2(90f, Screen.height - 108f), 44f, hp / Mathf.Max(1f, maxHp),
            new Color(1f, .22f, .30f));
        Ring(new Vector2(90f, Screen.height - 108f), 34f, mana / MaxMana, new Color(.18f, .48f, 1f));
        Ring(new Vector2(90f, Screen.height - 108f), 25f,
            (float)progression.Experience / Mathf.Max(1, progression.ExperienceToNext),
            new Color(.70f, .40f, 1f));

        }

        // Radar of the small prototype arena ONLY: not the six-region Raven Atlas.
        float x = Screen.width - 178f;
        FillRect(new Rect(x, 18f, 160f, 160f), new Color(.04f, .05f, .09f, .9f));
        FillRect(new Rect(x + 8f, 26f, 144f, 144f), new Color(.13f, .17f, .22f, .9f));
        // Local radar follows the hero across the whole connected world.
        foreach (Transform cache in chamberRewards)
        {
            Vector3 d = cache.position - hero.position;
            if (Mathf.Abs(d.x) <= 17f && Mathf.Abs(d.z) <= 17f)
                FillRect(new Rect(x + 80f + d.x * 4f - 2f, 98f - d.z * 4f - 2f, 4f, 4f), Color.green);
        }
        foreach (WorldEnemy enemy in enemies)
        {
            Vector3 d = enemy.body.position - hero.position;
            if (Mathf.Abs(d.x) <= 17f && Mathf.Abs(d.z) <= 17f)
                FillRect(new Rect(x + 80f + d.x * 4f - 3f, 98f - d.z * 4f - 3f, 6f, 6f), Color.yellow);
        }
        Vector3 bossDelta = boss.position - hero.position;
        if (!won && Mathf.Abs(bossDelta.x) <= 17f && Mathf.Abs(bossDelta.z) <= 17f)
            FillRect(new Rect(x + 80f + bossDelta.x * 4f - 5f, 98f - bossDelta.z * 4f - 5f, 10f, 10f), Color.red);
        FillRect(new Rect(x + 75f, 93f, 10f, 10f), Color.cyan);
    }

    // Only the minimap and resource orbs appear during gameplay.
    void OnGUI()
    {
        if (hero != null && boss != null) DrawRpgHud();
    }
}
