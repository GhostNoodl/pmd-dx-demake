#!/usr/bin/env python3
"""Generate the Rescue Team DX species data for modern builds (DX_RULES).

Writes data/monster/dx/monster_data.json (Red's table with DX changes) and
data/monster/dx/learnset/learnset_data.json (learnsets for species that Red
leaves without one). The build turns them into .inc files with dungeonjson,
like Red's own data, and data/system_sbin.s picks them under DX_RULES.

Munchlax (#420) already exists in Red as a cameo with a sprite and portrait
but placeholder data. Stats are Red-scale estimates (Snorlax's, scaled by the
mainline Munchlax/Snorlax ratio); DX's own numbers aren't published.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

MONSTER_OVERRIDES = {
    420: {  # Munchlax
        "category": "MonsterCategoryBigEater",
        "types": ["TYPE_NORMAL"],
        "friendArea": "FRIEND_AREA_ENERGETIC_FOREST",  # Snorlax's camp
        "abilities": ["ABILITY_PICKUP", "ABILITY_THICK_FAT"],
        "shadowSize": 1,
        "chanceAsleep": 8,
        "baseHP": 28,
        "expYield": 60,
        "baseAtkSpAtk": [5, 5],
        "baseDefSpDef": [5, 5],
        "recruitRate": 50,
        "dexInternal": [446, 446],
    },
}

LEARNSETS = [
    {   # Munchlax: Gen 4 level-up moves that exist in Red; TMs as Snorlax
        "pokemon": "Munchlax",
        "levelUpMoves": [
            {"move": "MOVE_METRONOME", "level": 1}, {"move": "MOVE_ODOR_SLEUTH", "level": 1},
            {"move": "MOVE_TACKLE", "level": 1}, {"move": "MOVE_DEFENSE_CURL", "level": 4},
            {"move": "MOVE_AMNESIA", "level": 9}, {"move": "MOVE_LICK", "level": 12},
            {"move": "MOVE_RECYCLE", "level": 17}, {"move": "MOVE_SCREECH", "level": 20},
            {"move": "MOVE_STOCKPILE", "level": 25}, {"move": "MOVE_SWALLOW", "level": 25},
            {"move": "MOVE_BODY_SLAM", "level": 33}, {"move": "MOVE_ROLLOUT", "level": 44},
        ],
        "HMTMMovesFrom": "Snorlax",
    },
]


def main():
    data = json.loads((ROOT / "data/monster/monster_data.json").read_text())
    for idx, fields in MONSTER_OVERRIDES.items():
        data[idx].update(fields)
    out = ROOT / "data/monster/dx"
    (out / "learnset").mkdir(parents=True, exist_ok=True)
    (out / "monster_data.json").write_text(json.dumps(data, indent=4) + "\n")

    base = {e["pokemon"]: e for e in json.loads((ROOT / "data/monster/learnset/learnset_data.json").read_text())}
    sets = []
    for e in LEARNSETS:
        e = dict(e)
        e["HMTMMoves"] = base[e.pop("HMTMMovesFrom")]["HMTMMoves"]
        sets.append(e)
    (out / "learnset/learnset_data.json").write_text(json.dumps(sets, indent=4) + "\n")
    print(f"dx species: {len(MONSTER_OVERRIDES)} overrides, {len(sets)} learnsets")


if __name__ == "__main__":
    main()
