using UnityEngine;
using System.Collections.Generic;
// Experimental primitive-only Moonfall combat slice, separate from production Godot.
public sealed class MoonfallPrototype : MonoBehaviour
{
    Transform hero, boss, warning;
    Camera view;
    float hp, bossHp = 100, attackCd, dodge, dodgeCooldown, castCd = 3, castTimer, zoom = 16;
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
        if (hp > 0)
        {
            Vector3 direction = new Vector3(Input.GetAxisRaw("Horizontal"),0,Input.GetAxisRaw("Vertical")).normalized;
            if (Input.GetKeyDown(KeyCode.LeftShift) && dodgeCooldown<=0)
            {
                dodge=.32f;
                dodgeCooldown=1.2f;
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
    void OnGUI()
    {
        GUI.Box(new Rect(10,10,410,155),"Moonfall Unity RPG Experiment");
        GUI.Label(new Rect(20,35,330,22),"HP: "+Mathf.CeilToInt(hp)+" | Mana: 100 | XP: 0");
        GUI.Label(new Rect(20,60,390,22),"Boss HP: "+Mathf.CeilToInt(bossHp)+" | Permanent wins: "+wins);
        GUI.Label(new Rect(20,85,390,22),"LV "+progression.Level+" XP "+progression.Experience+"/"+progression.ExperienceToNext+" | Hero/Tamer "+progression.HeroPoints+"/"+progression.TamerPoints);
        GUI.Label(new Rect(20,110,390,22),"WASD move | Click attack | Shift dodge | Wheel zoom");
        if (won) GUI.Label(new Rect(20,135,390,22),"Victory! Loot: " + bossLoot + " | Bag: " + bossInventory.Count + "/100");
        else if (hp<=0) GUI.Label(new Rect(20,110,330,22),"Defeated. Restart Play to retry.");
    }
}
