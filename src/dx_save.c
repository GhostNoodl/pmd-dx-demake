#include "global.h"

#ifdef DX_RULES

#include "globaldata.h"
#include "dx_save.h"
#include "memory.h"
#include "pokemon.h"
#include "save.h"

// Rescue Team DX additions that don't fit Red's save layout live in their own
// checksummed block in a flash sector Red leaves free (main saves use sectors
// 0-11, the quicksave 16-20, a small block 31). Older saves simply have no block
// there, which reads as "nothing extra yet".
#define DX_SAVE_SECTOR 24
#define DX_SAVE_MAGIC 0x56534458 // "DXSV"
#define DX_SAVE_VERSION 1

typedef struct DxSaveExt
{
    u32 checksum; // CalculateChecksum() writes the first word
    u32 magic;
    u16 version;
    u16 extraSlots;
    u16 pokemonSize;
    u16 pad;
    Pokemon extraRecruits[DX_EXTRA_RECRUIT_SLOTS];
} DxSaveExt;

void DxSaveExtWrite(void)
{
    s32 sector = DX_SAVE_SECTOR;
    DxSaveExt *ext = MemoryAlloc(sizeof(DxSaveExt), MEMALLOC_GROUP_5);

    MemoryFill8(ext, 0, sizeof(DxSaveExt));
    ext->magic = DX_SAVE_MAGIC;
    ext->version = DX_SAVE_VERSION;
    ext->extraSlots = DX_EXTRA_RECRUIT_SLOTS;
    ext->pokemonSize = sizeof(Pokemon);
    MemoryCopy8(ext->extraRecruits, &gRecruitedPokemonRef->pokemon[VANILLA_RECRUIT_SLOTS], sizeof(ext->extraRecruits));
    WriteSaveSector(&sector, (u8 *)ext, sizeof(DxSaveExt));
    MemoryFree(ext);
}

void DxSaveExtRead(void)
{
    s32 sector = DX_SAVE_SECTOR;
    DxSaveExt *ext = MemoryAlloc(sizeof(DxSaveExt), MEMALLOC_GROUP_5);
    Pokemon *extras = &gRecruitedPokemonRef->pokemon[VANILLA_RECRUIT_SLOTS];

    if (ReadSaveSector(&sector, (u8 *)ext, sizeof(DxSaveExt)) == READ_SAVE_VALID
        && ext->magic == DX_SAVE_MAGIC && ext->version == DX_SAVE_VERSION
        && ext->extraSlots == DX_EXTRA_RECRUIT_SLOTS && ext->pokemonSize == sizeof(Pokemon)) {
        MemoryCopy8(extras, ext->extraRecruits, sizeof(ext->extraRecruits));
    }
    else {
        MemoryFill8(extras, 0, sizeof(Pokemon) * DX_EXTRA_RECRUIT_SLOTS);
    }
    MemoryFree(ext);
}

#endif // DX_RULES
