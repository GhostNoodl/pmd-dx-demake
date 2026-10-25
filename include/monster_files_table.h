#ifndef GUARD_MONSTER_FILES_TABLE_H
#define GUARD_MONSTER_FILES_TABLE_H

#ifdef DX_RULES
#define DX_NEW_SPECIES_FILES DX_NEW_SPECIES_COUNT
#else
#define DX_NEW_SPECIES_FILES 0
#endif
#define MONSTER_FILES_COUNT (497 + DX_NEW_SPECIES_FILES)

#include "structs/str_file_system.h"

extern const File gMonsterFiles[MONSTER_FILES_COUNT];

#endif // GUARD_MONSTER_FILES_TABLE_H
