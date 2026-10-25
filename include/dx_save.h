#ifndef GUARD_DX_SAVE_H
#define GUARD_DX_SAVE_H

#ifdef DX_RULES
// Extra save block for Rescue Team DX data (see dx_save.c)
void DxSaveExtWrite(void);
void DxSaveExtRead(void);
#endif

#endif // GUARD_DX_SAVE_H
