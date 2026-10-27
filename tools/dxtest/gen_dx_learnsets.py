#!/usr/bin/env python3
"""DX learnsets: every species learns its Gen 7 (Ultra Sun / Ultra Moon) level-up moves.

Writes
  data/monster/dx/red_learnset/learnset_data.json   Red's species (DX builds include it instead of Red's file;
                                                     same labels, so Red's pointer table is unchanged)
  data/monster/dx/learnset/learnset_data.json       the DX newcomers' level-up moves, rewritten in place
  tools/dxtest/dx_learnset_log.txt                   moves skipped because Red doesn't have them, with counts

From PokeAPI's GraphQL API (cached in cache/pokeapi_usum.json). Level 0 (learned on evolving) counts as 1.
TM/HM lists stay Red's. A species PokeAPI has nothing for keeps its Red learnset."""
import json
import re
import urllib.request
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
CACHE = HERE / "cache" / "pokeapi_usum.json"
URL = "https://beta.pokeapi.co/graphql/v1beta"
USUM = 18  # PokeAPI version group ultra-sun-ultra-moon
QUERY = """query($ids: [Int!]) {
  pokemon_v2_pokemonmove(where: {pokemon_id: {_in: $ids}, version_group_id: {_eq: %d},
                                 pokemon_v2_movelearnmethod: {name: {_eq: "level-up"}}}) {
    pokemon_id level pokemon_v2_move { name }
  }
}""" % USUM
# PokeAPI name -> Red name, where they differ
RENAMED = {"feintattack": "faintattack", "highjumpkick": "hijumpkick", "smellingsalts": "smellingsalt", "visegrip": "vicegrip"}
MOVE_CLASSES = HERE / "cache" / "pokeapi_moves.json"  # tools/dxtest/gen_dx_move_class.py


def norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


def fetch(dexes):
    cache = json.loads(CACHE.read_text()) if CACHE.exists() else {}
    missing = sorted(d for d in set(dexes) if str(d) not in cache)
    for i in range(0, len(missing), 100):
        chunk = missing[i:i + 100]
        body = json.dumps({"query": QUERY, "variables": {"ids": chunk}}).encode()
        req = urllib.request.Request(URL, body, {"Content-Type": "application/json", "User-Agent": "pmd-dx-demake-tools/1.0"})
        rows = json.loads(urllib.request.urlopen(req, timeout=180).read())["data"]["pokemon_v2_pokemonmove"]
        for d in chunk:
            cache[str(d)] = sorted({(r["level"], r["pokemon_v2_move"]["name"]) for r in rows if r["pokemon_id"] == d})
    CACHE.parent.mkdir(exist_ok=True)
    CACHE.write_text(json.dumps(cache, indent=1))
    return {int(k): v for k, v in cache.items()}


def main():
    red_moves = {}
    for k in re.findall(r"#define MOVE_(\w+) ", (ROOT / "include/constants/move_id.h").read_text()):
        red_moves.setdefault(norm(k), "MOVE_" + k)
    mons = json.loads((ROOT / "data/monster/dx/monster_data.json").read_text())
    dex_of = {}
    for m in mons:
        dex_of.setdefault(m["name"][len("MonsterName"):], m["dexInternal"][0])
    red_sets = json.loads((ROOT / "data/monster/learnset/learnset_data.json").read_text())
    dx_path = ROOT / "data/monster/dx/learnset/learnset_data.json"
    dx_sets = json.loads(dx_path.read_text())

    def dex(entry):
        return dex_of.get(entry["pokemon"]) or dex_of.get(re.sub(r"[^A-Za-z]", "", entry["pokemon"]))

    wanted = [dex(e) for e in red_sets + dx_sets]
    api = fetch([d for d in wanted if d and 0 < d <= 809])
    skipped, kept_red, moved = Counter(), [], []
    classes = {norm(k): v for k, v in json.loads(MOVE_CLASSES.read_text()).items()}
    to_api = {v: k for k, v in RENAMED.items()}

    def damaging(move):
        key = norm(move[5:])
        return classes.get(to_api.get(key, key)) in ("physical", "special")

    def usum(entry):
        d = dex(entry)
        rows = api.get(d) if d else None
        if not rows:
            kept_red.append(entry["pokemon"])
            return entry["levelUpMoves"]
        best = {}
        for lvl, name in rows:
            key = RENAMED.get(norm(name), norm(name))
            if key not in red_moves:
                skipped[name] += 1
                continue
            mv = red_moves[key]
            best[mv] = min(best.get(mv, 999), max(lvl, 1))
        # DX: every Pokemon can attack from level 1, so the first damaging move comes at level 1
        if best and not any(lvl == 1 and damaging(m) for m, lvl in best.items()):
            first = min(((lvl, m) for m, lvl in best.items() if damaging(m)), default=None)
            if first:
                best[first[1]] = 1
                moved.append(f"{entry['pokemon']}: {first[1][5:]} {first[0]} -> 1")
        moves = sorted(({"move": m, "level": lvl} for m, lvl in best.items()), key=lambda x: (x["level"], x["move"]))
        return moves or entry["levelUpMoves"]

    out = []
    for e in red_sets:
        out.append(dict(e, levelUpMoves=usum(e)) if dex(e) else e)
    (ROOT / "data/monster/dx/red_learnset/learnset_data.json").write_text(json.dumps(out, indent=4) + "\n")
    for e in dx_sets:
        e["levelUpMoves"] = usum(e)
    dx_path.write_text(json.dumps(dx_sets, indent=4) + "\n")
    log = ["# Gen 7 (Ultra Sun/Ultra Moon) level-up moves Red has no move for, and how many learnsets drop each",
           "# (tools/dxtest/gen_dx_learnsets.py)"]
    log += [f"{n:4} {m}" for m, n in skipped.most_common()]
    log += ["", "# kept Red's learnset (no PokeAPI data): " + ", ".join(sorted(set(kept_red)))]
    log += ["", "# first damaging move moved to level 1 (DX: every Pokemon can attack from the start)"] + moved
    (HERE / "dx_learnset_log.txt").write_text("\n".join(log) + "\n")
    print(f"{len(out)} Red + {len(dx_sets)} DX learnsets; {sum(skipped.values())} moves skipped ({len(skipped)} kinds); "
          f"Red's kept for {len(set(kept_red))}")


if __name__ == "__main__":
    main()
