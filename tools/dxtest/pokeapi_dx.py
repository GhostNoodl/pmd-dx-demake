#!/usr/bin/env python3
"""Fetch (and cache) mainline data the DX species need from PokeAPI's GraphQL API:
English category ("genus") and level-up learnset (Diamond/Pearl; X/Y for Gen 6+)."""
import json
import urllib.request
from pathlib import Path

CACHE = Path(__file__).resolve().parent / "cache" / "pokeapi_dx.json"
URL = "https://beta.pokeapi.co/graphql/v1beta"
QUERY = """query($ids: [Int!]) {
  pokemon_v2_pokemonspecies(where: {id: {_in: $ids}}) {
    id name
    pokemon_v2_pokemonspeciesnames(where: {language_id: {_eq: 9}}) { name genus }
    pokemon_v2_pokemons(where: {is_default: {_eq: true}}) {
      pokemon_v2_pokemonmoves(where: {pokemon_v2_movelearnmethod: {name: {_eq: "level-up"}},
                                      version_group_id: {_in: [8, 15]}}) {
        level version_group_id pokemon_v2_move { name }
      }
    }
  }
}"""


def fetch(dex_ids):
    cache = json.loads(CACHE.read_text()) if CACHE.exists() else {}
    missing = [d for d in dex_ids if str(d) not in cache]
    if missing:
        body = json.dumps({"query": QUERY, "variables": {"ids": missing}}).encode()
        req = urllib.request.Request(URL, body, {"Content-Type": "application/json", "User-Agent": "pmd-dx-demake-tools/1.0"})
        data = json.loads(urllib.request.urlopen(req, timeout=120).read())["data"]["pokemon_v2_pokemonspecies"]
        for sp in data:
            names = sp["pokemon_v2_pokemonspeciesnames"][0]
            moves = sp["pokemon_v2_pokemons"][0]["pokemon_v2_pokemonmoves"]
            vg = 8 if any(m["version_group_id"] == 8 for m in moves) else 15
            learn = sorted({(m["level"], m["pokemon_v2_move"]["name"]) for m in moves if m["version_group_id"] == vg})
            cache[str(sp["id"])] = {"name": names["name"], "genus": names["genus"], "levelup": learn,
                                    "version_group": "diamond-pearl" if vg == 8 else "x-y"}
        CACHE.parent.mkdir(exist_ok=True)
        CACHE.write_text(json.dumps(cache, indent=1))
    return {d: cache[str(d)] for d in dex_ids}


SPEED_CACHE = CACHE.parent / "pokeapi_speed.json"
SPEED_QUERY = """query($ids: [Int!]) {
  pokemon_v2_pokemon(where: {id: {_in: $ids}}) {
    id pokemon_v2_pokemonstats(where: {pokemon_v2_stat: {name: {_eq: "speed"}}}) { base_stat }
  }
}"""


def fetch_speeds(dex_ids):
    """Mainline base Speed per national dex number (default form; ids 1-10000 are default forms)."""
    cache = {int(k): v for k, v in json.loads(SPEED_CACHE.read_text()).items()} if SPEED_CACHE.exists() else {}
    missing = sorted(set(dex_ids) - set(cache))
    if missing:
        body = json.dumps({"query": SPEED_QUERY, "variables": {"ids": missing}}).encode()
        req = urllib.request.Request(URL, body, {"Content-Type": "application/json", "User-Agent": "pmd-dx-demake-tools/1.0"})
        for p in json.loads(urllib.request.urlopen(req, timeout=120).read())["data"]["pokemon_v2_pokemon"]:
            cache[p["id"]] = p["pokemon_v2_pokemonstats"][0]["base_stat"]
        CACHE.parent.mkdir(exist_ok=True)
        SPEED_CACHE.write_text(json.dumps({str(k): v for k, v in sorted(cache.items())}))
    return {d: cache[d] for d in dex_ids}


FORM_CACHE = CACHE.parent / "pokeapi_forms.json"
FORM_QUERY = """query($names: [String!]) {
  pokemon_v2_pokemon(where: {name: {_in: $names}}) {
    name
    pokemon_v2_pokemontypes { slot pokemon_v2_type { name } }
    pokemon_v2_pokemonabilities { slot pokemon_v2_ability { name } }
    pokemon_v2_pokemonstats { base_stat pokemon_v2_stat { name } }
  }
}"""


def fetch_forms(names):
    """Types, abilities and base stats for named Pokemon/forms (e.g. "charizard-mega-x")."""
    cache = json.loads(FORM_CACHE.read_text()) if FORM_CACHE.exists() else {}
    missing = [n for n in names if n not in cache]
    if missing:
        body = json.dumps({"query": FORM_QUERY, "variables": {"names": missing}}).encode()
        req = urllib.request.Request(URL, body, {"Content-Type": "application/json", "User-Agent": "pmd-dx-demake-tools/1.0"})
        for p in json.loads(urllib.request.urlopen(req, timeout=120).read())["data"]["pokemon_v2_pokemon"]:
            cache[p["name"]] = {
                "types": [t["pokemon_v2_type"]["name"] for t in sorted(p["pokemon_v2_pokemontypes"], key=lambda t: t["slot"])],
                "abilities": [a["pokemon_v2_ability"]["name"] for a in sorted(p["pokemon_v2_pokemonabilities"], key=lambda a: a["slot"])],
                "stats": {s["pokemon_v2_stat"]["name"]: s["base_stat"] for s in p["pokemon_v2_pokemonstats"]},
            }
        CACHE.parent.mkdir(exist_ok=True)
        FORM_CACHE.write_text(json.dumps(cache, indent=1))
    return {n: cache.get(n) for n in names}


if __name__ == "__main__":
    import sys
    for d, v in fetch([int(x) for x in sys.argv[1:]] or [447]).items():
        print(d, v["name"], v["genus"], v["version_group"], v["levelup"][:6])
