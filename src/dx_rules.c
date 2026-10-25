#include "global.h"

#ifdef DX_RULES

#include "dx_rules.h"
#include "constants/rare_quality.h"
#include "friend_area.h"
#include "dungeon_cutscene.h"
#include "event_flag.h"
#include "dx_tuning.h"
#include "constants/dungeon.h"
#include "constants/dungeon_action.h"
#include "dungeon_util.h"
#include "dungeon_floor_spawns.h"
#include "pokemon_3.h"
#include "moves.h"
#include "dx_rare_quality.h"
#include "move_orb_effects_1.h"
#include "dungeon_message.h"
#include "dungeon_logic.h"
#include "dungeon_misc.h"
#include "text_util.h"
#include "structs/menu.h"
#include "dungeon_items.h"
#include "items.h"
#include "pokemon.h"
#include "dungeon_random.h"
#include "structs/str_dungeon.h"

// mainline base Speed per species id (Red has no Speed stat), from tools/dxtest/gen_dx_species.py
#include "data/dx_base_speed.h"

bool8 DxActionPausesRegen(u16 action)
{
    switch (action) {
        case ACTION_USE_MOVE_PLAYER:
        case ACTION_USE_MOVE_AI:
        case ACTION_STRUGGLE:
        case ACTION_REGULAR_ATTACK:
        case ACTION_EAT_BERRY_SEED_DRINK:
        case ACTION_EAT_AI:
        case ACTION_THROW_ITEM_PLAYER:
        case ACTION_THROW_ITEM_AI:
        case ACTION_THROW_STRAIGHT_PLAYER:
        case ACTION_THROW_ARC_PLAYER:
        case ACTION_USE_ORB:
        case ACTION_USE_ITEM:
        case ACTION_USE_TM:
            return TRUE;
    }
    return FALSE;
}

static s32 EffectiveSpeed(Entity *entity)
{
    EntityInfo *info = GetEntInfo(entity);
    s32 speed = (info->id > 0 && info->id < MONSTER_MAX) ? sDxBaseSpeed[info->id] : 0;

    // speed stage 1 is normal; boosts multiply, a slowed Pokemon counts half
    if (info->speedStage == 0)
        return speed / 2;
    return speed * info->speedStage;
}

s32 DxDoubleMoveChance(Entity *attacker, Entity *target)
{
    s32 lead = EffectiveSpeed(attacker) - EffectiveSpeed(target);
    s32 chance;

    if (lead < DX_DOUBLE_MOVE_SPEED_GAP)
        return 0;
    chance = DX_DOUBLE_MOVE_BASE_CHANCE + (lead - DX_DOUBLE_MOVE_SPEED_GAP) / DX_DOUBLE_MOVE_SPEED_PER_PERCENT;
    return chance > DX_DOUBLE_MOVE_MAX_CHANCE ? DX_DOUBLE_MOVE_MAX_CHANCE : chance;
}

// --- Move Growth ---
// Rescue Team DX: moves grow stronger with use (see dx_tuning.h)
void DxTryMoveGrowth(EntityInfo *user, Move *move)
{
    s32 chance;

    if (user->isNotTeamMember || GetMoveBasePower(move) == 0 || move->ginseng >= GetMoveMaxUpgradeLevel(move))
        return;
    chance = DX_MOVE_GROWTH_BASE_CHANCE - DX_MOVE_GROWTH_FALLOFF * move->ginseng;
    if (chance < DX_MOVE_GROWTH_MIN_CHANCE)
        chance = DX_MOVE_GROWTH_MIN_CHANCE;
    if (DxTeamHasQuality(RQ_MOVING_UP))
        chance *= 2;
    if (DungeonRandInt(100) < chance)
        move->ginseng++;
}

// --- Empowerment Seed ---
static const u8 sDxEmpowered[] = _("{POKEMON_0} became Empowered!");
static const u8 sDxMegaEvolved[] = _("{POKEMON_0} Mega Evolved!");
static const u8 sDxPrimal[] = _("{POKEMON_0} underwent Primal Reversion!");

#include "data/dx_megas.h"

static const u8 sDxWhichMega[] = _("Which Mega Evolution?");

// the form an Empowered Pokemon takes: Charizard and Mewtwo have two, and a team member picks
static const struct DxMegaForm *DxChooseMega(Entity *target)
{
    const struct DxMegaForm *forms[2];
    MenuItem items[3];
    s32 i, count = 0, choice;
    EntityInfo *info = GetEntInfo(target);

    for (i = 0; i < ARRAY_COUNT(sDxMegaForms) && count < 2; i++) {
        if (sDxMegaForms[i].base == info->id)
            forms[count++] = &sDxMegaForms[i];
    }
    if (count == 0)
        return NULL;
    if (count == 1 || info->isNotTeamMember)
        return forms[0];
    for (i = 0; i < count; i++) {
        items[i].text = GetMonSpecies(forms[i]->mega);
        items[i].menuAction = i + 1;
    }
    items[count].text = NULL;
    items[count].menuAction = 0;
    choice = DisplayDungeonMenuMessage(NULL, sDxWhichMega, items, 0);
    return (choice >= 1 && choice <= count) ? forms[choice - 1] : forms[0];
}

// a Mega form's base species (any other id is returned as is)
s32 DxMegaBase(s32 species)
{
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sDxMegaForms); i++) {
        if (sDxMegaForms[i].mega == species)
            return sDxMegaForms[i].base;
    }
    return species;
}

// The Empowered Pokemon takes on its Mega/Primal form for the floor: sprite, types and abilities
// come from the form (its species id stays the base's, so name, data and saves are unchanged),
// and the form's stat edge is extra stat stages.
static void DxMegaEvolve(Entity *user, Entity *target, const struct DxMegaForm *form)
{
    EntityInfo *info = GetEntInfo(target);
    StatIndex physical = {STAT_INDEX_PHYSICAL}, special = {STAT_INDEX_SPECIAL};
    s32 i;

    LoadPokemonSprite(form->mega, FALSE);
    info->apparentID = form->mega;
    target->axObj.spriteFile = GetSpriteData(form->mega);
    for (i = 0; i < 2; i++) {
        info->types[i] = GetPokemonType(form->mega, i);
        info->abilities[i] = GetPokemonAbility(form->mega, i);
    }
    SetMessageArgument_2(gFormatBuffer_Monsters[0], info, 0);
    TryDisplayDungeonLoggableMessage3_Async(user, target, form->primal ? sDxPrimal : sDxMegaEvolved);
    if (form->stages[0]) BoostOffensiveStat(user, target, physical, form->stages[0]);
    if (form->stages[1]) BoostOffensiveStat(user, target, special, form->stages[1]);
    if (form->stages[2]) BoostDefensiveStat(user, target, physical, form->stages[2]);
    if (form->stages[3]) BoostDefensiveStat(user, target, special, form->stages[3]);
    if (form->faster)
        BoostSpeed(user, target, 0, TRUE);
    gDungeon->unkC = 1; // abilities changed: re-check weather abilities (Drizzle, Drought...)
}

// Rescue Team DX: an Empowered Pokemon's stats all rise (Mega forms come on top of this)
void DxEmpower(Entity *user, Entity *target)
{
    StatIndex physical = {STAT_INDEX_PHYSICAL}, special = {STAT_INDEX_SPECIAL};
    const struct DxMegaForm *form;

    if (!EntityIsValid(target))
        return;
    SetMessageArgument_2(gFormatBuffer_Monsters[0], GetEntInfo(target), 0);
    TryDisplayDungeonLoggableMessage3_Async(user, target, sDxEmpowered);
    BoostOffensiveStat(user, target, physical, DX_EMPOWER_STAGES);
    BoostOffensiveStat(user, target, special, DX_EMPOWER_STAGES);
    BoostDefensiveStat(user, target, physical, DX_EMPOWER_STAGES);
    BoostDefensiveStat(user, target, special, DX_EMPOWER_STAGES);
    BoostSpeed(user, target, 0, TRUE);

    form = DxChooseMega(target);
    if (form != NULL && GetEntInfo(target)->apparentID != form->mega)
        DxMegaEvolve(user, target, form);
}

// --- Strong Foes ---
#include "data/dx_strong_foes.h"

// dungeons cleared at least once (saved in the DX save block): DX's Strong Foes need a first clear
EWRAM_DATA u8 gDxClearedDungeons[16] = {0};
// this floor's Strong Foe, waiting to spawn (0 = none)
EWRAM_DATA s16 gDxStrongFoeSpecies = 0;
// tests: a Strong Foe on every floor, cleared or not
EWRAM_DATA bool8 gDxForceStrongFoe = FALSE;

void DxMarkDungeonCleared(u8 dungeon)
{
    if (dungeon < 128)
        gDxClearedDungeons[dungeon / 8] |= 1 << (dungeon % 8);
}

bool8 DxDungeonCleared(u8 dungeon)
{
    return dungeon < 128 && (gDxClearedDungeons[dungeon / 8] & (1 << (dungeon % 8)));
}

// for the briefing: a cleared dungeon with a Strong Foe
// tests: act as if the credits have rolled (and boss rooms are in their post-story state)
EWRAM_DATA bool8 gDxForcePostgame = FALSE;

bool8 DxPostgame(void)
{
    return gDxForcePostgame || CheckQuest(QUEST_REACHED_POSTGAME);
}

// DX: after the credits, a story boss waits in its room again for a rematch, until it has joined
bool8 DxBossRematch(s16 species)
{
    return DxPostgame() && !HasRecruitedMon(species);
}

// DX recruiting after the credits: a story boss can be recruited like anyone (with a fallback rate
// for the ones Red never lets join), and a legendary beaten in its boss room always offers to join
u8 DxPostgameBossKind(u8 monsterBehavior)
{
    if (!DxPostgame())
        return DX_BOSS_NONE;
    switch (monsterBehavior) {
        case BEHAVIOR_SKARMORY:
        case BEHAVIOR_GENGAR:
        case BEHAVIOR_EKANS:
        case BEHAVIOR_MEDICHAM:
        case BEHAVIOR_MANKEY:
            return DX_BOSS_STORY;
        case BEHAVIOR_ZAPDOS:
        case BEHAVIOR_MOLTRES:
        case BEHAVIOR_ARTICUNO:
        case BEHAVIOR_GROUDON_1:
        case BEHAVIOR_GROUDON_2:
        case BEHAVIOR_RAYQUAZA:
        case BEHAVIOR_MEWTWO:
        case BEHAVIOR_ENTEI:
        case BEHAVIOR_RAIKOU:
        case BEHAVIOR_SUICUNE:
        case BEHAVIOR_HO_OH:
        case BEHAVIOR_LATIOS:
        case BEHAVIOR_REGIROCK:
        case BEHAVIOR_REGICE:
        case BEHAVIOR_REGISTEEL:
        case BEHAVIOR_JIRACHI:
        case BEHAVIOR_LUGIA:
        case BEHAVIOR_KYOGRE:
        case BEHAVIOR_DEOXYS_NORMAL:
        case BEHAVIOR_CELEBI:
            return DX_BOSS_LEGENDARY;
    }
    return DX_BOSS_NONE;
}

// a post-game story boss Red never lets join (Gengar: -999) can join at a fallback rate
s32 DxBossRecruitRate(s32 recruitRate, u8 monsterBehavior)
{
    if (DxPostgameBossKind(monsterBehavior) == DX_BOSS_STORY && recruitRate <= 0)
        return DX_BOSS_FALLBACK_RECRUIT_RATE;
    return recruitRate;
}

bool8 DxDungeonHasStrongFoe(u8 dungeon)
{
    s32 i;

    if (!DxDungeonCleared(dungeon))
        return FALSE;
    for (i = 0; i < ARRAY_COUNT(sDxStrongFoes); i++) {
        if (sDxStrongFoes[i].dungeon == dungeon)
            return TRUE;
    }
    return FALSE;
}

// as a floor is generated: does a Strong Foe show up here?
void DxRollStrongFoe(void)
{
    s32 i;
    u8 dungeon = gDungeon->unk644.dungeonLocation.id;

    gDxStrongFoeSpecies = 0;
    if (!gDxForceStrongFoe && (!DxDungeonCleared(dungeon) || DungeonRandInt(100) >= DX_STRONG_FOE_CHANCE))
        return;
    for (i = 0; i < ARRAY_COUNT(sDxStrongFoes); i++) {
        if (sDxStrongFoes[i].dungeon == dungeon) {
            gDxStrongFoeSpecies = sDxStrongFoes[i].species;
            return;
        }
    }
}

#include "data/dx_shiny.h"

// tests: Strong Foes of shiny species are always shiny
EWRAM_DATA bool8 gDxForceShiny = FALSE;

// the shared palette a shiny is drawn with (-1: no shiny form)
// a recruited Pokemon's sprite palette outside dungeons (towns, camps, the maps): its shiny one if it is shiny
u8 DxPokemonPalette(Pokemon *mon)
{
    u8 pal = GetPokemonOverworldPalette(mon->speciesNum, FALSE);

    if ((mon->dxFlags & DX_FLAG_SHINY) && DxShinyPalette(mon->speciesNum) >= 0)
        pal = DxShinyPalette(mon->speciesNum);
    return pal;
}

s32 DxShinyPalette(s32 species)
{
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sDxShinies); i++) {
        if (sDxShinies[i].species == species)
            return sDxShinies[i].palette;
    }
    return -1;
}

// a freshly spawned Strong Foe: tougher, marked, worth more, and sometimes shiny
void DxMakeStrongFoe(Entity *entity)
{
    EntityInfo *info = GetEntInfo(entity);

    info->dxStrongFoe = TRUE;
    if (DxShinyPalette(info->id) >= 0 && (gDxForceShiny || DungeonRandInt(100) < DX_SHINY_CHANCE))
        info->dxShiny = TRUE;
    info->offensiveStages[0] += DX_STRONG_FOE_STAGES;
    info->offensiveStages[1] += DX_STRONG_FOE_STAGES;
    info->defensiveStages[0] += DX_STRONG_FOE_STAGES;
    info->defensiveStages[1] += DX_STRONG_FOE_STAGES;
}

// DX: a defeated Strong Foe leaves a chest of rare items (here: a DX Gummi on its tile)
void DxStrongFoeReward(Entity *foe)
{
    Item item;

    CreateItemWithStickyChance(&item, ITEM_DX_GUMMI, FORCE_STICKY_NEVER);
    SpawnItem(&foe->pos, &item, TRUE);
}

// --- Mystery Houses ---
#include "data/dx_mystery_houses.h"

// the newcomer waiting in this floor's Mystery House (0 = none)
EWRAM_DATA s16 gDxMysteryHouseSpecies = 0;
// this floor rolled a Mystery House (for its message, even after the newcomer has spawned)
EWRAM_DATA bool8 gDxMysteryHouseFloor = FALSE;
// tests: force the roll, and record which newcomer actually spawned
EWRAM_DATA bool8 gDxForceMysteryHouse = FALSE;
EWRAM_DATA s16 gDxMysteryHouseSpawned = 0;
// tests: the next recruit rolls always succeed
EWRAM_DATA bool8 gDxForceRecruit = FALSE;

// Called as a floor is generated: TRUE forces a Monster House, whose first occupant is the newcomer
bool8 DxRollMysteryHouse(void)
{
    s32 i, count = 0;
    s16 choices[ARRAY_COUNT(sDxMysteryHomes) + 1];
    u8 dungeon = gDungeon->unk644.dungeonLocation.id;

    gDxMysteryHouseSpecies = 0;
    gDxMysteryHouseFloor = FALSE;
    for (i = 0; i < ARRAY_COUNT(sDxMysteryHomes); i++) {
        if (sDxMysteryHomes[i].dungeon == dungeon)
            choices[count++] = sDxMysteryHomes[i].species;
    }
    // DX: Purity Forest's Mystery Houses can also hold a shiny Celebi
    if (dungeon == DUNGEON_PURITY_FOREST)
        choices[count++] = MONSTER_CELEBI;
    if (count == 0 || (!gDxForceMysteryHouse && DungeonRandInt(100) >= DX_MYSTERY_HOUSE_CHANCE))
        return FALSE;
    gDxMysteryHouseSpecies = choices[DungeonRandInt(count)];
    if (gDxForceMysteryHouse == 3) // tests: the last choice (Purity Forest: Celebi)
        gDxMysteryHouseSpecies = choices[count - 1];
    gDxMysteryHouseFloor = TRUE;
    return TRUE;
}

// a Mystery House occupant has just spawned
void DxMysteryHouseSpawned(Entity *mon)
{
    gDxMysteryHouseSpawned = gDxMysteryHouseSpecies;
    gDxMysteryHouseSpecies = 0;
    if (GetEntInfo(mon)->id == MONSTER_CELEBI)
        GetEntInfo(mon)->dxShiny = TRUE; // Purity Forest's is always shiny
}

// the strongest level on this floor's spawn list
s32 DxMysteryHouseLevel(void)
{
    s32 i, level = 1;

    for (i = 0; i < gDungeon->currFloorMonsterSpawnsCount; i++) {
        s32 l = ExtractLevel(&gDungeon->monsterSpawns[i]);
        if (gDungeon->monsterSpawns[i].randNum[0] != 0 && l > level)
            level = l;
    }
    return level;
}

#endif // DX_RULES

// --- recruits without a camp ---
// turns passed this adventure (UpdateWindTurns), and when each team slot's visitor joined
EWRAM_DATA u16 gDxAdventureTurns = 0;
static EWRAM_DATA u16 sDxVisitorJoinTurn[MAX_TEAM_MEMBERS] = {0};

static const u8 sDxVisitorLeaves[] = _("{POKEMON_0} had a fun adventure! With no\ncamp to stay in, it went home and left\n{COLOR YELLOW}%d{RESET} Poké as thanks.");

// Rescue Team DX: you can recruit a Pokemon whose camp you don't own; it joins for the adventure
void DxMarkVisitor(s32 teamSlot, DungeonMon *dmon)
{
    if (GetFriendAreaStatus(GetFriendArea(dmon->speciesNum)))
        return;
    dmon->dxFlags |= DX_FLAG_VISITOR;
    sDxVisitorJoinTurn[teamSlot] = gDxAdventureTurns;
}

// the adventure is over: this camp's visitors go home, each leaving Poke for the turns spent together
void DxPayVisitors(u8 friendArea)
{
    s32 i;
    u8 text[160];

    for (i = 0; i < MAX_TEAM_MEMBERS; i++) {
        DungeonMon *dmon = &gRecruitedPokemonRef->dungeonTeam[i];
        s32 poke;

        if (!DungeonMonExists(dmon) || !(dmon->dxFlags & DX_FLAG_VISITOR) || GetFriendArea(dmon->speciesNum) != friendArea)
            continue;
        poke = (gDxAdventureTurns - sDxVisitorJoinTurn[i]) * DX_VISITOR_POKE_PER_TURN;
        if (poke < DX_VISITOR_POKE_MIN)
            poke = DX_VISITOR_POKE_MIN;
        if (poke > DX_VISITOR_POKE_MAX)
            poke = DX_VISITOR_POKE_MAX;
        gTeamInventoryRef->teamMoney += poke;
        if (gTeamInventoryRef->teamMoney > MAX_TEAM_MONEY)
            gTeamInventoryRef->teamMoney = MAX_TEAM_MONEY;
        CopyCyanMonsterNametoBuffer(gFormatBuffer_Monsters[0], dmon->speciesNum);
        sprintfStatic(text, sDxVisitorLeaves, poke);
        DisplayDungeonMessage_Async(NULL, text, TRUE);
        dmon->dxFlags &= ~DX_FLAG_VISITOR;
    }
}

// --- camp Exp. ---
#define DX_MAX_LEVEL 100
// Exp. the leader earned this adventure (AddExpPoints)
EWRAM_DATA s32 gDxAdventureExp = 0;
#ifdef DX_DEBUG
// tests: set gDxTestProbe and the next turn stores a few rule results (dungeon_wind.c)
EWRAM_DATA bool8 gDxTestProbe = FALSE;
EWRAM_DATA s16 gDxTestOut[8] = {0};
#endif

static u8 DxStat(s32 v)
{
    return v < 1 ? 1 : (v > 255 ? 255 : v);
}

// Exp. for a Pokemon outside a dungeon, levelling it up with Red's per-level stat gains
void DxGivePokemonExp(Pokemon *mon, s32 exp)
{
    LevelData data;

    if (exp <= 0)
        return;
    mon->currExp += exp;
    while (mon->level < DX_MAX_LEVEL) {
        GetLvlUpEntry(&data, mon->speciesNum, mon->level + 1);
        if (mon->currExp < data.expRequired)
            break;
        mon->level++;
        mon->pokeHP = mon->pokeHP + data.gainHP > 999 ? 999 : mon->pokeHP + data.gainHP;
        mon->offense.att[0] = DxStat(mon->offense.att[0] + data.gainAtt[0]);
        mon->offense.att[1] = DxStat(mon->offense.att[1] + data.gainAtt[1]);
        mon->offense.def[0] = DxStat(mon->offense.def[0] + data.gainDef[0]);
        mon->offense.def[1] = DxStat(mon->offense.def[1] + data.gainDef[1]);
    }
    if (mon->level >= DX_MAX_LEVEL) {
        GetLvlUpEntry(&data, mon->speciesNum, DX_MAX_LEVEL);
        mon->currExp = data.expRequired;
    }
}

// the adventure went well: every Pokemon resting in a camp (not on the team) gets a share
void DxGiveCampExp(void)
{
    s32 i, share = gDxAdventureExp * DX_CAMP_EXP_PERCENT / 100;

    for (i = 0; i < NUM_MONSTERS; i++) {
        Pokemon *mon = &gRecruitedPokemonRef->pokemon[i];
        if (PokemonExists(mon) && !(mon->flags & POKEMON_FLAG_ON_TEAM))
            DxGivePokemonExp(mon, share);
    }
    gDxAdventureExp = 0;
}
