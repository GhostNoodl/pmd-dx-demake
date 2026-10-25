#include "global.h"

#ifdef DX_RULES

#include "dx_rules.h"
#include "dx_tuning.h"
#include "constants/dungeon_action.h"
#include "dungeon_util.h"

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

#endif // DX_RULES
