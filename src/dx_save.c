#include "global.h"
#include <stddef.h>

#ifdef DX_RULES

#include "globaldata.h"
#include "dx_save.h"
#include "dx_rules.h"
#include "exclusive_pokemon.h"
#include "memory.h"
#include "pokemon.h"
#include "save.h"

// Rescue Team DX additions that don't fit Red's save layout live in their own
// checksummed block in a flash sector Red leaves free (main saves use sectors
// 0-11, the quicksave 16-20, a small block 31). Older saves simply have no block
// there, which reads as "nothing extra yet".
#define DX_SAVE_SECTOR 24
#define DX_SAVE_MAGIC 0x56534458 // "DXSV"
#define DX_SAVE_VERSION 5 // v4: Rare Qualities and DX flags for every recruit slot; v5: cleared dungeons
// room for every planned newcomer, so adding species doesn't change the layout
#define DX_SEEN_CAPACITY 64
#define DX_SAVE_V2_SEEN 2 // v2 (Riolu/Lucario builds) stored exactly 2 seen flags

typedef struct DxSaveExt
{
    u32 checksum; // CalculateChecksum() writes the first word
    u32 magic;
    u16 version;
    u16 extraSlots;
    u16 pokemonSize;
    u16 pad;
    Pokemon extraRecruits[DX_EXTRA_RECRUIT_SLOTS];
    u8 newSpeciesSeen[DX_SEEN_CAPACITY]; // Red's save only has seen flags for Red's species
    // v4: Red's save drops these Pokemon fields (they live in Red's struct padding)
    u8 qualities[NUM_MONSTERS];
    u8 dxFlags[NUM_MONSTERS];
    // v5: dungeons cleared at least once (Strong Foes)
    u8 clearedDungeons[16];
} DxSaveExt;

typedef char DxSeenCapacityCheck[DX_NEW_SPECIES_COUNT <= DX_SEEN_CAPACITY ? 1 : -1];

void DxSaveExtWrite(void)
{
    s32 i;
    s32 sector = DX_SAVE_SECTOR;
    DxSaveExt *ext = MemoryAlloc(sizeof(DxSaveExt), MEMALLOC_GROUP_5);

    MemoryFill8(ext, 0, sizeof(DxSaveExt));
    ext->magic = DX_SAVE_MAGIC;
    ext->version = DX_SAVE_VERSION;
    ext->extraSlots = DX_EXTRA_RECRUIT_SLOTS;
    ext->pokemonSize = sizeof(Pokemon);
    MemoryCopy8(ext->extraRecruits, &gRecruitedPokemonRef->pokemon[VANILLA_RECRUIT_SLOTS], sizeof(ext->extraRecruits));
    for (i = 0; i < DX_NEW_SPECIES_COUNT; i++)
        ext->newSpeciesSeen[i] = GetMonSeenFlag(DX_FIRST_NEW_SPECIES + i);
    for (i = 0; i < NUM_MONSTERS; i++) {
        ext->qualities[i] = gRecruitedPokemonRef->pokemon[i].dxQuality;
        ext->dxFlags[i] = gRecruitedPokemonRef->pokemon[i].dxFlags;
    }
    MemoryCopy8(ext->clearedDungeons, gDxClearedDungeons, sizeof(ext->clearedDungeons));
    WriteSaveSector(&sector, (u8 *)ext, sizeof(DxSaveExt));
    MemoryFree(ext);
}

void DxSaveExtRead(void)
{
    s32 i;
    s32 seenCount = -1;
    bool8 hasQualities = FALSE;
    bool8 hasCleared = FALSE;
    s32 sector = DX_SAVE_SECTOR;
    DxSaveExt *ext = MemoryAlloc(sizeof(DxSaveExt), MEMALLOC_GROUP_5);
    Pokemon *extras = &gRecruitedPokemonRef->pokemon[VANILLA_RECRUIT_SLOTS];

    if (ReadSaveSector(&sector, (u8 *)ext, sizeof(DxSaveExt)) == READ_SAVE_VALID
        && ext->magic == DX_SAVE_MAGIC && ext->version == DX_SAVE_VERSION) {
        seenCount = DX_NEW_SPECIES_COUNT;
        hasQualities = TRUE;
        hasCleared = TRUE;
    }
    else if ((sector = DX_SAVE_SECTOR, ReadSaveSector(&sector, (u8 *)ext, offsetof(DxSaveExt, clearedDungeons)) == READ_SAVE_VALID)
        && ext->magic == DX_SAVE_MAGIC && ext->version == 4) {
        seenCount = DX_NEW_SPECIES_COUNT;
        hasQualities = TRUE;
    }
    else if ((sector = DX_SAVE_SECTOR, ReadSaveSector(&sector, (u8 *)ext, offsetof(DxSaveExt, qualities)) == READ_SAVE_VALID)
        && ext->magic == DX_SAVE_MAGIC && ext->version == 3)
        seenCount = DX_NEW_SPECIES_COUNT;
    else if ((sector = DX_SAVE_SECTOR, ReadSaveSector(&sector, (u8 *)ext, offsetof(DxSaveExt, newSpeciesSeen) + DX_SAVE_V2_SEEN)) == READ_SAVE_VALID
        && ext->magic == DX_SAVE_MAGIC && ext->version == 2)
        seenCount = DX_SAVE_V2_SEEN;
    if (seenCount >= 0 && ext->extraSlots == DX_EXTRA_RECRUIT_SLOTS && ext->pokemonSize == sizeof(Pokemon)) {
        MemoryCopy8(extras, ext->extraRecruits, sizeof(ext->extraRecruits));
        for (i = 0; i < seenCount; i++) {
            if (ext->newSpeciesSeen[i])
                SetMonSeenFlag(DX_FIRST_NEW_SPECIES + i);
        }
    }
    else {
        MemoryFill8(extras, 0, sizeof(Pokemon) * DX_EXTRA_RECRUIT_SLOTS);
    }
    for (i = 0; i < NUM_MONSTERS; i++) {
        gRecruitedPokemonRef->pokemon[i].dxQuality = hasQualities ? ext->qualities[i] : 0;
        gRecruitedPokemonRef->pokemon[i].dxFlags = hasQualities ? ext->dxFlags[i] : 0;
    }
    if (hasCleared)
        MemoryCopy8(gDxClearedDungeons, ext->clearedDungeons, sizeof(gDxClearedDungeons));
    else
        MemoryFill8(gDxClearedDungeons, 0, sizeof(gDxClearedDungeons));
    MemoryFree(ext);
}

#endif // DX_RULES
