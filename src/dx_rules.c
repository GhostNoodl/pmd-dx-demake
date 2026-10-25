#include "global.h"

#ifdef DX_RULES

#include "dx_rules.h"
#include "dx_tuning.h"
#include "constants/dungeon.h"
#include "constants/dungeon_action.h"
#include "dungeon_util.h"
#include "dungeon_floor_spawns.h"
#include "pokemon_3.h"
#include "moves.h"
#include "move_orb_effects_1.h"
#include "dungeon_message.h"
#include "dungeon_logic.h"
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
    if (DungeonRandInt(100) < chance)
        move->ginseng++;
}

// --- Empowerment Seed ---
static const u8 sDxEmpowered[] = _("{POKEMON_0} became Empowered!");

// Rescue Team DX: an Empowered Pokemon's stats all rise (Mega forms come on top of this)
void DxEmpower(Entity *user, Entity *target)
{
    StatIndex physical = {STAT_INDEX_PHYSICAL}, special = {STAT_INDEX_SPECIAL};

    if (!EntityIsValid(target))
        return;
    SetMessageArgument_2(gFormatBuffer_Monsters[0], GetEntInfo(target), 0);
    TryDisplayDungeonLoggableMessage3_Async(user, target, sDxEmpowered);
    BoostOffensiveStat(user, target, physical, DX_EMPOWER_STAGES);
    BoostOffensiveStat(user, target, special, DX_EMPOWER_STAGES);
    BoostDefensiveStat(user, target, physical, DX_EMPOWER_STAGES);
    BoostDefensiveStat(user, target, special, DX_EMPOWER_STAGES);
    BoostSpeed(user, target, 0, TRUE);
}

// --- Mystery Houses ---
#include "data/dx_mystery_houses.h"

// the newcomer waiting in this floor's Mystery House (0 = none)
EWRAM_DATA s16 gDxMysteryHouseSpecies = 0;
// tests: force the roll, and record which newcomer actually spawned
EWRAM_DATA bool8 gDxForceMysteryHouse = FALSE;
EWRAM_DATA s16 gDxMysteryHouseSpawned = 0;

// Called as a floor is generated: TRUE forces a Monster House, whose first occupant is the newcomer
bool8 DxRollMysteryHouse(void)
{
    s32 i, count = 0;
    s16 choices[ARRAY_COUNT(sDxMysteryHomes)];
    u8 dungeon = gDungeon->unk644.dungeonLocation.id;

    gDxMysteryHouseSpecies = 0;
    for (i = 0; i < ARRAY_COUNT(sDxMysteryHomes); i++) {
        if (sDxMysteryHomes[i].dungeon == dungeon)
            choices[count++] = sDxMysteryHomes[i].species;
    }
    if (count == 0 || (!gDxForceMysteryHouse && DungeonRandInt(100) >= DX_MYSTERY_HOUSE_CHANCE))
        return FALSE;
    gDxMysteryHouseSpecies = choices[DungeonRandInt(count)];
    return TRUE;
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
