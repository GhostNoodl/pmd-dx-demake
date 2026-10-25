# dx C3 (applied to preprocessed src/monster_gfx*.c, see Makefile DX_LZ_MON)
# 1. monster frames come from the .4bpp.lz file, 4-byte aligned for the BIOS LZ77 decoder
s#static const u8 (s[A-Za-z0-9_]+Gfx[0-9]+)\[\] = INCBIN_U8\("(graphics/ax/mon/[^"]+\.4bpp)"\)#static const u8 \1[] __attribute__((aligned(4))) = INCBIN_U8("\2.lz")#g
# 2. negative byteCount marks the frame as compressed (sprite.c reads the real size from the header)
s#\{(s[A-Za-z0-9_]+Gfx[0-9]+), \(sizeof\(\1\) / sizeof\(\(\1\)\[0\]\)\)\}#{\1, -(s32)sizeof(\1)}#g
