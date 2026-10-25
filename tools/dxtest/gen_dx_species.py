#!/usr/bin/env python3
"""Generate everything the Rescue Team DX species need, for modern builds (DX_RULES).

Adding a species = add a line to SPECIES and run this (it also imports the sprite).

Sources:
- Explorers of Sky (pret/pmd-sky BALANCE/monster.md and m_level.bin; eos_md.py, eos_level.py):
  types, abilities, body size, base stats, weight/size and level-up growth. Explorers is
  PMD-scaled like Red: ability/type ids match Red's for every shared species and growth is on
  the same scale, so base stats and growth are taken together from Explorers.
- PokeAPI (pokeapi_dx.py): English category and level-up learnset (Diamond/Pearl; X/Y for
  Sylveon), keeping the moves that exist in Red.
- SpriteCollab (tools/spritecollab/sc_import.py): dungeon sprites; credits in CREDITS.md.
- Camp, TMs and recruit rate come from the species' Red relative.

Outputs (all only used under DX_RULES):
  include/constants/dx_species.h       MONSTER_* ids, DX_NEW_SPECIES_COUNT
  include/dx_species_gfx.h             externs for the sprite archives
  src/data/dx_species_files.h          monster file archive entries (ax###)
  data/monster/dx/monster_data.json    Red's table + DX changes + newcomers
  data/monster/dx/learnset/learnset_data.json
  data/monster/dx/monster_names_dx.s   names and categories
  data/monster/dx/learnset_ptrs_dx.s   learnset pointers for ids 424+
  data/monster/dx/level_table_dx.s     system archive entries for lvmp424+
  data/monster/dx/level_data_dx.s      lvmp files (SIRO + uncompressed AT3PN)
  src/data/ax/dx/*.h, graphics/ax/mon/<name>/   sprites
"""
import json
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import eos_level  # noqa: E402
import eos_md  # noqa: E402
import pokeapi_dx  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
FIRST_NEW_ID = 424

# Rescue Team DX's newcomers, in id order from 424 (Riolu and Lucario keep 424/425).
#   evolves_from: Red species it evolves from; baby_of: Red species that now evolves from it.
#   evo: (type, requirement). DX evolves with Evolution Crystals; Red has none, so stone /
#   trade / special evolutions become a level (stone 30, special 33, trade 50), friendship ones IQ 200.
SPECIES = [
    dict(dex=447, ident="Riolu", camp="MACHOP", tms="Machop"),
    dict(dex=448, ident="Lucario", evolves_from="RIOLU", evo=("EVOLVE_TYPE_IQ", 200), tms="Machoke", camp="MACHOP"),
    dict(dex=406, ident="Budew", baby_of="ROSELIA", evo=("EVOLVE_TYPE_IQ", 200)),
    dict(dex=407, ident="Roserade", evolves_from="ROSELIA", evo=("EVOLVE_TYPE_LEVEL", 30)),
    dict(dex=424, ident="Ambipom", evolves_from="AIPOM", evo=("EVOLVE_TYPE_LEVEL", 33)),
    dict(dex=429, ident="Mismagius", evolves_from="MISDREAVUS", evo=("EVOLVE_TYPE_LEVEL", 30)),
    dict(dex=430, ident="Honchkrow", evolves_from="MURKROW", evo=("EVOLVE_TYPE_LEVEL", 30)),
    dict(dex=433, ident="Chingling", baby_of="CHIMECHO", evo=("EVOLVE_TYPE_IQ", 200)),
    dict(dex=438, ident="Bonsly", baby_of="SUDOWOODO", evo=("EVOLVE_TYPE_LEVEL", 17)),
    dict(dex=439, ident="MimeJr", baby_of="MR_MIME", evo=("EVOLVE_TYPE_LEVEL", 18)),
    dict(dex=440, ident="Happiny", baby_of="CHANSEY", evo=("EVOLVE_TYPE_IQ", 200)),
    dict(dex=458, ident="Mantyke", baby_of="MANTINE", evo=("EVOLVE_TYPE_LEVEL", 20)),
    dict(dex=461, ident="Weavile", evolves_from="SNEASEL", evo=("EVOLVE_TYPE_LEVEL", 33)),
    dict(dex=462, ident="Magnezone", evolves_from="MAGNETON", evo=("EVOLVE_TYPE_LEVEL", 33)),
    dict(dex=463, ident="Lickilicky", evolves_from="LICKITUNG", evo=("EVOLVE_TYPE_LEVEL", 33)),
    dict(dex=464, ident="Rhyperior", evolves_from="RHYDON", evo=("EVOLVE_TYPE_LEVEL", 50)),
    dict(dex=465, ident="Tangrowth", evolves_from="TANGELA", evo=("EVOLVE_TYPE_LEVEL", 33)),
    dict(dex=466, ident="Electivire", evolves_from="ELECTABUZZ", evo=("EVOLVE_TYPE_LEVEL", 50)),
    dict(dex=467, ident="Magmortar", evolves_from="MAGMAR", evo=("EVOLVE_TYPE_LEVEL", 50)),
    dict(dex=468, ident="Togekiss", evolves_from="TOGETIC", evo=("EVOLVE_TYPE_LEVEL", 30)),
    dict(dex=469, ident="Yanmega", evolves_from="YANMA", evo=("EVOLVE_TYPE_LEVEL", 33)),
    dict(dex=470, ident="Leafeon", evolves_from="EEVEE", evo=("EVOLVE_TYPE_LEVEL", 30)),
    dict(dex=471, ident="Glaceon", evolves_from="EEVEE", evo=("EVOLVE_TYPE_LEVEL", 30)),
    dict(dex=472, ident="Gliscor", evolves_from="GLIGAR", evo=("EVOLVE_TYPE_LEVEL", 33)),
    dict(dex=473, ident="Mamoswine", evolves_from="PILOSWINE", evo=("EVOLVE_TYPE_LEVEL", 33)),
    dict(dex=474, ident="PorygonZ", evolves_from="PORYGON2", evo=("EVOLVE_TYPE_LEVEL", 50)),
    dict(dex=475, ident="Gallade", evolves_from="KIRLIA", evo=("EVOLVE_TYPE_LEVEL", 30)),
    dict(dex=476, ident="Probopass", evolves_from="NOSEPASS", evo=("EVOLVE_TYPE_LEVEL", 33)),
    dict(dex=477, ident="Dusknoir", evolves_from="DUSCLOPS", evo=("EVOLVE_TYPE_LEVEL", 50)),
    dict(dex=478, ident="Froslass", evolves_from="SNORUNT", evo=("EVOLVE_TYPE_LEVEL", 30)),
    # Sylveon (Gen 6) isn't in Explorers: Espeon's Explorers data with Sylveon's Fairy type
    dict(dex=700, ident="Sylveon", evolves_from="EEVEE", evo=("EVOLVE_TYPE_IQ", 200), eos_dex=196,
         types=["TYPE_FAIRY"], abilities=["ABILITY_CUTE_CHARM"]),
]

# Mystery House homes (DUNGEON_* without the prefix). DX's real Mystery House locations aren't
# documented, so these are picked by type and level: base forms in story dungeons, evolved forms in
# later ones. A newcomer only appears in a Mystery House on its home dungeons (dx_rules.c).
HOMES = {
    "Riolu": ["MT_STEEL"], "Lucario": ["MT_FARAWAY"], "Munchlax": ["HOWLING_FOREST"],
    "Budew": ["SINISTER_WOODS"], "Roserade": ["PURITY_FOREST"], "Ambipom": ["NORTHERN_RANGE"],
    "Mismagius": ["DARKNIGHT_RELIC"], "Honchkrow": ["WYVERN_HILL"], "Chingling": ["ODDITY_CAVE"],
    "Bonsly": ["GREAT_CANYON"], "MimeJr": ["UPROAR_FOREST"], "Happiny": ["LAPIS_CAVE"],
    "Mantyke": ["STORMY_SEA"], "Weavile": ["MT_FREEZE"], "Magnezone": ["LIGHTNING_FIELD"],
    "Lickilicky": ["WISH_CAVE"], "Rhyperior": ["DESERT_REGION"], "Tangrowth": ["NORTHWIND_FIELD"],
    "Electivire": ["LIGHTNING_FIELD"], "Magmortar": ["FIERY_FIELD"], "Togekiss": ["REMAINS_ISLAND"],
    "Yanmega": ["SOUTHERN_CAVERN"], "Leafeon": ["PURITY_FOREST"], "Glaceon": ["NORTHERN_RANGE"],
    "Gliscor": ["WYVERN_HILL"], "Mamoswine": ["SILVER_TRENCH"], "PorygonZ": ["BURIED_RELIC"],
    "Gallade": ["WESTERN_CAVE"], "Probopass": ["DESERT_REGION"], "Dusknoir": ["DARKNIGHT_RELIC"],
    "Froslass": ["MT_FREEZE"], "Sylveon": ["JOYOUS_TOWER"],
}
# Red relatives that can't be recruited (-999 etc.) give their evolutions this rate instead (5%)
FALLBACK_RECRUIT_RATE = 50

# Mega Evolution / Primal Reversion forms (Floor 6): PokeAPI name, base species, SpriteCollab form.
# Each gets a species id after the newcomers; an Empowered base (Empowerment Seed) takes on its
# sprite, types and abilities for the floor (dx_rules.c). Megas SpriteCollab has no sprite for yet
# use the base sprite. Charizard and Mewtwo have two: a team member picks one (dx_rules.c).
MEGAS = [
    ("venusaur-mega", "VENUSAUR", "0003/0001"), ("charizard-mega-x", "CHARIZARD", "0006/0001"),
    ("charizard-mega-y", "CHARIZARD", "0006/0002"), ("blastoise-mega", "BLASTOISE", "0009/0001"),
    ("beedrill-mega", "BEEDRILL", "0015/0001"), ("pidgeot-mega", "PIDGEOT", "0018/0001"),
    ("alakazam-mega", "ALAKAZAM", "0065/0001"), ("slowbro-mega", "SLOWBRO", "0080/0002"),
    ("gengar-mega", "GENGAR", "0094/0001"), ("kangaskhan-mega", "KANGASKHAN", "0115/0001"),
    ("pinsir-mega", "PINSIR", "0127/0001"), ("gyarados-mega", "GYARADOS", "0130/0001"),
    ("aerodactyl-mega", "AERODACTYL", "0142/0001"), ("mewtwo-mega-y", "MEWTWO", "0150/0002"),
    ("mewtwo-mega-x", "MEWTWO", "0150/0001"), ("ampharos-mega", "AMPHAROS", "0181/0001"),
    ("steelix-mega", "STEELIX", "0208/0001"), ("scizor-mega", "SCIZOR", "0212/0001"),
    ("heracross-mega", "HERACROSS", "0214/0001"), ("houndoom-mega", "HOUNDOOM", "0229/0001"),
    ("tyranitar-mega", "TYRANITAR", "0248/0001"), ("sceptile-mega", "SCEPTILE", "0254/0001"),
    ("blaziken-mega", "BLAZIKEN", "0257/0001"), ("swampert-mega", "SWAMPERT", "0260/0001"),
    ("gardevoir-mega", "GARDEVOIR", "0282/0001"), ("sableye-mega", "SABLEYE", "0302/0001"),
    ("mawile-mega", "MAWILE", "0303/0001"), ("aggron-mega", "AGGRON", "0306/0001"),
    ("medicham-mega", "MEDICHAM", "0308/0001"), ("manectric-mega", "MANECTRIC", "0310/0001"),
    ("sharpedo-mega", "SHARPEDO", "0319/0001"), ("camerupt-mega", "CAMERUPT", "0323/0001"),
    ("altaria-mega", "ALTARIA", "0334/0001"), ("banette-mega", "BANETTE", "0354/0001"),
    ("absol-mega", "ABSOL", "0359/0001"), ("glalie-mega", "GLALIE", "0362/0001"),
    ("salamence-mega", "SALAMENCE", "0373/0001"), ("metagross-mega", "METAGROSS", "0376/0001"),
    ("latias-mega", "LATIAS", "0380/0001"), ("latios-mega", "LATIOS", "0381/0001"),
    ("rayquaza-mega", "RAYQUAZA", "0384/0001"), ("lucario-mega", "LUCARIO", "0448/0001"),
    ("gallade-mega", "GALLADE", "0475/0001"),
    ("kyogre-primal", "KYOGRE", "0382/0001"), ("groudon-primal", "GROUDON", "0383/0001"),
]

# DX evolves with Evo Crystals (Red's unused G Machine 6, gen_dx_items.py); Eevee's forms take two
EVO_CRYSTAL = "ITEM_G_MACHINE_6"
EVO_REQ_TWO_CRYSTALS = 14  # additionalRequirement handled in pokemon_evolution.c

# Munchlax already exists in Red (#420) with placeholder data
MUNCHLAX = dict(dex=446, ident="Munchlax", camp="SNORLAX", tms="Snorlax", recruit=50)

TYPE_NAMES = ["TYPE_NONE", "TYPE_NORMAL", "TYPE_FIRE", "TYPE_WATER", "TYPE_GRASS", "TYPE_ELECTRIC", "TYPE_ICE",
              "TYPE_FIGHTING", "TYPE_POISON", "TYPE_GROUND", "TYPE_FLYING", "TYPE_PSYCHIC", "TYPE_BUG", "TYPE_ROCK",
              "TYPE_GHOST", "TYPE_DRAGON", "TYPE_DARK", "TYPE_STEEL"]


def norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


def main():
    consts = (ROOT / "include/constants/monster.h").read_text()
    red_ids = {k: int(v) for k, v in re.findall(r"#define MONSTER_(\w+) (\d+)\b", consts)}
    abilities = {int(v, 0): k for k, v in re.findall(r"#define (ABILITY_\w+) (0x[0-9A-Fa-f]+|\d+)",
                                                    (ROOT / "include/constants/ability.h").read_text())}
    red_moves = {norm(k): "MOVE_" + k for k in re.findall(r"#define MOVE_(\w+) ", (ROOT / "include/constants/move_id.h").read_text())}
    md = eos_md.by_dex(eos_md.load())
    abilities_by_name = {k[8:].lower().replace("_", "-"): k for k in abilities.values()}
    red = json.loads((ROOT / "data/monster/monster_data.json").read_text())
    assert len(red) == 424, len(red)
    red_sets = {norm(e["pokemon"]): e for e in json.loads((ROOT / "data/monster/learnset/learnset_data.json").read_text())}
    api = pokeapi_dx.fetch([s["dex"] for s in SPECIES] + [MUNCHLAX["dex"]])

    for i, s in enumerate(SPECIES):
        s["id"] = FIRST_NEW_ID + i
    ident_ids = {s["ident"].upper(): s["id"] for s in SPECIES}

    def relative(s):
        return s.get("camp") or s.get("evolves_from") or s.get("baby_of")

    def red_entry(const):
        return red[red_ids[const]]

    def learnset(s):
        moves, dropped = [], []
        for lvl, m in api[s["dex"]]["levelup"]:
            if norm(m) in red_moves:
                moves.append({"move": red_moves[norm(m)], "level": max(lvl, 1)})
            else:
                dropped.append(m)
        if not moves:
            moves = [{"move": "MOVE_TACKLE", "level": 1}]
        seen, out = set(), []
        for mv in sorted(moves, key=lambda x: x["level"]):
            if mv["move"] not in seen:
                seen.add(mv["move"])
                out.append(mv)
        tms_from = s.get("tms") or norm(red_entry(relative(s))["name"].replace("MonsterName", ""))
        return {"pokemon": s["ident"], "levelUpMoves": out, "HMTMMoves": red_sets[norm(tms_from)]["HMTMMoves"]}, dropped

    def eos_fields(s):
        e = md[s.get("eos_dex", s["dex"])]
        types = s.get("types") or ([TYPE_NAMES[e["type1"]]] + ([TYPE_NAMES[e["type2"]]] if e["type2"] else []))
        abil = s.get("abilities") or [abilities[a] for a in (e["ability1"], e["ability2"]) if a and a in abilities]
        if not abil:  # only Gen 4 abilities: use the Red relative's
            abil = [a for a in red_entry(relative(s))["abilities"] if a != "ABILITY_UNKNOWN"] or ["ABILITY_UNKNOWN"]
            s["ability_note"] = f"Gen 4 abilities only; {relative(s)}'s {abil}"
        return e, {
            "bodySize": e["body_size"], "types": types,
            "abilities": abil,
            "baseHP": e["base_hp"], "baseAtkSpAtk": [e["atk"], e["spatk"]], "baseDefSpDef": [e["def"], e["spdef"]],
            "expYield": round(0.852 * e["exp_yield"] + 13.9),  # Red vs Explorers exp yield fit over shared species
            "weight": e["weight"], "size": e["size"], "chanceAsleep": max(e["asleep"], 0),
            "dexInternal": [s["dex"], s["dex"]],
        }

    learnsets, levels, names, log = [], [], [], []

    # Munchlax: real data for Red's cameo species
    e, f = eos_fields(MUNCHLAX)
    red[420].update(f)
    red[420].update({"category": "MonsterCategoryBigEater", "friendArea": red_entry(MUNCHLAX["camp"])["friendArea"],
                     "recruitRate": MUNCHLAX["recruit"]})
    ls, dropped = learnset(MUNCHLAX)
    learnsets.append(ls)
    levels.append((420, eos_level.levels(e["index"])[1]))
    names.append(("MonsterCategoryBigEater", "Big Eater"))
    log.append(f"Munchlax #420: Explorers #446; moves not in Red: {dropped}")

    for s in SPECIES:
        e, f = eos_fields(s)
        rel = red_entry(relative(s)) if relative(s) in red_ids else red[ident_ids[relative(s)]]
        display = api[s["dex"]]["name"]
        category = api[s["dex"]]["genus"].replace(" Pokémon", "")
        cat_sym = "MonsterCategory" + re.sub(r"[^A-Za-z0-9]", "", category)
        # sprite (palette is picked by the importer)
        header = ROOT / f"src/data/ax/dx/{s['ident'].lower()}.h"
        sc_dir = Path.home() / "pmd/spritecollab" / f"{s['dex']:04d}"
        if not header.exists():
            r = subprocess.run([sys.executable, "-I", str(ROOT / "tools/spritecollab/sc_import.py"), str(sc_dir),
                                s["ident"].lower(), s["ident"], "--body-size", str(s.get("body", max(e["body_size"], 1))),
                                "--fetch", f"{s['dex']:04d}"], capture_output=True, text=True, cwd=ROOT)
            if r.returncode:
                sys.exit(f"{s['ident']}: sprite import failed\n{r.stderr}")
            (ROOT / f"src/data/ax/{s['ident'].lower()}.h").replace(header)
            print(r.stdout.strip())
        pal = int(re.search(r"OAM3\(0, 3, (\d+)\)", header.read_text()).group(1))
        # portraits (SpriteCollab), as a Red kao file; the bitmask says which emotions exist
        kao = ROOT / f"data/kao/dx/{s['ident'].lower()}.inc"
        r = subprocess.run([sys.executable, "-I", str(ROOT / "tools/spritecollab/sc_portrait.py"), str(kao),
                            s["ident"], f"{s['dex']:04d}"], capture_output=True, text=True, cwd=ROOT)
        if r.returncode:
            sys.exit(f"{s['ident']}: portrait import failed\n{r.stderr}")
        s["portraits"] = r.stdout.strip()
        entry = json.loads(json.dumps(rel))  # Red relative's entry for the fields Explorers doesn't give
        entry.update(f)
        if "body" in s:
            entry["bodySize"] = s["body"]
        entry.update({
            "name": f"MonsterName{s['ident']}", "category": cat_sym, "overworldPalette": pal,
            "friendArea": rel["friendArea"],
            "recruitRate": rel["recruitRate"] if rel["recruitRate"] > 0 else FALLBACK_RECRUIT_RATE,
            "dialogueSprites": s["portraits"],
            "alphabetParent": [s["id"], s["id"]],
        })
        entry.pop("preEvolution", None)
        entry.pop("evolutionRequirements", None)
        if s.get("evolves_from"):
            evolves_from = s["evolves_from"]
            entry["preEvolution"] = {"evolveFrom": f"MONSTER_{evolves_from}", "evolveType": s["evo"][0]}
            entry["evolutionRequirements"] = {"mainRequirement": s["evo"][1]}
            if s["evo"][0] == "EVOLVE_TYPE_LEVEL" or evolves_from == "EEVEE":
                # stone / trade / special evolutions: an Evo Crystal, as in DX (Eevee forms take two)
                entry["preEvolution"]["evolveType"] = "EVOLVE_TYPE_ITEM"
                entry["evolutionRequirements"] = {"mainRequirement": EVO_CRYSTAL}
                if evolves_from == "EEVEE":
                    entry["evolutionRequirements"]["additionalRequirement"] = EVO_REQ_TWO_CRYSTALS
        if s.get("baby_of"):
            adult = red_entry(s["baby_of"])
            adult["preEvolution"] = {"evolveFrom": f"MONSTER_{s['ident'].upper()}", "evolveType": s["evo"][0]}
            adult["evolutionRequirements"] = {"mainRequirement": s["evo"][1]}
        assert s["id"] == len(red)
        red.append(entry)
        ls, dropped = learnset(s)
        learnsets.append(ls)
        levels.append((s["id"], eos_level.levels(e["index"])[1]))
        names += [(f"MonsterName{s['ident']}", display), (cat_sym, category)]
        log.append(f"{display} #{s['id']}: Explorers #{s.get('eos_dex', s['dex'])}, {entry['types']}, {entry['abilities']}, "
                   f"HP {e['base_hp']} {e['atk']}/{e['spatk']}/{e['def']}/{e['spdef']}, body {e['body_size']}, "
                   f"palette {pal}, camp of {relative(s)}; moves not in Red: {dropped}" + (f"; {s['ability_note']}" if s.get("ability_note") else ""))

    # DX: Red's item, trade and ribbon evolutions all use Evo Crystals (Eevee's forms take two)
    for e in red[1:424]:
        pre, req = e.get("preEvolution"), e.get("evolutionRequirements") or {}
        if not pre or not pre.get("evolveType"):
            continue
        extra = req.get("additionalRequirement", 0)
        if pre["evolveType"] in ("EVOLVE_TYPE_ITEM", "EVOLVE_TYPE_LINK_CABLE") or extra in (8, 9, 10):
            pre["evolveType"] = "EVOLVE_TYPE_ITEM"
            e["evolutionRequirements"] = {"mainRequirement": EVO_CRYSTAL}
            if pre["evolveFrom"] == "MONSTER_EEVEE":
                e["evolutionRequirements"]["additionalRequirement"] = EVO_REQ_TWO_CRYSTALS
            log.append(f"{e['name'][11:]}: evolves from {pre['evolveFrom'][8:].title()} with "
                       + ("two Evo Crystals" if pre["evolveFrom"] == "MONSTER_EEVEE" else "an Evo Crystal"))

    # --- Mega / Primal forms ---
    forms = pokeapi_dx.fetch_forms([m[0] for m in MEGAS] + sorted({m[0].split("-")[0] for m in MEGAS}))
    gfx_text = (ROOT / "include/monster_gfx.h").read_text()
    megas, used_bases = [], set()
    first_mega = FIRST_NEW_ID + len(SPECIES)
    for i, (api_name, base, sc_path) in enumerate(MEGAS):
        kind = "Primal" if api_name.endswith("primal") else "Mega"
        suffix = api_name.split("-mega-")[1].upper() if "-mega-" in api_name else ""
        ident = f"{kind}{base.title().replace('_', '')}{suffix}"
        display = f"{kind} {base.title()}" + (f" {suffix}" if suffix else "")
        base_id = red_ids.get(base, ident_ids.get(base))
        base_entry = red[base_id]
        form, base_form = forms[api_name], forms[api_name.split("-")[0]]
        m = dict(ident=ident, id=first_mega + i, base=base, display=display, kind=kind, sc=sc_path,
                 dex=int(sc_path.split("/")[0]))
        # sprite: SpriteCollab's, sized like the base (the transformed Pokemon keeps its VRAM slot)
        header = ROOT / f"src/data/ax/dx/{ident.lower()}.h"
        if not header.exists() and not (ROOT / f"src/data/ax/dx/{ident.lower()}.nosprite").exists():
            sc_dir = Path.home() / "pmd/spritecollab" / sc_path.replace("/", "_")
            r = subprocess.run([sys.executable, "-I", str(ROOT / "tools/spritecollab/sc_import.py"), str(sc_dir),
                                ident.lower(), ident, "--body-size", str(base_entry["bodySize"]), "--fetch", sc_path],
                               capture_output=True, text=True, cwd=ROOT)
            if r.returncode == 0:
                (ROOT / f"src/data/ax/{ident.lower()}.h").replace(header)
                print(r.stdout.strip())
            else:  # remember, so later runs don't retry the download
                (ROOT / f"src/data/ax/dx/{ident.lower()}.nosprite").write_text(r.stderr[-400:])
        m["gfx"] = f"gAx{ident}" if header.exists() else None
        if m["gfx"] is None:
            base_gfx = f"gAx{base_entry['name'][11:]}" if base_id < FIRST_NEW_ID else f"gAx{base.title()}"
            assert f"extern const SiroArchive {base_gfx};" in gfx_text or base_id >= FIRST_NEW_ID, base_gfx
            m["base_gfx"] = base_gfx
        pal = int(re.search(r"OAM3\(0, 3, (\d+)\)", header.read_text()).group(1)) if header.exists() else base_entry["overworldPalette"]
        kao = ROOT / f"data/kao/dx/{ident.lower()}.inc"
        r = subprocess.run([sys.executable, "-I", str(ROOT / "tools/spritecollab/sc_portrait.py"), str(kao), ident, sc_path],
                           capture_output=True, text=True, cwd=ROOT)
        m["portraits"] = r.stdout.strip() if r.returncode == 0 else "0b0000000000000000"
        types = [f"TYPE_{t.upper()}" for t in form["types"]]
        abil = [abilities_by_name[a] for a in form["abilities"] if a in abilities_by_name][:2] or base_entry["abilities"]
        entry = json.loads(json.dumps(base_entry))
        entry.pop("preEvolution", None)
        entry.pop("evolutionRequirements", None)
        entry.update({"name": f"MonsterName{ident}", "types": types, "abilities": abil, "overworldPalette": pal,
                      "recruitRate": -999, "dialogueSprites": m["portraits"], "alphabetParent": [m["id"], m["id"]]})
        assert m["id"] == len(red)
        red.append(entry)
        base_ls = next((x for x in learnsets if x["pokemon"].upper() == base), None) or red_sets[norm(base_entry["name"][11:])]
        learnsets.append({"pokemon": ident, "levelUpMoves": base_ls["levelUpMoves"], "HMTMMoves": base_ls["HMTMMoves"]})
        names.append((f"MonsterName{ident}", display))
        # the Mega's edge over its base, as extra stat stages (+1 per 25% more, at most +3)
        def stage(stat):
            return max(0, min(3, round((form["stats"][stat] / base_form["stats"][stat] - 1) * 4)))
        m["stages"] = [stage("attack"), stage("special-attack"), stage("defense"), stage("special-defense")]
        m["speed"] = form["stats"]["speed"]
        m["faster"] = form["stats"]["speed"] >= base_form["stats"]["speed"] + 20
        m["transforms"] = base not in used_bases
        used_bases.add(base)
        megas.append(m)
        log.append(f"{display} #{m['id']}: {types}, {abil}, stages {m['stages']}" + (" +speed" if m["faster"] else "")
                   + ("" if m["gfx"] else " [no SpriteCollab sprite: base sprite]")
                   + ("" if m["transforms"] else " [not used: base has an earlier form]"))

    # --- write everything ---
    n = len(SPECIES)
    w = lambda p, t: (ROOT / p).write_text(t)
    hdr = "// Generated by tools/dxtest/gen_dx_species.py; don't edit by hand.\n"
    w("include/constants/dx_species.h", hdr + "#ifndef GUARD_CONSTANTS_DX_SPECIES_H\n#define GUARD_CONSTANTS_DX_SPECIES_H\n\n"
      + "".join(f"#define MONSTER_{s['ident'].upper()} {s['id']}\n" for s in SPECIES)
      + f"\n#define DX_FIRST_NEW_SPECIES {FIRST_NEW_ID}\n#define DX_NEW_SPECIES_COUNT {n}\n"
      + "".join(f"#define MONSTER_{m['ident'].upper()} {m['id']}\n" for m in megas)
      + f"#define DX_FIRST_MEGA {first_mega}\n#define DX_MEGA_COUNT {len(megas)}\n"
      + f"#define DX_NEW_PORTRAIT_COUNT {sum(x['portraits'] != '0b0000000000000000' for x in SPECIES + megas)}\n\n#endif\n")
    kaos = [x for x in SPECIES + megas if x["portraits"] != "0b0000000000000000"]
    w("include/dx_species_gfx.h", hdr + "".join(f"extern const SiroArchive gAx{s['ident']};\n" for s in SPECIES)
      + "".join(f"extern const SiroArchive {m['gfx']};\n" for m in megas if m["gfx"])
      + "".join(f"extern const u8 gKao{s['ident']}[];\n" for s in kaos))
    w("src/data/dx_kao_files.h", hdr + "".join(
        f"    [496 + DX_NEW_SPECIES_FILES + {i}] = {{ \"kao{s['id']:03d}\", gKao{s['ident']},}},\n" for i, s in enumerate(kaos)))
    w("data/kao/dx/all.inc", "@ Generated by tools/dxtest/gen_dx_species.py: DX newcomer and Mega portraits\n"
      + "".join(f'.include "data/kao/dx/{s["ident"].lower()}.inc"\n' for s in kaos))
    w("src/data/dx_species_files.h", hdr + "".join(f"    [MONSTER_{s['ident'].upper()} - 1] = {{ \"ax{s['id']:03d}\", &gAx{s['ident']},}},\n" for s in SPECIES)
      + "".join(f"    [MONSTER_{m['ident'].upper()} - 1] = {{ \"ax{m['id']:03d}\", &{m['gfx'] or m['base_gfx']},}},\n" for m in megas))
    # Mega forms an Empowered Pokemon takes (dx_rules.c)
    w("src/data/dx_megas.h", hdr + "// base species, Mega/Primal form, extra Atk/SpAtk/Def/SpDef stages, faster, Primal\n"
      + "static const struct DxMegaForm sDxMegaForms[] = {\n"
      + "".join(f"    {{ MONSTER_{m['base']}, MONSTER_{m['ident'].upper()}, {{ {', '.join(map(str, m['stages']))} }}, "
                f"{'TRUE' if m['faster'] else 'FALSE'}, {'TRUE' if m['kind'] == 'Primal' else 'FALSE'} }},\n"
                for m in megas) + "};\n")
    out = ROOT / "data/monster/dx"
    (out / "learnset").mkdir(parents=True, exist_ok=True)
    (out / "monster_data.json").write_text(json.dumps(red, indent=4) + "\n")
    (out / "learnset/learnset_data.json").write_text(json.dumps(learnsets, indent=4) + "\n")
    seen = set(re.findall(r"^(MonsterCategory\w+):", (ROOT / "data/monster/monster_names.s").read_text(), re.M))
    txt = ["@ Generated by tools/dxtest/gen_dx_species.py: Rescue Team DX species text"]
    for sym, text in names:
        if sym not in seen:
            seen.add(sym)
            txt += [f".global {sym}", f"{sym}:", f'.string "{text}\\0"', ".align 2,0", ""]
    (out / "monster_names_dx.s").write_text("\n".join(txt) + "\n")
    (out / "learnset_ptrs_dx.s").write_text("@ Generated: learnsets for ids 424+\n" + "".join(
        f".4byte LevelUpMoves{s['ident']}\n.4byte HMTMMoves{s['ident']}\n" for s in SPECIES + megas))
    (out / "level_table_dx.s").write_text("@ Generated: lvmp files for ids 424+ (the table is name-sorted)\n" + "".join(
        f".4byte gDxLevelName{s['id']}\n.4byte gDxLevel{s['id']}\n" for s in SPECIES))
    s_ = ["@ Generated by tools/dxtest/gen_dx_species.py: level-up data from Explorers of Sky (m_level.bin)",
          "@ as Red lvmp files (SIRO header + uncompressed AT3PN stream of 12-byte LevelData records).", ""]
    for sid, raw in levels:
        s_ += [f".global gDxLevelName{sid}", f"gDxLevelName{sid}:", f'.string "lvmp{sid:03d}\\0"', ".align 2, 0",
               f".global gDxLevel{sid}", f"gDxLevel{sid}:", '.ascii "SIRO"', f".4byte gDxLevelAT{sid}", ".4byte 0, 0",
               f"gDxLevelAT{sid}:", '.ascii "AT3PN"', f".2byte {len(raw)}"]
        for i in range(0, len(raw), 16):
            s_.append(".byte " + ", ".join(f"0x{b:02x}" for b in raw[i:i + 16]))
        s_ += [".align 2, 0", ""]
    (out / "level_data_dx.s").write_text("\n".join(s_) + "\n")
    # mainline base Speed per species id (Red has no Speed stat): speed-based double moves
    dex_of = [e["dexInternal"][0] if 1 <= i <= 420 else 0 for i, e in enumerate(red)]
    for s in SPECIES:
        dex_of[s["id"]] = s["dex"]
    dex_of[420] = MUNCHLAX["dex"]
    speeds = pokeapi_dx.fetch_speeds(sorted({d for d in dex_of if d}))
    rows = [speeds.get(d, 0) if d else 0 for d in dex_of]
    for m in megas:
        rows[m["id"]] = m["speed"]
    w("src/data/dx_base_speed.h", hdr + "// mainline base Speed by species id (PokeAPI); 0 = not a real species\n"
      + "static const u8 sDxBaseSpeed[MONSTER_MAX] = {\n"
      + "".join("    " + ", ".join(str(v) for v in rows[i:i + 16]) + ",\n" for i in range(0, len(rows), 16)) + "};\n")
    # dungeon Talk lines: Red's talk files stop at its own species, so newcomers borrow their relative's
    def talk_as(s):
        r = relative(s)
        while r in ident_ids:  # Lucario -> Riolu -> Machop
            r = relative(next(x for x in SPECIES if x["ident"].upper() == r))
        return r
    w("src/data/dx_talk_as.h", hdr + "// Red species whose dungeon Talk lines each newcomer uses\n"
      + "static const s16 sDxTalkAs[DX_NEW_SPECIES_COUNT] = {\n"
      + "".join(f"    MONSTER_{talk_as(s)}, // {s['ident']}\n" for s in SPECIES) + "};\n")
    # Mystery House homes
    rows = [(d, "MUNCHLAX" if n == "Munchlax" else n.upper()) for n, ds in HOMES.items() for d in ds]
    w("src/data/dx_mystery_houses.h", hdr + "// Mystery House homes (gen_dx_species.py HOMES): dungeon, newcomer\n"
      + "static const struct DxMysteryHome sDxMysteryHomes[] = {\n"
      + "".join(f"    {{ DUNGEON_{d}, MONSTER_{m} }},\n" for d, m in rows) + "};\n")
    # credits manifest
    w("tools/spritecollab/imported.txt", "# <SpriteCollab dex> <name in this repo> [sprite|portrait|both]\n"
      + "".join(f"{s['dex']:04d} {s['ident'].lower()} both\n" for s in SPECIES)
      + "".join(f"{m['sc']} {m['ident'].lower()} " + ("both" if m["gfx"] else "portrait") + "\n" for m in megas))
    (ROOT / "tools/dxtest/dx_species_log.txt").write_text("\n".join(log) + "\n")
    print("\n".join(log))


if __name__ == "__main__":
    main()
