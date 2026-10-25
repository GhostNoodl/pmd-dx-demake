#include "global.h"
#include "structs/str_pokemon.h"
#include <stddef.h>
const int monSize = sizeof(Pokemon);
const int monFlags = offsetof(Pokemon, flags);
const int monSpecies = offsetof(Pokemon, speciesNum);
const int monLevel = offsetof(Pokemon, level);
