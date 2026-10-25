#ifndef GUARD_DX_RARE_QUALITY_H
#define GUARD_DX_RARE_QUALITY_H

#ifdef DX_RULES

#include "constants/rare_quality.h"
#include "structs/dungeon_entity.h"
#include "structs/str_pokemon.h"

const u8 *DxQualityName(u32 quality);
Pokemon *DxEntityPokemon(Entity *entity);
u8 DxEntityQuality(Entity *entity);
u8 DxRecruitQuality(s16 species, bool8 shiny, Entity *leader);
bool8 DxTeamHasQuality(u32 quality);
s32 DxTeamSize(void);
u32 DxRandomQuality(void);
void DxGrantQuality(Entity *user, Entity *target, u32 quality);
void DxGummiItemAction(Entity *user, Entity *target, bool8 alwaysQuality);
bool8 DxQualityAlwaysHits(Entity *attacker, u16 moveId);
bool8 DxSteamroll(Entity *attacker);
s32 DxQualityDamagePercent(Entity *attacker, u16 moveId);
s32 DxBlastDamage(Entity *target, s32 dmg, s32 dungeonExitReason);
void DxQualityOnTeamHit(Entity *attacker, Entity *target);
void DxQualityOnDefeat(Entity *attacker);
bool8 DxTeamMemberHas(Entity *entity, u32 quality);
void DxSqueezeOut(Entity *entity);
s32 DxShopPrice(s32 price, bool8 selling);
s32 DxTightFormationPercent(Entity *target, u16 moveId);
void DxLeaveHalf(Entity *target, u8 itemId);
void DxSharedHappiness(Entity *user, Entity *target, u8 kind);
bool8 DxCanForgeAPath(Entity *entity);
void DxLeapAhead(Entity *leader);
void DxForgeAPathCost(Entity *entity);

// DxSharedHappiness: which bad condition was cured
enum { DX_CURE_ALL, DX_CURE_PARALYSIS, DX_CURE_POISON, DX_CURE_BURN };

#endif // DX_RULES

#endif // GUARD_DX_RARE_QUALITY_H
