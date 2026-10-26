#!/usr/bin/env python3
"""Audit the DX newcomers' SpriteCollab sprites against their sprite VRAM budget (body size x 16 tiles):
per species, the tiles its biggest frame needs, how many frames the importer has to crop, and which of
Red's 13 animations the sprite lacks (those borrow Walk).

    sc_audit.py            print the table
    sc_audit.py --bodies   print only "<ident> <body size that fits every frame>" for species that need more"""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / "tools/dxtest"))
from sc_import import NUM_ANIMS, OAM_SHAPES, parse_anims  # noqa: E402
import gen_dx_species  # noqa: E402
from PIL import Image  # noqa: E402

SC = Path.home() / "pmd/spritecollab"


def needed_tiles(bw, bh):
    fits = [(w // 8) * (h // 8) for (w, h) in OAM_SHAPES if w >= bw and h >= bh]
    return min(fits) if fits else None  # None: bigger than any GBA sprite (64x64)


def audit(dex, body):
    sc_dir = SC / f"{dex:04d}"
    anims = parse_anims(sc_dir)
    by_name = {a["name"]: a for a in anims}
    have = {a["index"] for a in anims if a["index"] is not None}
    worst, cropped, frames, too_big = 0, 0, 0, 0
    for a in anims:
        if a["index"] is None:
            continue
        src = a
        while src["copy"]:
            src = by_name[src["copy"]]
        img = Image.open(sc_dir / f"{src['name']}-Anim.png").convert("RGBA")
        fw, fh = src["w"], src["h"]
        for row in range(img.height // fh):
            for col in range(img.width // fw):
                box = img.crop((col * fw, row * fh, (col + 1) * fw, (row + 1) * fh)).getbbox()
                if box is None:
                    continue
                frames += 1
                need = needed_tiles(box[2] - box[0], box[3] - box[1])
                if need is None:
                    too_big += 1
                    need = 64
                worst = max(worst, need)
                if need > body * 16:
                    cropped += 1
    missing = [i for i in range(NUM_ANIMS) if i not in have]
    return worst, cropped, frames, too_big, missing


def main():
    data = json.loads((ROOT / "data/monster/dx/monster_data.json").read_text())
    body_of = {m["name"]: m["bodySize"] for m in data}
    rows = []
    for s in gen_dx_species.SPECIES:
        body = s.get("body", body_of.get("MonsterName" + s["ident"], 1))
        worst, cropped, frames, too_big, missing = audit(s["dex"], body)
        rows.append((s["ident"], body, worst, cropped, frames, too_big, missing))
    if "--bodies" in sys.argv:
        for ident, body, worst, *_ in rows:
            if worst > body * 16:
                print(ident, min(4, -(-worst // 16)))
        return
    print(f"{'species':14} body budget worst cropped/frames >64x64 missing-anims")
    for ident, body, worst, cropped, frames, too_big, missing in rows:
        print(f"{ident:14} {body:4} {body * 16:6} {worst:5} {cropped:4}/{frames:<5} {too_big:6} {missing}")


if __name__ == "__main__":
    main()
