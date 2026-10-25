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
extern bool8 gDxForceRecruit;
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

struct DxStrongFoe { u8 dungeon; s16 species; };
extern u8 gDxClearedDungeons[16];
extern s16 gDxStrongFoeSpecies;
extern bool8 gDxForceStrongFoe;
void DxMarkDungeonCleared(u8 dungeon);
bool8 DxDungeonCleared(u8 dungeon);
void DxRollStrongFoe(void);
void DxMakeStrongFoe(Entity *entity);
void DxStrongFoeReward(Entity *foe);
bool8 DxDungeonHasStrongFoe(u8 dungeon);
// post-game boss rematches
extern bool8 gDxForcePostgame;
bool8 DxPostgame(void);
bool8 DxBossRematch(s16 species);
void DxBriefingSummary(u8 dungeonIndex, u8 *buffer);
struct DxShiny { s16 species; u8 palette; };
s32 DxShinyPalette(s32 species);
extern bool8 gDxForceShiny;

#endif // GUARD_DX_RULES_H
