#ifndef GUARD_DX_WARP_H
#define GUARD_DX_WARP_H

#ifdef DX_DEBUG

// dx test tool: debug menu "Warp" screen (pick a dungeon + floor, then start it)
void CreateDxWarpMenu(void);
u32 UpdateDxWarpMenu(void);
void DeleteDxWarpMenu(void);

// Consumed by RunGameMode_Async after the main menu hands over to the game
bool8 DxTakePendingWarp(s16 *scriptDungeon, s32 *floor);

#endif // DX_DEBUG

#endif // GUARD_DX_WARP_H
