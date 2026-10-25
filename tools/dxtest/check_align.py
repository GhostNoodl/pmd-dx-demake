#!/usr/bin/env python3
"""Fail if DX data shifted a word-aligned table off its alignment.

DX builds insert data (learnsets, loot, species) into Red's data files. Red's tables often rely
on landing word-aligned by accident; when an insertion leaves them 2 bytes off, pointer fields
read rotated values and the game breaks in odd places (twice so far: garbled move names, and a
dungeon that never loaded). Every data/ assembly label that is word-aligned in the matching build
(pmd_red.elf) must still be word-aligned in the given ELF.

    check_align.py pmd_red.elf pmd_red_modern.elf
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def asm_data_labels():
    """Labels defined in data/ assembly (C constants move around freely under modern GCC)."""
    names = set()
    for f in list((ROOT / "data").rglob("*.s")) + list((ROOT / "data").rglob("*.inc")):
        lines = f.read_text(errors="ignore").splitlines()
        for i, line in enumerate(lines):
            m = re.match(r"^(\w+):", line)
            if not m:
                continue
            # only tables of words/pointers need word alignment (byte and halfword data don't)
            for nxt in lines[i + 1:i + 6]:
                nxt = nxt.strip()
                if not nxt or nxt.startswith("@") or nxt.startswith(".global") or re.match(r"^\w+:", nxt):
                    continue
                if re.match(r"^\.(4byte|word|int|long)\b", nxt):
                    names.add(m.group(1))
                break
    return names


def data_syms(elf):
    out = subprocess.run(["arm-none-eabi-nm", elf], capture_output=True, text=True, check=True).stdout
    syms = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in "rRdD" and parts[0][:2] in ("08", "09"):
            syms[parts[2]] = int(parts[0], 16)
    return syms


def main():
    vanilla, dx = data_syms(sys.argv[1]), data_syms(sys.argv[2])
    labels = asm_data_labels()
    vanilla = {n: a for n, a in vanilla.items() if n in labels}
    bad = sorted(n for n, a in vanilla.items() if a % 4 == 0 and n in dx and dx[n] % 4 != 0)
    if bad:
        print(f"check_align: {len(bad)} word-aligned table(s) are misaligned in {sys.argv[2]}:")
        for n in bad[:20]:
            print(f"  {n} @ {dx[n]:08x}")
        sys.exit(1)
    print(f"check_align: OK ({len(vanilla)} data symbols checked)")


if __name__ == "__main__":
    main()
