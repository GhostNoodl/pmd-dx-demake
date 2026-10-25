#ifndef GUARD_POKEMON_EVOLUTION_H
#define GUARD_POKEMON_EVOLUTION_H

#include "structs/str_pokemon.h"

enum
{
    EVO_STRING_NOT_NOW,
    EVO_STRING_POSSIBLE,
    EVO_STRING_NO_MORE,
    EVO_STRING_EMPTY,
};

// size: 0x8
#ifdef DX_RULES
#define DX_MAX_EVO_CANDIDATES 8 // Eevee
#define DX_EVO_REQ_TWO_CRYSTALS 14 // additionalRequirement: two Evo Crystals (gen_dx_species.py)
#endif

typedef struct EvolveStatus
{
    /* 0x0 */ u8 evoItem1;
    /* 0x1 */ u8 evoItem2;
    // This value is set to a random number in the range [0, 254].
    // Even values produce Silcoon while odd values produce Cascoon.
    /* 0x2 */ u8 wurmpleVal;
    /* 0x4 */ u16 evolutionConditionStatus;
    /* 0x6 */ s16 targetEvolveSpecies;
#ifdef DX_RULES
    // every form whose conditions are met (DX lets you pick, e.g. Eevee with two Evo Crystals)
    s16 dxCandidates[DX_MAX_EVO_CANDIDATES];
    u8 dxCandidateCount;
#endif
} EvolveStatus;

void SetMonEvolveStatus(Pokemon *pokemon, EvolveStatus *evolveStatus,bool8 param_3);
s32 GetMonSummaryScreenEvoStringId(Pokemon *pokemon);
Pokemon *sub_808F734(Pokemon *pokemon, s16 _species);
Pokemon *sub_808F798(Pokemon *pokemon, s16 _species);

#endif // GUARD_POKEMON_EVOLUTION_H
