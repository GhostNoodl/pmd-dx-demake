#ifndef GUARD_DX_RULES_H
#define GUARD_DX_RULES_H

#include "structs/dungeon_entity.h"

bool8 DxActionPausesRegen(u16 action);
s32 DxDoubleMoveChance(Entity *attacker, Entity *target);

#endif // GUARD_DX_RULES_H
