#ifndef GUARD_DX_RARE_QUALITY_H
#define GUARD_DX_RARE_QUALITY_H

#ifdef DX_RULES

#include "constants/rare_quality.h"
#include "structs/dungeon_entity.h"
#include "structs/str_pokemon.h"

const u8 *DxQualityName(u32 quality);
Pokemon *DxEntityPokemon(Entity *entity);
bool8 DxTeamHasQuality(u32 quality);
s32 DxTeamSize(void);
u32 DxRandomQuality(void);
void DxGrantQuality(Entity *user, Entity *target, u32 quality);
void DxGummiItemAction(Entity *user, Entity *target, bool8 alwaysQuality);

#endif // DX_RULES

#endif // GUARD_DX_RARE_QUALITY_H
