#!/usr/bin/env python3
"""Read Explorers of Sky's species table (BALANCE/monster.md) as used by pret/pmd-sky.

Record layout (68 bytes) follows SkyTemple's documentation of monster.md.
"""
import struct
import urllib.request
from pathlib import Path

CACHE = Path(__file__).resolve().parent / "cache" / "eos_monster.md"
URL = "https://raw.githubusercontent.com/pret/pmd-sky/main/files/BALANCE/monster.md"

FIELDS = [
    ("entid", "H"), ("unk31", "H"), ("dex", "H"), ("speed", "H"), ("pre_evo", "H"), ("evo_method", "H"),
    ("evo_param1", "H"), ("evo_param2", "H"), ("sprite", "H"), ("gender", "B"), ("body_size", "B"),
    ("type1", "B"), ("type2", "B"), ("movement", "B"), ("iq_group", "B"), ("ability1", "B"), ("ability2", "B"),
    ("flags", "H"), ("exp_yield", "H"), ("recruit1", "h"), ("base_hp", "H"), ("recruit2", "h"),
    ("atk", "B"), ("spatk", "B"), ("def", "B"), ("spdef", "B"), ("weight", "H"), ("size", "H"),
    ("unk17", "B"), ("unk18", "B"), ("shadow", "b"), ("asleep", "b"), ("regen", "B"), ("unk21", "b"),
    ("base_form", "h"), ("ex1", "h"), ("ex2", "h"), ("ex3", "h"), ("ex4", "h"),
    ("unk27", "h"), ("unk28", "h"), ("unk29", "h"), ("unk30", "h"),
]
FMT = "<" + "".join(f for _, f in FIELDS)
assert struct.calcsize(FMT) == 68


def load():
    if not CACHE.exists():
        CACHE.parent.mkdir(exist_ok=True)
        CACHE.write_bytes(urllib.request.urlopen(URL, timeout=60).read())
    data = CACHE.read_bytes()
    assert data[:4] == b"MD\0\0"
    count = struct.unpack_from("<I", data, 4)[0]
    out = []
    for i in range(count):
        rec = dict(zip((n for n, _ in FIELDS), struct.unpack_from(FMT, data, 8 + 68 * i)))
        rec["index"] = i
        out.append(rec)
    return out


def by_dex(entries):
    """First (male/base) entry for each national dex number."""
    seen = {}
    for e in entries:
        if e["dex"] and e["dex"] not in seen:
            seen[e["dex"]] = e
    return seen


if __name__ == "__main__":
    md = by_dex(load())
    for d in (25, 143, 446, 447, 448):
        e = md[d]
        print(d, {k: e[k] for k in ("index", "body_size", "type1", "type2", "ability1", "ability2", "base_hp", "atk", "spatk", "def", "spdef", "exp_yield", "recruit1", "recruit2", "pre_evo", "evo_method", "evo_param1", "weight", "size")})
