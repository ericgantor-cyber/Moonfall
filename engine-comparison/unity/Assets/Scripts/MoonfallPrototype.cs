using UnityEngine;
using System.Collections.Generic;
// Experimental primitive-only Moonfall combat slice, separate from production Godot.
public sealed class MoonfallPrototype : MonoBehaviour
{
    Transform hero, boss, warning;
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
    void Start()
    {
        ornateOrbs = Resources.Load<Texture2D>("MoonfallOrbs");
        wins = Mathf.Clamp(PlayerPrefs.GetInt("moonfall_experiment_wins", 0), 0, 100);
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
        Make("Floor", PrimitiveType.Cube, new Vector3(0,-.3f,0), new Vector3(34,.5f,34), Color.gray);
        hero = Make("Hero placeholder", PrimitiveType.Capsule, new Vector3(0,1,-5), Vector3.one, Color.cyan);
        boss = Make("Boss placeholder", PrimitiveType.Capsule, new Vector3(0,1,6), new Vector3(2,2,2), Color.red);
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
            if (!won && Input.GetKeyDown(KeyCode.Q) && boltCooldown <= 0f && mana >= 30f
                && Vector3.Distance(hero.position, boss.position) <= 10f)
            {
                mana -= 30f;
                boltCooldown = 1.1f;
                bossHp = Mathf.Max(0f, bossHp - (25f + progression.Level * 2f));
                if (bossHp <= 0f) Victory();
            }
            if (Input.GetKeyDown(KeyCode.E) && healCooldown <= 0f && mana >= 40f
                && hp < 100f + 5f * wins)
            {
                mana -= 40f;
                healCooldown = 5f;
                hp = Mathf.Min(100f + 5f * wins, hp + 30f + progression.Level);
            }
            hero.position += direction * (dodge>0 ? 12 : 6) * dt;
            hero.position = new Vector3(Mathf.Clamp(hero.position.x,-16,16),1,Mathf.Clamp(hero.position.z,-16,16));
            if (!won && Input.GetMouseButtonDown(0) && attackCd<=0)
            {
                attackCd=.35f;
                if (Vector3.Distance(hero.position,boss.position)<4)
                {
                    bossHp=Mathf.Max(0,bossHp-15);
                    if (bossHp<=0) Victory();
                }
            }
        }
        zoom=Mathf.Clamp(zoom-Input.mouseScrollDelta.y*1.25f,9,25);
        view.transform.position=Vector3.Lerp(view.transform.position,hero.position+new Vector3(0,zoom,-zoom),Mathf.Clamp01(dt*8));
        view.transform.LookAt(hero.position);
        if (won || hp<=0) return;
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
        warning.gameObject.SetActive(false);
        boss.gameObject.SetActive(false);
        int previousWins = wins;
        wins = Mathf.Min(100, wins + 1);
        PlayerPrefs.SetInt("moonfall_experiment_wins", wins);
        PlayerPrefs.Save();
        hp += 5 * (wins - previousWins);
        int levelsGained = progression.AwardDefeat(0, true, true, Time.time);
        hp = Mathf.Min(100 + 5 * wins, hp + 20 * levelsGained);
        PlayerPrefs.SetInt("moonfall_experiment_level", progression.Level);
        PlayerPrefs.SetInt("moonfall_experiment_xp", progression.Experience);
        PlayerPrefs.SetInt("moonfall_experiment_hero_points", progression.HeroPoints);
        PlayerPrefs.SetInt("moonfall_experiment_tamer_points", progression.TamerPoints);
        PlayerPrefs.Save();
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
        float heroX = x + 80f + Mathf.Clamp(hero.position.x / 17f, -1f, 1f) * 68f;
        float heroY = 98f - Mathf.Clamp(hero.position.z / 17f, -1f, 1f) * 68f;
        float bossX = x + 80f + Mathf.Clamp(boss.position.x / 17f, -1f, 1f) * 68f;
        float bossY = 98f - Mathf.Clamp(boss.position.z / 17f, -1f, 1f) * 68f;
        FillRect(new Rect(heroX - 5f, heroY - 5f, 10f, 10f), Color.cyan);
        if (!won) FillRect(new Rect(bossX - 5f, bossY - 5f, 10f, 10f), Color.red);
    }

    // Only the minimap and three resource orbs appear during gameplay.
    void OnGUI()
    {
        if (hero != null && boss != null) DrawRpgHud();
    }
}
