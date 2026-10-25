#ifndef GUARD_CONSTANTS_GLOBAL_H
#define GUARD_CONSTANTS_GLOBAL_H

#define POKEMON_NAME_LENGTH 10
#define TEAM_NAME_LENGTH 10
#ifdef DX_PARTY_SIZE
// dx: bigger in-dungeon party (Rescue Team DX allows up to 8 with no body-size limit)
#define MAX_TEAM_MEMBERS DX_PARTY_SIZE
#define MAX_TEAM_BODY_SIZE (DX_PARTY_SIZE * 3 / 2)
#else
#define MAX_TEAM_MEMBERS 4
#define MAX_TEAM_BODY_SIZE 6
#endif
// Team slots past the original four are saved after a marker, so 4-member saves still load
#define VANILLA_TEAM_MEMBERS 4
#define DX_SAVE_TAIL_MAGIC 0x6478

#define MAX_MON_MOVES 4

#define MALE 0
#define FEMALE 1

#endif // GUARD_CONSTANTS_GLOBAL_H
