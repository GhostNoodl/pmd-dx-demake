#ifndef GUARD_DX_RULES_H
#define GUARD_DX_RULES_H

#include "structs/dungeon_entity.h"

bool8 DxActionPausesRegen(u16 action);
s32 DxDoubleMoveChance(Entity *attacker, Entity *target);

struct DxMysteryHome { u8 dungeon; s16 species; };
extern s16 gDxMysteryHouseSpecies;
extern s16 gDxMysteryHouseSpawned;
extern bool8 gDxForceMysteryHouse;
bool8 DxRollMysteryHouse(void);
s32 DxMysteryHouseLevel(void);

#endif // GUARD_DX_RULES_H
