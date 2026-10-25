#!/usr/bin/env python3
"""Explorers of Sky level-up data (BALANCE/m_level.bin, as used by pret/pmd-sky).

m_level.bin is a pack of SIR0 files, one per base species (indexed like monster.md),
each wrapping a PKDPX-compressed array of per-level records. A record has the same
12-byte layout as Red's LevelData: s32 exp, u16 hp gain, u8 atk, spatk, def, spdef, u16 pad.
The PX decompressor below is a port of Red's DecompressAT (src/decompress_at.c).
"""
import struct
import urllib.request
from pathlib import Path

CACHE = Path(__file__).resolve().parent / "cache" / "eos_m_level.bin"
URL = "https://raw.githubusercontent.com/pret/pmd-sky/main/files/BALANCE/m_level.bin"


def px_decompress(src, data_start, length):
    flags = [src[7 + i] + 3 for i in range(9)]
    special = {flags[i]: 0x1f - i for i in range(9)}
    dst = bytearray()
    i, cmd_bit, cur = data_start, 8, 0
    while i < length:
        if cmd_bit == 8:
            cur = src[i]; i += 1; cmd_bit = 0
        if cur & 0x80:
            dst.append(src[i]); i += 1
        else:
            command = (src[i] >> 4) + 3
            hi = (src[i] & 0xF) << 8
            command = special.get(command, command)
            if command >= 0x17:
                c = src[i] & 0xF; i += 1
                pat = {0x1f: (0, 0, 0, 0), 0x1e: (0, 1, 1, 1), 0x1d: (0, -1, 0, 0), 0x1c: (0, 0, -1, 0),
                       0x1b: (0, 0, 0, -1), 0x1a: (0, -1, -1, -1), 0x19: (0, 1, 0, 0), 0x18: (0, 0, 1, 0),
                       0x17: (0, 0, 0, 1)}[command]
                n = [(c + d) & 0xF for d in pat]
                dst.append((n[0] << 4) | n[1]); dst.append((n[2] << 4) | n[3])
            else:
                off = hi + src[i + 1]; i += 2
                pos = len(dst) - 0x1000 + off
                for _ in range(command):
                    dst.append(dst[pos]); pos += 1
        cmd_bit += 1
        cur = (cur << 1) & 0xFF
    return bytes(dst)


def load():
    if not CACHE.exists():
        CACHE.parent.mkdir(exist_ok=True)
        CACHE.write_bytes(urllib.request.urlopen(URL, timeout=120).read())
    return CACHE.read_bytes()


def levels(index):
    """List of (exp, hp, atk, spatk, def, spdef) per level for monster.md base index."""
    pack = load()
    count = struct.unpack_from("<I", pack, 4)[0]
    assert index < count
    off, size = struct.unpack_from("<II", pack, 8 + 8 * index)
    sir0 = pack[off:off + size]
    assert sir0[:4] == b"SIR0"
    content = struct.unpack_from("<I", sir0, 4)[0]
    px = sir0[content:]
    assert px[:5] == b"PKDPX", px[:5]
    length = struct.unpack_from("<H", px, 5)[0]
    raw = px_decompress(px, 0x14, length)
    out = []
    for k in range(len(raw) // 12):
        exp, hp, atk, spatk, dfn, spdef, _ = struct.unpack_from("<iHBBBBH", raw, 12 * k)
        out.append((exp, hp, atk, spatk, dfn, spdef))
    return out, raw


if __name__ == "__main__":
    for idx in (25, 489, 490):
        lv, raw = levels(idx)
        print(idx, len(lv), "levels;", lv[:3], "...", lv[-1])
