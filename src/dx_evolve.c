#include "global.h"

#ifdef DX_DEBUG

#include "globaldata.h"
#include "constants/main_menu.h"
#include "dx_evolve.h"
#include "luminous_cave.h"

// Debug menu "Evolve": Luminous Cave's evolution screen for the team leader, from the title
// menu, so evolutions (Evo Crystals, DX form choice) can be tried without the story trip.
// Changes stay in RAM until the game is saved (e.g. by playing on and saving, or Storage).
static EWRAM_DATA bool8 sDxEvolveOpen = FALSE;

void CreateDxEvolveMenu(void)
{
    sub_802465C();
    sDxEvolveOpen = TRUE;
}

u32 UpdateDxEvolveMenu(void)
{
    u32 result;

    if (!sDxEvolveOpen)
        return MENU_DEBUG;
    result = sub_80246F0();
    if (result == 2 || result == 3) {
        CleanLuminousCave();
        sDxEvolveOpen = FALSE;
        return MENU_DEBUG;
    }
    return MENU_NO_SCREEN_CHANGE;
}

void DeleteDxEvolveMenu(void)
{
    if (sDxEvolveOpen) {
        CleanLuminousCave();
        sDxEvolveOpen = FALSE;
    }
}

#endif // DX_DEBUG
