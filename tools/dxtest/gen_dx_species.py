#!/usr/bin/env python3
"""Generate the Rescue Team DX species data for modern builds (DX_RULES).

Primary source for species data: Explorers of Sky (BALANCE/monster.md and m_level.bin
from pret/pmd-sky, read by eos_md.py / eos_level.py). Explorers is PMD-scaled like Red:
its abilities and types use Red's ids (538/538 abilities of shared species match) and its
level-up growth is on the same scale as Red's (e.g. Pikachu gains 102 HP by Lv. 50 in Red,
103 in Explorers). Base stats + growth are taken together from Explorers, since each game's
pair only makes sense together.

Writes:
  data/monster/dx/monster_data.json        Red's table + DX changes + newcomers
  data/monster/dx/learnset/learnset_data.json
  data/monster/dx/level_data_dx.s           lvmp files (SIRO + uncompressed AT3PN) for system_sbin.s
"""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import eos_level  # noqa: E402
import eos_md  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]

# Species DX changes or adds. "eos" = national dex number in Explorers of Sky.
# Fields given here override what Explorers provides.
SPECIES = [
    {   # Red ships Munchlax as a cameo with placeholder data and a placeholder lvmp420
        "id": 420, "eos": 446, "name": "MonsterNameMunchlax", "category": "MonsterCategoryBigEater",
        "friendArea": "FRIEND_AREA_ENERGETIC_FOREST", "recruitRate": 50,
        "learnset": ["METRONOME:1", "ODOR_SLEUTH:1", "TACKLE:1", "DEFENSE_CURL:4", "AMNESIA:9", "LICK:12",
                     "RECYCLE:17", "SCREECH:20", "STOCKPILE:25", "SWALLOW:25", "BODY_SLAM:33", "ROLLOUT:44"],
        "tms": "Snorlax",
    },
    {
        "id": 424, "eos": 447, "name": "MonsterNameRiolu", "category": "MonsterCategoryEmanation",
        "overworldPalette": 5, "friendArea": "FRIEND_AREA_MT_DISCIPLINE", "recruitRate": 30,
        "learnset": ["QUICK_ATTACK:1", "FORESIGHT:1", "ENDURE:1", "COUNTER:6", "REVERSAL:15", "SCREECH:19"],
        "tms": "Machop",
    },
    {   # evolves from Riolu by IQ, Red's stand-in for friendship (like Pichu -> Pikachu)
        "id": 425, "eos": 448, "name": "MonsterNameLucario", "category": "MonsterCategoryAura",
        "overworldPalette": 5, "friendArea": "FRIEND_AREA_MT_DISCIPLINE", "recruitRate": 0,
        "preEvolution": {"evolveFrom": "MONSTER_RIOLU", "evolveType": "EVOLVE_TYPE_IQ"},
        "evolutionRequirements": {"mainRequirement": 200},
        "learnset": ["QUICK_ATTACK:1", "FORESIGHT:1", "DETECT:1", "METAL_CLAW:1", "COUNTER:6", "REVERSAL:15",
                     "SCREECH:19", "BONE_RUSH:24", "SWORDS_DANCE:37", "EXTREMESPEED:51"],
        "tms": "Machoke",
    },
]

TYPE_NAMES = ["TYPE_NONE", "TYPE_NORMAL", "TYPE_FIRE", "TYPE_WATER", "TYPE_GRASS", "TYPE_ELECTRIC", "TYPE_ICE",
              "TYPE_FIGHTING", "TYPE_POISON", "TYPE_GROUND", "TYPE_FLYING", "TYPE_PSYCHIC", "TYPE_BUG", "TYPE_ROCK",
              "TYPE_GHOST", "TYPE_DRAGON", "TYPE_DARK", "TYPE_STEEL"]


def main():
    abilities = {int(v, 0): k for k, v in re.findall(r"#define (ABILITY_\w+) (0x[0-9A-Fa-f]+|\d+)",
                                                    (ROOT / "include/constants/ability.h").read_text())}
    md = eos_md.by_dex(eos_md.load())
    red = json.loads((ROOT / "data/monster/monster_data.json").read_text())
    base_sets = {e["pokemon"]: e for e in json.loads((ROOT / "data/monster/learnset/learnset_data.json").read_text())}
    assert len(red) == 424, len(red)

    learnsets, level_blobs, log = [], [], []
    for s in SPECIES:
        e = md[s["eos"]]
        entry = dict(red[s["id"]]) if s["id"] < len(red) else dict(red[143])  # Snorlax as a template for new ids
        entry.update({
            "name": s["name"], "category": s["category"],
            "bodySize": e["body_size"],
            "types": [TYPE_NAMES[e["type1"]]] + ([TYPE_NAMES[e["type2"]]] if e["type2"] else []),
            "abilities": [abilities[a] for a in (e["ability1"], e["ability2"]) if a and a in abilities] or ["ABILITY_UNKNOWN"],
            "baseHP": e["base_hp"], "baseAtkSpAtk": [e["atk"], e["spatk"]], "baseDefSpDef": [e["def"], e["spdef"]],
            "expYield": round(0.852 * e["exp_yield"] + 13.9),  # Red vs Explorers exp yield fit over shared species
            "weight": e["weight"], "size": e["size"], "chanceAsleep": max(e["asleep"], 0),
            "friendArea": s["friendArea"], "recruitRate": s["recruitRate"],
            "dexInternal": [s["eos"], s["eos"]],
        })
        if s["id"] >= len(red):
            entry["dialogueSprites"] = "0b0000000000000000"  # no portrait yet
            entry["alphabetParent"] = [s["id"], s["id"]]
            entry.pop("preEvolution", None)
            entry.pop("evolutionRequirements", None)
        for k in ("overworldPalette", "preEvolution", "evolutionRequirements"):
            if k in s:
                entry[k] = s[k]
        if s["id"] < len(red):
            red[s["id"]] = entry
        else:
            assert s["id"] == len(red), (s["id"], len(red))
            red.append(entry)

        poke = s["name"].replace("MonsterName", "")
        learnsets.append({
            "pokemon": poke,
            "levelUpMoves": [{"move": "MOVE_" + m.split(":")[0], "level": int(m.split(":")[1])} for m in s["learnset"]],
            "HMTMMoves": base_sets[s["tms"]]["HMTMMoves"],
        })
        _, raw = eos_level.levels(e["index"])
        level_blobs.append((s["id"], raw))
        log.append(f"{poke}: Explorers #{s['eos']} (index {e['index']}): {entry['types']}, {entry['abilities']}, "
                   f"HP {e['base_hp']} Atk/SpA {e['atk']}/{e['spatk']} Def/SpD {e['def']}/{e['spdef']}, body {e['body_size']}")

    out = ROOT / "data/monster/dx"
    (out / "learnset").mkdir(parents=True, exist_ok=True)
    (out / "monster_data.json").write_text(json.dumps(red, indent=4) + "\n")
    (out / "learnset/learnset_data.json").write_text(json.dumps(learnsets, indent=4) + "\n")

    s = ["@ Generated by tools/dxtest/gen_dx_species.py: level-up data from Explorers of Sky (m_level.bin)",
         "@ as Red lvmp files (SIRO header + uncompressed AT3PN stream of 12-byte LevelData records).", ""]
    for sid, raw in level_blobs:
        s += [f".global gDxLevelName{sid}", f"gDxLevelName{sid}:", f'.string "lvmp{sid:03d}\\0"', ".align 2, 0",
              f".global gDxLevel{sid}", f"gDxLevel{sid}:", '.ascii "SIRO"', f".4byte gDxLevelAT{sid}", ".4byte 0, 0",
              f"gDxLevelAT{sid}:", '.ascii "AT3PN"', f".2byte {len(raw)}"]
        for i in range(0, len(raw), 16):
            s.append(".byte " + ", ".join(f"0x{b:02x}" for b in raw[i:i + 16]))
        s += [".align 2, 0", ""]
    (out / "level_data_dx.s").write_text("\n".join(s) + "\n")
    print("\n".join(log))


if __name__ == "__main__":
    main()
