#ifndef GUARD_DX_RULES_H
#define GUARD_DX_RULES_H

#include "structs/dungeon_entity.h"

bool8 DxActionPausesRegen(u16 action);
s32 DxDoubleMoveChance(Entity *attacker, Entity *target);

struct DxMysteryHome { u8 dungeon; s16 species; };
extern s16 gDxMysteryHouseSpecies;
extern bool8 gDxMysteryHouseFloor;
extern s16 gDxMysteryHouseSpawned;
extern bool8 gDxForceMysteryHouse;
bool8 DxRollMysteryHouse(void);
void DxTryMoveGrowth(EntityInfo *user, Move *move);
void DxEmpower(Entity *user, Entity *target);
s32 DxMegaBase(s32 species);

struct DxMegaForm
{
    s16 base;
    s16 mega;
    u8 stages[4]; // extra Atk, Sp. Atk, Def, Sp. Def stages
    bool8 faster;
    bool8 primal;
};
s32 DxMysteryHouseLevel(void);

#endif // GUARD_DX_RULES_H
