#ifndef GUARD_DX_RULES_H
#define GUARD_DX_RULES_H

#include "structs/dungeon_entity.h"
#include "structs/str_pokemon.h"

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
enum { DX_BOSS_NONE, DX_BOSS_STORY, DX_BOSS_LEGENDARY };
u8 DxPostgameBossKind(u8 monsterBehavior);
s32 DxBossRecruitRate(s32 recruitRate, u8 monsterBehavior);
void DxBriefingSummary(u8 dungeonIndex, u8 *buffer);
struct DxShiny { s16 species; u8 palette; };
s32 DxShinyPalette(s32 species);
u8 DxPokemonPalette(Pokemon *mon);
// recruits without a camp (DX): they help for the adventure, then leave with a gift of Poke
extern u16 gDxAdventureTurns;
void DxMarkVisitor(s32 teamSlot, DungeonMon *dmon);
void DxPayVisitors(u8 friendArea);
// camp Exp. (DX): Pokemon resting in camps grow while the team is out
extern s32 gDxAdventureExp;
#ifdef DX_DEBUG
extern bool8 gDxTestProbe;
extern s16 gDxTestOut[8];
// tests: set the scene on the leader's turn (one wild Pokemon placed next to the leader, the rest gone)
extern u8 gDxTestStage;
extern s8 gDxTestStageFoe[2];
extern u8 gDxTestStageTypes[2];
extern s8 gDxTestStageMate[2];
extern u16 gDxTestStageHP;
extern Entity *gDxTestStageMon;
// tests: a recruit's "Give it a nickname?" is answered No without asking
extern bool8 gDxTestNoNickname;
// tests: the staged wild Pokemon is this floor's Mystery House newcomer
extern bool8 gDxTestStageNewcomer;
// tests: [0] team Exp lines, [1] team members' own Exp lines, [2] regular attacks by anyone but the
// leader, [3] AI moves used, [4] Struggles
extern u16 gDxTestCount[5];
void DxTestStageScene(Entity *leader);
#endif
// the move deals damage (Red's move data gives status moves small powers too)
bool8 DxMoveIsDamaging(Move *move);
void DxGivePokemonExp(Pokemon *mon, s32 exp);
void DxGiveCampExp(void);
void DxMysteryHouseSpawned(Entity *mon);
extern bool8 gDxForceShiny;

#endif // GUARD_DX_RULES_H
