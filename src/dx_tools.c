#include "global.h"

#ifdef DX_DEBUG

#include "globaldata.h"
#include "constants/dungeon.h"
#include "constants/event_flag.h"
#include "constants/item.h"
#include "constants/main_menu.h"
#include "dx_tools.h"
#include "wigglytuff_shop3.h"
#include "event_flag.h"
#include "friend_area.h"
#include "input.h"
#include "items.h"
#include "memory.h"
#include "menu_input.h"
#include "pokemon.h"
#include "save.h"
#include "text_1.h"
#include "text_2.h"
#include "text_3.h"
#include "text_util.h"

#define INFO_WINDOW 0
#define INPUT_WINDOW 1
#define MAX_STEPS 2

typedef struct DxToolWork
{
    u32 tool;
    u32 step;
    s32 values[MAX_STEPS];
    s32 count;                      // entries in list[] (tools that pick from a list)
    s16 list[NUM_MONSTERS];
    u8 status[48];                  // result of the last apply, shown under the title
    bool8 shopOpen;                 // SELECT: Wigglytuff's Friend Area shop, as in town
    u32 exitTo;                     // set by an apply that leaves the tool (a scenario's warp)
    unkStructFor8013AA0 input;
    WindowTemplates windows;
} DxToolWork;

typedef struct DxToolDef
{
    const u8 *title;
    const u8 *prompts[MAX_STEPS];
    void (*begin)(void);
    void (*range)(u32 step, s32 *min, s32 *max, s32 *init);
    void (*describe)(u32 step, s32 value, u8 *line1, u8 *line2);
    void (*apply)(void);
} DxToolDef;

static EWRAM_DATA DxToolWork *sDxTool = NULL;

static const WindowTemplate sDxToolEmptyWindow = {
    0,
    3,
    0, 0,
    0, 0,
    0, 0,
    NULL
};

static const WindowTemplate sDxToolInfoWindow = {
    0,
    3,
    2, 2,
    26, 5,
    5, 0,
    NULL
};

static const WindowTemplate sDxToolInputWindow = {
    0,
    3,
    2, 9,
    8, 4,
    4, 0,
    NULL
};

static void DxToolSetStep(u32 step);
static void DxToolDraw(void);

// ---------------------------------------------------------------------------
// Give Item: item id, then quantity added to Kangaskhan storage (capped at 999)

ALIGNED(4) static const u8 sGiveTitle[] = _("Give Item (A: next, B: back)");
ALIGNED(4) static const u8 sGiveItemPrompt[] = _("Item");
ALIGNED(4) static const u8 sGiveQtyPrompt[] = _("Amount");
ALIGNED(4) static const u8 sFmtItem[] = _("#%d {COLOR CYAN}%s{RESET}");
ALIGNED(4) static const u8 sFmtInStorage[] = _("In storage: %d");
ALIGNED(4) static const u8 sFmtGave[] = _("Stored %d x %s");

static void GiveRange(u32 step, s32 *min, s32 *max, s32 *init)
{
    if (step == 0) {
        *min = 1;
        *max = NUMBER_OF_ITEM_IDS - 1;
        *init = (sDxTool->values[0] > 0) ? sDxTool->values[0] : 1;
    }
    else {
        *min = 1;
        *max = 999;
        *init = 1;
    }
}

static void GiveDescribe(u32 step, s32 value, u8 *line1, u8 *line2)
{
    s32 id = (step == 0) ? value : sDxTool->values[0];

    sprintfStatic((char *)line1, (const char *)sFmtItem, id, gItemParametersData[id].name);
    sprintfStatic((char *)line2, (const char *)sFmtInStorage, gTeamInventoryRef->teamStorage[id]);
}

static void GiveApply(void)
{
    s32 id = sDxTool->values[0];
    s32 total = gTeamInventoryRef->teamStorage[id] + sDxTool->values[1];

    if (total > 999)
        total = 999;
    gTeamInventoryRef->teamStorage[id] = total;
    sprintfStatic((char *)sDxTool->status, (const char *)sFmtGave, total, gItemParametersData[id].name);
}

// ---------------------------------------------------------------------------
// Shared: change a Pokemon's level, adding/removing the species' per-level stat gains

static u8 DxClampStat(s32 v)
{
    if (v < 1)
        return 1;
    if (v > 255)
        return 255;
    return v;
}

static void DxSetPokemonLevel(Pokemon *mon, s32 level)
{
    LevelData data;
    s32 hp = mon->pokeHP;
    s32 att0 = mon->offense.att[0], att1 = mon->offense.att[1];
    s32 def0 = mon->offense.def[0], def1 = mon->offense.def[1];
    s32 lv;

    for (lv = mon->level + 1; lv <= level; lv++) {
        GetLvlUpEntry(&data, mon->speciesNum, lv);
        hp += data.gainHP;
        att0 += data.gainAtt[0];
        att1 += data.gainAtt[1];
        def0 += data.gainDef[0];
        def1 += data.gainDef[1];
    }
    for (lv = mon->level; lv > level; lv--) {
        GetLvlUpEntry(&data, mon->speciesNum, lv);
        hp -= data.gainHP;
        att0 -= data.gainAtt[0];
        att1 -= data.gainAtt[1];
        def0 -= data.gainDef[0];
        def1 -= data.gainDef[1];
    }

    mon->pokeHP = (hp < 1) ? 1 : (hp > 999 ? 999 : hp);
    mon->offense.att[0] = DxClampStat(att0);
    mon->offense.att[1] = DxClampStat(att1);
    mon->offense.def[0] = DxClampStat(def0);
    mon->offense.def[1] = DxClampStat(def1);
    mon->level = level;
    GetLvlUpEntry(&data, mon->speciesNum, level);
    mon->currExp = data.expRequired;
}

// ---------------------------------------------------------------------------
// Recruit: species, then level; unlocks its Friend Area if needed

ALIGNED(4) static const u8 sRecruitTitle[] = _("Recruit (A: next, B: back)");
ALIGNED(4) static const u8 sRecruitSpeciesPrompt[] = _("Species");
ALIGNED(4) static const u8 sRecruitLevelPrompt[] = _("Level");
ALIGNED(4) static const u8 sFmtSpecies[] = _("#%d {COLOR CYAN}%s{RESET}");
ALIGNED(4) static const u8 sFmtRecruitLevel[] = _("Joins at Lv. %d");
ALIGNED(4) static const u8 sFmtRecruited[] = _("%s joined at Lv. %d! HP %d");
ALIGNED(4) static const u8 sFmtNoRoom[] = _("No room for %s");

static void RecruitRange(u32 step, s32 *min, s32 *max, s32 *init)
{
    if (step == 0) {
        *min = 1;
        *max = DX_FIRST_MEGA - 1; // Mega forms only exist while Empowered
        *init = (sDxTool->values[0] > 0) ? sDxTool->values[0] : 1;
    }
    else {
        *min = 1;
        *max = 100;
        *init = 5;
    }
}

static void RecruitDescribe(u32 step, s32 value, u8 *line1, u8 *line2)
{
    u8 name[20];
    s32 species = (step == 0) ? value : sDxTool->values[0];

    CopyMonsterNameToBuffer(name, species);
    sprintfStatic((char *)line1, (const char *)sFmtSpecies, species, name);
    sprintfStatic((char *)line2, (const char *)sFmtRecruitLevel, (step == 0) ? 5 : value);
}

static void RecruitApply(void)
{
    Pokemon pokemon;
    Pokemon *added;
    u8 name[20];
    DungeonLocation loc = {.id = DUNGEON_TINY_WOODS, .floor = 1};
    s32 species = sDxTool->values[0];

    UnlockFriendArea(GetFriendArea(species));
    CreateLevel1Pokemon(&pokemon, species, NULL, ITEM_NOTHING, &loc, NULL);
    DxSetPokemonLevel(&pokemon, sDxTool->values[1]);
    added = TryAddPokemonToRecruited(&pokemon);

    CopyMonsterNameToBuffer(name, species);
    if (added != NULL)
        sprintfStatic((char *)sDxTool->status, (const char *)sFmtRecruited, name, sDxTool->values[1], added->pokeHP);
    else
        sprintfStatic((char *)sDxTool->status, (const char *)sFmtNoRoom, name);
}

// ---------------------------------------------------------------------------
// Set Level: pick a recruited Pokemon (team first), then its new level

ALIGNED(4) static const u8 sLevelTitle[] = _("Set Level (A: next, B: back)");
ALIGNED(4) static const u8 sLevelWhoPrompt[] = _("Who");
ALIGNED(4) static const u8 sLevelLevelPrompt[] = _("Level");
ALIGNED(4) static const u8 sFmtWho[] = _("#%d {COLOR CYAN}%s{RESET} %s");
ALIGNED(4) static const u8 sFmtWhoLevel[] = _("Lv. %d -> %d   HP %d");
ALIGNED(4) static const u8 sOnTeam[] = _("(team)");
ALIGNED(4) static const u8 sEmpty[] = _("");
ALIGNED(4) static const u8 sNobody[] = _("Nobody recruited yet");
ALIGNED(4) static const u8 sFmtLeveled[] = _("%s is now Lv. %d");

static void LevelBegin(void)
{
    s32 i, pass;

    sDxTool->count = 0;
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < NUM_MONSTERS; i++) {
            Pokemon *mon = &gRecruitedPokemonRef->pokemon[i];

            if (!PokemonExists(mon) || (PokemonIsOnTeam(mon) != (pass == 0)))
                continue;
            sDxTool->list[sDxTool->count++] = i;
        }
    }
}

static Pokemon *LevelMon(s32 index)
{
    if (index < 0 || index >= sDxTool->count)
        return NULL;
    return &gRecruitedPokemonRef->pokemon[sDxTool->list[index]];
}

static void LevelRange(u32 step, s32 *min, s32 *max, s32 *init)
{
    if (step == 0) {
        *min = 0;
        *max = (sDxTool->count > 0) ? sDxTool->count - 1 : 0;
        *init = 0;
    }
    else {
        *min = 1;
        *max = 100;
        *init = LevelMon(sDxTool->values[0])->level;
    }
}

static void LevelDescribe(u32 step, s32 value, u8 *line1, u8 *line2)
{
    u8 name[POKEMON_NAME_LENGTH + 1];
    Pokemon *mon = LevelMon((step == 0) ? value : sDxTool->values[0]);

    if (mon == NULL) {
        sprintfStatic((char *)line1, (const char *)sNobody);
        line2[0] = '\0';
        return;
    }
    BoundedCopyStringtoBuffer(name, mon->name, POKEMON_NAME_LENGTH);
    sprintfStatic((char *)line1, (const char *)sFmtWho, (step == 0) ? value : sDxTool->values[0], name,
        PokemonIsOnTeam(mon) ? sOnTeam : sEmpty);
    sprintfStatic((char *)line2, (const char *)sFmtWhoLevel, mon->level, (step == 0) ? mon->level : value, mon->pokeHP);
}

static void LevelApply(void)
{
    u8 name[POKEMON_NAME_LENGTH + 1];
    Pokemon *mon = LevelMon(sDxTool->values[0]);

    if (mon == NULL)
        return;
    DxSetPokemonLevel(mon, sDxTool->values[1]);
    BoundedCopyStringtoBuffer(name, mon->name, POKEMON_NAME_LENGTH);
    sprintfStatic((char *)sDxTool->status, (const char *)sFmtLeveled, name, mon->level);
}

// ---------------------------------------------------------------------------
// Story: set SCENARIO_MAIN (chapter, step). Takes effect when you Continue.

ALIGNED(4) static const u8 sStoryTitle[] = _("Story jump (A: next, B: back)");
ALIGNED(4) static const u8 sStoryMainPrompt[] = _("Chapter");
ALIGNED(4) static const u8 sStorySubPrompt[] = _("Step");
ALIGNED(4) static const u8 sFmtStoryNow[] = _("Now: chapter %d, step %d");
ALIGNED(4) static const u8 sFmtStoryNew[] = _("New: chapter %d, step %d");
ALIGNED(4) static const u8 sFmtStorySet[] = _("Story set to %d-%d. Continue!");

static void StoryRange(u32 step, s32 *min, s32 *max, s32 *init)
{
    *min = 0;
    *max = (step == 0) ? 27 : 20;
    *init = GetScriptVarArrayValue(NULL, SCENARIO_MAIN, step);
}

static void StoryDescribe(u32 step, s32 value, u8 *line1, u8 *line2)
{
    sprintfStatic((char *)line1, (const char *)sFmtStoryNow, GetScriptVarArrayValue(NULL, SCENARIO_MAIN, 0),
        GetScriptVarArrayValue(NULL, SCENARIO_MAIN, 1));
    sprintfStatic((char *)line2, (const char *)sFmtStoryNew, (step == 0) ? value : sDxTool->values[0], (step == 0) ? 0 : value);
}

static void StoryApply(void)
{
    ScenarioCalc(SCENARIO_MAIN, sDxTool->values[0], sDxTool->values[1]);
    sprintfStatic((char *)sDxTool->status, (const char *)sFmtStorySet, sDxTool->values[0], sDxTool->values[1]);
}

// ---------------------------------------------------------------------------


#define MAIN_MENU_CONTINUE_GAME 2

// ---------------------------------------------------------------------------
// Scenarios: hands-on test setups (TESTING.md / the checklist). Each one sets up the team, toolbox
// and test switches in memory (nothing is saved unless you save), then warps. Use a scratch save:
// play.bat scenarios.

#include "dx_rules.h"
#include "dx_warp.h"
#include "exclusive_pokemon.h"
#include "constants/rare_quality.h"
#include "constants/friend_area.h"
#include "constants/move.h"
#include "constants/script_dungeon_id.h"

EWRAM_DATA u8 gDxScenario = 0;

ALIGNED(4) static const u8 sScenTitle[] = _("Scenarios (A: go, B: back, SELECT: shop)");
ALIGNED(4) static const u8 sScenPrompt[] = _("Scenario");
ALIGNED(4) static const u8 sFmtScen[] = _("S%d {COLOR CYAN}%s{RESET}");

typedef struct DxScenario
{
    const u8 *name;
    const u8 *where;
} DxScenario;

static const DxScenario sScenarios[] = {
    {NULL, NULL},
    {_("Party of 6"), _("6 on the team -> Tiny Woods 1F")},
    {_("Smart A, auto, linking"), _("linked moves -> Mt. Steel 2F")},
    {_("DX survival rules"), _("Oran Berries -> Mt. Thunder 1F")},
    {_("Apple recruiting"), _("6 Apples -> Mt. Steel 1F")},
    {_("Rare Qualities"), _("5 qualities -> Tiny Woods 1F")},
    {_("Gummis"), _("Rainbow + DX Gummis -> Tiny Woods 1F")},
    {_("Evolution Crystals"), _("Eevee alone -> Luminous Cave")},
    {_("New species"), _("4 newcomers -> Tiny Woods 1F")},
    {_("Mystery House"), _("house forced -> Mt. Steel 1F")},
    {_("Strong Foe + shiny"), _("Friend Bow -> Mt. Steel 1F")},
    {_("Mega Evolution"), _("Empower Seeds -> Tiny Woods 1F")},
    {_("Briefing screen"), _("cleared Mt. Steel -> world map")},
    {_("Rescue Camps"), _("SELECT shop; -> Thunderwave 1F")},
    {_("Post-game rematches"), _("after credits -> Mt. Steel 9F")},
    {_("Fairy type"), _("Gardevoir team -> Wyvern Hill 1F")},
};
#define NUM_SCENARIOS (ARRAY_COUNT(sScenarios) - 1)

static void ScenRange(u32 step, s32 *min, s32 *max, s32 *init)
{
    *min = 1;
    *max = NUM_SCENARIOS;
    *init = (sDxTool->values[0] >= 1) ? sDxTool->values[0] : 1;
}

static void ScenDescribe(u32 step, s32 value, u8 *line1, u8 *line2)
{
    sprintfStatic((char *)line1, (const char *)sFmtScen, value, sScenarios[value].name);
    strcpy((char *)line2, (const char *)sScenarios[value].where);
}

// --- building blocks ---

static void ScenToolbox(const u8 *items, s32 count)
{
    s32 i;
    Item item;

    for (i = 0; i < INVENTORY_SIZE; i++)
        gTeamInventoryRef->teamItems[i].flags = 0;
    for (i = 0; i < count; i++) {
        ItemIdToItem(&item, items[i], FALSE);
        AddItemToInventory(&item);
    }
}

// the team: the leader, the partner if asked, and new recruits of these species (Lv. level)
static Pokemon *ScenTeam(const s16 *species, s32 count, s32 level, bool8 keepPartner)
{
    s32 i;
    Pokemon *partner = GetPartnerMon2(), *last = NULL;
    DungeonLocation loc = {.id = DUNGEON_TINY_WOODS, .floor = 1};

    for (i = 0; i < NUM_MONSTERS; i++) {
        Pokemon *mon = &gRecruitedPokemonRef->pokemon[i];
        if (PokemonExists(mon) && !mon->isTeamLeader)
            mon->flags &= ~POKEMON_FLAG_ON_TEAM;
    }
    if (keepPartner && partner != NULL)
        partner->flags |= POKEMON_FLAG_ON_TEAM;
    for (i = 0; i < count; i++) {
        Pokemon pokemon, *added;
        UnlockFriendArea(GetFriendArea(species[i]));
        CreateLevel1Pokemon(&pokemon, species[i], NULL, ITEM_NOTHING, &loc, NULL);
        DxSetPokemonLevel(&pokemon, level);
        added = TryAddPokemonToRecruited(&pokemon);
        if (added != NULL) {
            added->flags |= POKEMON_FLAG_ON_TEAM;
            last = added;
        }
    }
    return last;
}

// the leader becomes another species (keeping its name, leader status and join record)
static void ScenLeader(s16 species, s32 level)
{
    Pokemon *leader = GetLeaderMon1();
    Pokemon pokemon;
    DungeonLocation loc;
    u16 flags;
    u8 name[POKEMON_NAME_LENGTH];

    if (leader == NULL)
        return;
    loc = leader->dungeonLocation;
    flags = leader->flags;
    memcpy(name, leader->name, POKEMON_NAME_LENGTH);
    CreateLevel1Pokemon(&pokemon, species, NULL, ITEM_NOTHING, &loc, NULL);
    DxSetPokemonLevel(&pokemon, level);
    *leader = pokemon;
    leader->flags = flags;
    leader->isTeamLeader = TRUE;
    leader->dungeonLocation = loc;
    memcpy(leader->name, name, POKEMON_NAME_LENGTH);
}

static void ScenUnlockCamps(void)
{
    s32 i;
    for (i = 1; i < FRIEND_AREA_COUNT; i++)
        UnlockFriendArea(i);
}

static void ScenGo(s16 scriptDungeon, s32 floor)
{
    DxRequestWarp(scriptDungeon, floor);
    sDxTool->exitTo = MAIN_MENU_CONTINUE_GAME;
}

static void ScenApply(void)
{
    s32 n = sDxTool->values[0];
    Pokemon *leader = GetLeaderMon1(), *partner = GetPartnerMon2(), *mon;
    s32 i;

    // a clean slate: no test switches from the last scenario; Poke to spend; camps (but S13) owned
    gDxScenario = n;
    gDxForceMysteryHouse = 0;
    gDxForceStrongFoe = FALSE;
    gDxForceShiny = FALSE;
    gDxForcePostgame = FALSE;
    gDxForceRecruit = FALSE;
    gDxWarpViaMap = FALSE;
    gTeamInventoryRef->teamMoney = 20000;
    if (n != 13)
        ScenUnlockCamps();
    if (leader != NULL)
        leader->heldItem.id = ITEM_NOTHING;

    switch (n) {
        case 1: { // Party of 6, three of them big
            static const s16 team[] = {MONSTER_SNORLAX, MONSTER_STEELIX, MONSTER_WAILORD, MONSTER_PIKACHU};
            static const u8 items[] = {ITEM_APPLE, ITEM_APPLE, ITEM_ORAN_BERRY};
            ScenTeam(team, ARRAY_COUNT(team), 20, TRUE);
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_TINY_WOODS, 1);
            break;
        }
        case 2: { // Smart A, auto-mode (L), linked moves (the leader's first two are linked)
            static const u8 items[] = {ITEM_APPLE, ITEM_ORAN_BERRY};
            ScenTeam(NULL, 0, 1, TRUE);
            if (leader != NULL)
                leader->moves[1].moveFlags |= MOVE_FLAG_SUBSEQUENT_IN_LINK_CHAIN;
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_MT_STEEL, 2);
            break;
        }
        case 3: { // regen, Oran Berry max HP, faster foes' double hits, faint penalty
            static const u8 items[] = {ITEM_ORAN_BERRY, ITEM_ORAN_BERRY, ITEM_ORAN_BERRY, ITEM_ORAN_BERRY, ITEM_ORAN_BERRY,
                                       ITEM_APPLE, ITEM_PECHA_BERRY};
            ScenTeam(NULL, 0, 1, TRUE);
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_MT_THUNDER, 1);
            break;
        }
        case 4: { // Apple recruiting
            static const u8 items[] = {ITEM_APPLE, ITEM_APPLE, ITEM_APPLE, ITEM_APPLE, ITEM_APPLE, ITEM_APPLE};
            ScenTeam(NULL, 0, 1, TRUE);
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_MT_STEEL, 1);
            break;
        }
        case 5: { // Rare Qualities: Forge a Path, Leap Ahead, Tight Formation, Shared Happiness, Leave Half
            static const s16 team[] = {MONSTER_PIKACHU, MONSTER_MACHOP, MONSTER_GEODUDE};
            static const u8 items[] = {ITEM_BIG_APPLE, ITEM_HUGE_APPLE, ITEM_PECHA_BERRY, ITEM_PECHA_BERRY, ITEM_HEAL_SEED,
                                       ITEM_HEAL_SEED, ITEM_ORAN_BERRY};
            static const u8 quals[] = {RQ_TIGHT_FORMATION, RQ_SHARED_HAPPINESS, RQ_LEAVE_HALF};
            ScenTeam(team, ARRAY_COUNT(team), 20, TRUE);
            if (leader != NULL)
                leader->dxQuality = RQ_FORGE_A_PATH;
            if (partner != NULL)
                partner->dxQuality = RQ_LEAP_AHEAD;
            for (i = 0, mon = &gRecruitedPokemonRef->pokemon[0]; i < NUM_MONSTERS; i++, mon++) {
                s32 k;
                for (k = 0; k < (s32)ARRAY_COUNT(team); k++) {
                    if (PokemonExists(mon) && (mon->flags & POKEMON_FLAG_ON_TEAM) && mon->speciesNum == team[k])
                        mon->dxQuality = quals[k];
                }
            }
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_TINY_WOODS, 1);
            break;
        }
        case 6: { // Gummis: Rainbow (a type Gummi) and DX Gummis
            static const u8 items[] = {ITEM_RAINBOW_GUMMI, ITEM_RAINBOW_GUMMI, ITEM_RAINBOW_GUMMI,
                                       ITEM_DX_GUMMI, ITEM_DX_GUMMI, ITEM_DX_GUMMI};
            ScenTeam(NULL, 0, 1, TRUE);
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_TINY_WOODS, 1);
            break;
        }
        case 7: { // Evolution Crystals: Eevee leads alone; a Pikachu waits in camp
            static const s16 camp[] = {MONSTER_PIKACHU};
            static const u8 items[] = {ITEM_EVO_CRYSTAL, ITEM_EVO_CRYSTAL, ITEM_EVO_CRYSTAL};
            ScenLeader(MONSTER_EEVEE, 20);
            mon = ScenTeam(camp, 1, 20, FALSE);
            if (mon != NULL)
                mon->flags &= ~POKEMON_FLAG_ON_TEAM; // in camp, not on the team
            ScenToolbox(items, ARRAY_COUNT(items));
            sDxTool->exitTo = MENU_DEBUG_UNKNOWN_1; // the Evolve tool: Luminous Cave
            break;
        }
        case 8: { // the DX newcomers (talk to them: portraits)
            static const s16 team[] = {MONSTER_SYLVEON, MONSTER_LUCARIO, MONSTER_TOGEKISS, MONSTER_MAMOSWINE};
            static const u8 items[] = {ITEM_APPLE, ITEM_ORAN_BERRY};
            ScenTeam(team, ARRAY_COUNT(team), 25, FALSE);
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_TINY_WOODS, 1);
            break;
        }
        case 9: { // a Mystery House on 1F of Riolu's home
            static const u8 items[] = {ITEM_APPLE, ITEM_APPLE, ITEM_APPLE};
            ScenTeam(NULL, 0, 1, TRUE);
            gDxForceMysteryHouse = 2;
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_MT_STEEL, 1);
            break;
        }
        case 10: { // a shiny Strong Foe in cleared Mt. Steel; the leader holds a Friend Bow
            static const u8 items[] = {ITEM_APPLE, ITEM_APPLE, ITEM_APPLE};
            ScenTeam(NULL, 0, 1, TRUE);
            DxMarkDungeonCleared(DUNGEON_MT_STEEL);
            gDxForceStrongFoe = TRUE;
            gDxForceShiny = TRUE;
            if (leader != NULL) {
                leader->heldItem.id = ITEM_FRIEND_BOW;
                leader->heldItem.quantity = 1;
            }
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_MT_STEEL, 1);
            break;
        }
        case 11: { // Mega Evolution: Charizard leads Gengar, Mewtwo and Lucario; Empower Seeds
            static const s16 team[] = {MONSTER_GENGAR, MONSTER_MEWTWO, MONSTER_LUCARIO};
            static const u8 items[] = {ITEM_EMPOWERMENT_SEED, ITEM_EMPOWERMENT_SEED, ITEM_EMPOWERMENT_SEED,
                                       ITEM_EMPOWERMENT_SEED};
            ScenLeader(MONSTER_CHARIZARD, 40);
            ScenTeam(team, ARRAY_COUNT(team), 40, FALSE);
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_TINY_WOODS, 1);
            break;
        }
        case 12: { // the briefing, on the world map (Mt. Steel cleared, so its Strong Foe is listed)
            ScenTeam(NULL, 0, 1, TRUE);
            DxMarkDungeonCleared(DUNGEON_MT_STEEL);
            gDxWarpViaMap = TRUE;
            ScenGo(SCRIPT_DUNGEON_MT_STEEL, 1);
            break;
        }
        case 13: { // Rescue Camps: the shop (SELECT, before this), and a camp-less recruit
            static const u8 items[] = {ITEM_APPLE, ITEM_APPLE, ITEM_APPLE, ITEM_APPLE, ITEM_APPLE};
            ScenTeam(NULL, 0, 1, TRUE);
            gDxForceRecruit = TRUE;
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_THUNDERWAVE_CAVE, 1);
            break;
        }
        case 14: { // after the credits: Skarmory waits on Mt. Steel 9F (also Sinister Woods 13F, Uproar Forest B4F)
            static const u8 items[] = {ITEM_APPLE, ITEM_APPLE, ITEM_APPLE, ITEM_ORAN_BERRY, ITEM_ORAN_BERRY};
            ScenTeam(NULL, 0, 1, TRUE);
            gDxForcePostgame = TRUE;
            // the story fights are done: Mt. Steel, Sinister Woods, Mt. Thunder Peak (flags 0-5)
            gExclusiveMonPtr->cutsceneFlags[0] |= 0x3F;
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_MT_STEEL, 9);
            break;
        }
        case 15: { // Fairy type: Gardevoir leads Clefairy against Wyvern Hill's Dratini and Bagon
            static const s16 team[] = {MONSTER_CLEFAIRY};
            static const u8 items[] = {ITEM_ORAN_BERRY, ITEM_ORAN_BERRY, ITEM_APPLE};
            ScenLeader(MONSTER_GARDEVOIR, 40);
            ScenTeam(team, ARRAY_COUNT(team), 40, FALSE);
            ScenToolbox(items, ARRAY_COUNT(items));
            ScenGo(SCRIPT_DUNGEON_WYVERN_HILL, 1);
            break;
        }
    }
}

static const DxToolDef sDxTools[DX_TOOL_COUNT] = {
    [DX_TOOL_GIVE_ITEM] = {sGiveTitle, {sGiveItemPrompt, sGiveQtyPrompt}, NULL, GiveRange, GiveDescribe, GiveApply},
    [DX_TOOL_RECRUIT] = {sRecruitTitle, {sRecruitSpeciesPrompt, sRecruitLevelPrompt}, NULL, RecruitRange, RecruitDescribe, RecruitApply},
    [DX_TOOL_SET_LEVEL] = {sLevelTitle, {sLevelWhoPrompt, sLevelLevelPrompt}, LevelBegin, LevelRange, LevelDescribe, LevelApply},
    [DX_TOOL_STORY] = {sStoryTitle, {sStoryMainPrompt, sStorySubPrompt}, NULL, StoryRange, StoryDescribe, StoryApply},
    [DX_TOOL_SCENARIO] = {sScenTitle, {sScenPrompt, NULL}, NULL, ScenRange, ScenDescribe, ScenApply},
};

void CreateDxToolMenu(u32 tool)
{
    if (sDxTool == NULL) {
        sDxTool = MemoryAlloc(sizeof(DxToolWork), MEMALLOC_GROUP_8);
        MemoryFill8(sDxTool, 0, sizeof(DxToolWork));
    }
    sDxTool->tool = tool;
    if (sDxTools[tool].begin != NULL)
        sDxTools[tool].begin();
    DxToolSetStep(0);
}

void DeleteDxToolMenu(void)
{
    ResetUnusedInputStruct();
    ShowWindows(NULL, TRUE, TRUE);

    if (sDxTool != NULL) {
        MemoryFree(sDxTool);
        sDxTool = NULL;
    }
}

u32 UpdateDxToolMenu(void)
{
    const DxToolDef *def = &sDxTools[sDxTool->tool];

    // Every tool edits the loaded save, so there has to be one
    if (sub_8011C34() == -1)
        return MENU_MAIN_SCREEN;

    // SELECT opens Wigglytuff's shop (the town's, run the same way its textbox runs it)
    if (sDxTool->shopOpen) {
        if (sub_8021C5C() >= 2) {
            CleanWigglytuffShop();
            sDxTool->shopOpen = FALSE;
            DxToolSetStep(sDxTool->step);
        }
        return MENU_NO_SCREEN_CHANGE;
    }
    if ((gRealInputs.pressed & SELECT_BUTTON) && CreateWigglytuffShop(0)) {
        sDxTool->shopOpen = TRUE;
        return MENU_NO_SCREEN_CHANGE;
    }

    switch (sub_8013BBC(&sDxTool->input)) {
        case 1:
            DxToolDraw();
            break;
        case 2:
            if (sDxTool->step == 0)
                return MENU_DEBUG;
            DxToolSetStep(sDxTool->step - 1);
            break;
        case 3:
            sDxTool->values[sDxTool->step] = sDxTool->input.unk0;
            if (sDxTool->step + 1 < MAX_STEPS && def->prompts[sDxTool->step + 1] != NULL) {
                if (def == &sDxTools[DX_TOOL_SET_LEVEL] && sDxTool->count == 0)
                    break;
                DxToolSetStep(sDxTool->step + 1);
            }
            else {
                def->apply();
                if (sDxTool->exitTo != 0)
                    return sDxTool->exitTo;
                if (def->begin != NULL)
                    def->begin();
                DxToolSetStep(0);
            }
            break;
    }

    return MENU_NO_SCREEN_CHANGE;
}

static void DxToolSetStep(u32 step)
{
    s32 i, min, max, init;
    unkStructFor8013AA0 *input = &sDxTool->input;

    sDxTool->step = step;

    for (i = 0; i < 4; i++)
        sDxTool->windows.id[i] = sDxToolEmptyWindow;
    sDxTool->windows.id[INFO_WINDOW] = sDxToolInfoWindow;
    sDxTool->windows.id[INPUT_WINDOW] = sDxToolInputWindow;
    ResetUnusedInputStruct();
    ShowWindows(&sDxTool->windows, TRUE, TRUE);

    sDxTools[sDxTool->tool].range(step, &min, &max, &init);
    if (init < min)
        init = min;
    if (init > max)
        init = max;

    // Same digit-entry widget as Warp (up/down: digit, left/right: column)
    input->unk4 = init;
    input->unk8 = min;
    input->unkC = max;
    input->unk10 = (max >= 100) ? 3 : 2;
    input->unk14 = INPUT_WINDOW;
    input->unk18 = &sDxTool->windows.id[INPUT_WINDOW];
    input->unk1C = 60;
    input->unk20 = 18;
    sub_8013AA0(input);

    DxToolDraw();
}

static void DxToolDraw(void)
{
    const DxToolDef *def = &sDxTools[sDxTool->tool];
    u8 line1[96], line2[96];

    def->describe(sDxTool->step, sDxTool->input.unk0, line1, line2);

    CallPrepareTextbox_8008C54(INFO_WINDOW);
    sub_80073B8(INFO_WINDOW);
    PrintStringOnWindow(4, 0, sDxTool->status[0] != '\0' ? sDxTool->status : def->title, INFO_WINDOW, 0);
    PrintStringOnWindow(4, 14, line1, INFO_WINDOW, 0);
    PrintStringOnWindow(4, 26, line2, INFO_WINDOW, 0);
    sub_80073E0(INFO_WINDOW);

    CallPrepareTextbox_8008C54(INPUT_WINDOW);
    sub_80073B8(INPUT_WINDOW);
    PrintStringOnWindow(4, 0, def->prompts[sDxTool->step], INPUT_WINDOW, 0);
    sub_8013C68(&sDxTool->input);
    sub_80073E0(INPUT_WINDOW);
}


// Struct offsets for the headless tests (tools/dxtest): `@gDxOff...` in a test reads these,
// so tests keep working when DX changes grow structs.
#include "structs/str_dungeon.h"
#include "structs/dungeon_entity.h"
const u32 gDxOffDungeonLocation = offsetof(Dungeon, unk644.dungeonLocation);
const u32 gDxOffDungeonFloor = offsetof(Dungeon, unk644.dungeonLocation.floor);
const u32 gDxOffFloorCount = offsetof(Dungeon, unk1CEC8);
const u32 gDxOffTeamPokemon = offsetof(Dungeon, teamPokemon);
const u32 gDxOffEntityPos = offsetof(Entity, pos);
const u32 gDxOffEntityInfo = offsetof(Entity, axObj.info.monster);
const u32 gDxOffInfoId = offsetof(EntityInfo, id);
const u32 gDxOffInfoTypes = offsetof(EntityInfo, types);
const u32 gDxOffInfoMoves = offsetof(EntityInfo, moves);
const u32 gDxOffInfoHP = offsetof(EntityInfo, HP);
const u32 gDxOffInfoMaxHP = offsetof(EntityInfo, maxHPStat);
const u32 gDxOffWindTurns = offsetof(Dungeon, unk644.windTurns);
const u32 gDxOffWildPokemon = offsetof(Dungeon, wildPokemon);
const u32 gDxOffInfoStages = offsetof(EntityInfo, offensiveStages);
const u32 gDxOffInfoApparentId = offsetof(EntityInfo, apparentID);
const u32 gDxOffInfoBelly = offsetof(EntityInfo, belly);
const u32 gDxOffInfoLevel = offsetof(EntityInfo, level);
const u32 gDxOffInfoExp = offsetof(EntityInfo, exp);
const u32 gDxOffCanRecruit = offsetof(Dungeon, unk644.canRecruit);
const u32 gDxOffInfoStrongFoe = offsetof(EntityInfo, dxStrongFoe);
const u32 gDxOffInfoShiny = offsetof(EntityInfo, dxShiny);
#include "exclusive_pokemon.h"
const u32 gDxOffCutsceneFlags = offsetof(ExclusivePokemonData, cutsceneFlags);
const u32 gDxOffFloorEnd = offsetof(Dungeon, unk2);
const u32 gDxOffFloorEvent = offsetof(Dungeon, unk4);
const u32 gDxOffInfoBehavior = offsetof(EntityInfo, monsterBehavior);
const u32 gDxOffInfoBurn = offsetof(EntityInfo, burnClassStatus);
const u32 gDxOffDMon2Quality = offsetof(RecruitedMon, dungeonTeam[2].dxQuality);
const u32 gDxOffInfoHeldItem = offsetof(EntityInfo, heldItem);
const u32 gDxOffInfoDirection = offsetof(EntityInfo, action.direction);
const u32 gDxOffMonLevel = offsetof(Pokemon, level);
const u32 gDxOffMonExp = offsetof(Pokemon, currExp);
const u32 gDxOffInfoSleep = offsetof(EntityInfo, sleepClassStatus);
const u32 gDxOffInfoFrozen = offsetof(EntityInfo, frozenClassStatus);
#endif // DX_DEBUG
