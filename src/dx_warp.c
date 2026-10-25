#include "global.h"

#ifdef DX_DEBUG

#include "globaldata.h"
#include "constants/main_menu.h"
#include "constants/script_dungeon_id.h"
#include "code_80A26CC.h"
#include "dungeon_info.h"
#include "dx_warp.h"
#include "input.h"
#include "memory.h"
#include "menu_input.h"
#include "save.h"
#include "text_1.h"
#include "text_2.h"
#include "text_3.h"

// Return value of the main menu that RunGameMode_Async treats as "Continue"
#define MAIN_MENU_CONTINUE_GAME 2

#define INFO_WINDOW 0
#define INPUT_WINDOW 1

enum DxWarpState
{
    DX_WARP_PICK_DUNGEON,
    DX_WARP_PICK_FLOOR,
};

typedef struct DxWarpWork
{
    u32 state;
    s32 count;
    s16 scriptDungeons[SCRIPT_DUNGEON_COUNT];
    unkStructFor8013AA0 input;
    WindowTemplates windows;
} DxWarpWork;

static EWRAM_DATA DxWarpWork *sDxWarp = NULL;
static EWRAM_DATA s32 sDxWarpLastIndex = 0;
static EWRAM_DATA bool8 sDxWarpPending = FALSE;
static EWRAM_DATA s16 sDxWarpScriptDungeon = 0;
static EWRAM_DATA s32 sDxWarpFloor = 0;

static const WindowTemplate sDxWarpEmptyWindow = {
    0,
    3,
    0, 0,
    0, 0,
    0, 0,
    NULL
};

static const WindowTemplate sDxWarpInfoWindow = {
    0,
    3,
    2, 2,
    26, 5,
    5, 0,
    NULL
};

static const WindowTemplate sDxWarpInputWindow = {
    0,
    3,
    2, 9,
    8, 4,
    4, 0,
    NULL
};

ALIGNED(4) static const u8 sDxPickDungeon[] = _("Warp: pick a dungeon (B: back)");
ALIGNED(4) static const u8 sDxPickFloor[] = _("Warp: pick a floor (B: dungeon)");
ALIGNED(4) static const u8 sDxFmtDungeon[] = _("#%d {COLOR CYAN}%s{RESET}");
ALIGNED(4) static const u8 sDxFmtFloors[] = _("Floors 1-%d   A: confirm");
ALIGNED(4) static const u8 sDxDungeonPrompt[] = _("Dungeon");
ALIGNED(4) static const u8 sDxFloorPrompt[] = _("Floor");

static void DxWarpSetState(u32 state);
static void DxWarpDraw(void);

static s32 DxWarpCurrentIndex(void)
{
    if (sDxWarp->state == DX_WARP_PICK_DUNGEON)
        return sDxWarp->input.unk0;
    return sDxWarpLastIndex;
}

static u8 DxWarpDungeonId(s32 index)
{
    return ScriptDungeonIdToDungeonId(sDxWarp->scriptDungeons[index]);
}

void CreateDxWarpMenu(void)
{
    s32 i;

    if (sDxWarp == NULL) {
        sDxWarp = MemoryAlloc(sizeof(DxWarpWork), MEMALLOC_GROUP_8);
        MemoryFill8(sDxWarp, 0, sizeof(DxWarpWork));
    }

    // Every enterable script dungeon (80-82 are rescue/mission placeholders)
    sDxWarp->count = 0;
    for (i = 0; i < SCRIPT_DUNGEON_80; i++) {
        const DungeonInfo *info = GetScriptDungeonInfo(i);

        if (!info->unk11 || info->dungeonID >= DUNGEON_OUT_ON_RESCUE)
            continue;
        if (GetDungeonFloorCount(info->dungeonID) < 1)
            continue;
        sDxWarp->scriptDungeons[sDxWarp->count++] = i;
    }

    if (sDxWarpLastIndex >= sDxWarp->count)
        sDxWarpLastIndex = 0;

    DxWarpSetState(DX_WARP_PICK_DUNGEON);
}

void DeleteDxWarpMenu(void)
{
    ResetUnusedInputStruct();
    ShowWindows(NULL, TRUE, TRUE);

    if (sDxWarp != NULL) {
        MemoryFree(sDxWarp);
        sDxWarp = NULL;
    }
}

u32 UpdateDxWarpMenu(void)
{
    // A warp needs a team to send in
    if (sub_8011C34() == -1 || sDxWarp->count == 0)
        return MENU_MAIN_SCREEN;

    switch (sub_8013BBC(&sDxWarp->input)) {
        case 1:
            DxWarpDraw();
            break;
        case 2:
            if (sDxWarp->state == DX_WARP_PICK_FLOOR)
                DxWarpSetState(DX_WARP_PICK_DUNGEON);
            else
                return MENU_DEBUG;
            break;
        case 3:
            if (sDxWarp->state == DX_WARP_PICK_DUNGEON) {
                sDxWarpLastIndex = sDxWarp->input.unk0;
                DxWarpSetState(DX_WARP_PICK_FLOOR);
            }
            else {
                sDxWarpPending = TRUE;
                sDxWarpScriptDungeon = sDxWarp->scriptDungeons[sDxWarpLastIndex];
                sDxWarpFloor = sDxWarp->input.unk0;
                return MAIN_MENU_CONTINUE_GAME;
            }
            break;
    }

    return MENU_NO_SCREEN_CHANGE;
}

bool8 DxTakePendingWarp(s16 *scriptDungeon, s32 *floor)
{
    if (!sDxWarpPending)
        return FALSE;

    sDxWarpPending = FALSE;
    *scriptDungeon = sDxWarpScriptDungeon;
    *floor = sDxWarpFloor;
    return TRUE;
}

static void DxWarpSetState(u32 state)
{
    s32 i;
    unkStructFor8013AA0 *input = &sDxWarp->input;

    sDxWarp->state = state;

    for (i = 0; i < 4; i++)
        sDxWarp->windows.id[i] = sDxWarpEmptyWindow;
    sDxWarp->windows.id[INFO_WINDOW] = sDxWarpInfoWindow;
    sDxWarp->windows.id[INPUT_WINDOW] = sDxWarpInputWindow;
    ResetUnusedInputStruct();
    ShowWindows(&sDxWarp->windows, TRUE, TRUE);

    // Same digit-entry widget the developers' debug screens use (up/down: digit, left/right: column)
    if (state == DX_WARP_PICK_DUNGEON) {
        input->unk4 = sDxWarpLastIndex;
        input->unk8 = 0;
        input->unkC = sDxWarp->count - 1;
    }
    else {
        input->unk4 = 1;
        input->unk8 = 1;
        input->unkC = GetDungeonFloorCount(DxWarpDungeonId(sDxWarpLastIndex));
    }
    input->unk10 = 2;
    input->unk14 = INPUT_WINDOW;
    input->unk18 = &sDxWarp->windows.id[INPUT_WINDOW];
    input->unk1C = 60;
    input->unk20 = 18;
    sub_8013AA0(input);

    DxWarpDraw();
}

static void DxWarpDraw(void)
{
    u8 buffer[96];
    s32 index = DxWarpCurrentIndex();
    u8 dungeonId;

    if (index < 0 || index >= sDxWarp->count)
        index = 0;
    dungeonId = DxWarpDungeonId(index);

    CallPrepareTextbox_8008C54(INFO_WINDOW);
    sub_80073B8(INFO_WINDOW);
    PrintStringOnWindow(4, 0, (sDxWarp->state == DX_WARP_PICK_DUNGEON) ? sDxPickDungeon : sDxPickFloor, INFO_WINDOW, 0);
    sprintfStatic((char *)buffer, (const char *)sDxFmtDungeon, index, GetDungeonName1(dungeonId));
    PrintStringOnWindow(4, 14, buffer, INFO_WINDOW, 0);
    sprintfStatic((char *)buffer, (const char *)sDxFmtFloors, GetDungeonFloorCount(dungeonId));
    PrintStringOnWindow(4, 26, buffer, INFO_WINDOW, 0);
    sub_80073E0(INFO_WINDOW);

    CallPrepareTextbox_8008C54(INPUT_WINDOW);
    sub_80073B8(INPUT_WINDOW);
    PrintStringOnWindow(4, 0, (sDxWarp->state == DX_WARP_PICK_DUNGEON) ? sDxDungeonPrompt : sDxFloorPrompt, INPUT_WINDOW, 0);
    sub_8013C68(&sDxWarp->input);
    sub_80073E0(INPUT_WINDOW);
}

#endif // DX_DEBUG
