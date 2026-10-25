#!/usr/bin/env python3
"""Import a PMDCollab SpriteCollab sprite into pmd-red's dungeon sprite (ax) format.

    sc_import.py <spritecollab dir> <name> <Prefix> --body-size N [--palette P] [--fetch DEX]

  <spritecollab dir>  folder with AnimData.xml and <Anim>-Anim/-Offsets/-Shadow.png
                      (--fetch DEX downloads them from github.com/PMDCollab/SpriteCollab first)
  <name>              output names: src/data/ax/<name>.h, graphics/ax/mon/<name>/sprite_N.png
  <Prefix>            C symbol prefix, e.g. Pikachu -> gAxPikachu, sPikachuPose1, ...
  --body-size N       the species' body size; each frame must fit N*16 tiles of sprite VRAM
  --palette P         one of Red's 13 shared monster palettes (graphics/ax/pal/P.pal);
                      default: the one that matches the sprite's colours best

Layout notes (from Red's data):
- Red's 13 animation tables are SpriteCollab Index 0..12, each with 8 directions
  (rows: down, down-right, right, up-right, up, up-left, left, down-left).
- The frame origin is the centre of the shadow; Offsets.png markers are
  black=head, red/blue=hands, green=centre -> PositionSets {head, red, blue, green}.
- One OAM per pose; identical (or mirrored) frames share tile data; empty tile
  runs are stored as {NULL, bytes} entries like Red does.
"""
import argparse
import re
import sys
import urllib.request
from pathlib import Path
from xml.etree import ElementTree

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
NUM_ANIMS = 13
NUM_DIRS = 8
SC_URL = "https://raw.githubusercontent.com/PMDCollab/SpriteCollab/master/sprite/{dex}/{file}"

# (width, height) -> (shape, size) for GBA OAM
OAM_SHAPES = {
    (8, 8): ("ST_OAM_SQUARE", 0), (16, 16): ("ST_OAM_SQUARE", 1), (32, 32): ("ST_OAM_SQUARE", 2), (64, 64): ("ST_OAM_SQUARE", 3),
    (16, 8): ("ST_OAM_H_RECTANGLE", 0), (32, 8): ("ST_OAM_H_RECTANGLE", 1), (32, 16): ("ST_OAM_H_RECTANGLE", 2), (64, 32): ("ST_OAM_H_RECTANGLE", 3),
    (8, 16): ("ST_OAM_V_RECTANGLE", 0), (8, 32): ("ST_OAM_V_RECTANGLE", 1), (16, 32): ("ST_OAM_V_RECTANGLE", 2), (32, 64): ("ST_OAM_V_RECTANGLE", 3),
}


def read_pal(path):
    lines = Path(path).read_text().split()
    n = int(lines[2])
    vals = list(map(int, lines[3:3 + 3 * n]))
    return [tuple(vals[i:i + 3]) for i in range(0, 3 * n, 3)]


def nearest(pal, rgb):
    return min(range(1, len(pal)), key=lambda i: sum((a - b) ** 2 for a, b in zip(pal[i], rgb)))


def fetch(dex, out):
    out.mkdir(parents=True, exist_ok=True)
    def get(name):
        url = SC_URL.format(dex=dex, file=name)
        data = urllib.request.urlopen(url, timeout=60).read()
        (out / name).write_bytes(data)
    get("AnimData.xml")
    for a in parse_anims(out):
        if a["copy"]:
            continue
        for kind in ("Anim", "Offsets", "Shadow"):
            get(f"{a['name']}-{kind}.png")


def parse_anims(sc_dir):
    root = ElementTree.parse(sc_dir / "AnimData.xml").getroot()
    anims = []
    for a in root.find("Anims"):
        idx = a.find("Index")
        anims.append({
            "name": a.find("Name").text,
            "index": int(idx.text) if idx is not None else None,
            "copy": a.find("CopyOf").text if a.find("CopyOf") is not None else None,
            "w": int(a.find("FrameWidth").text) if a.find("FrameWidth") is not None else None,
            "h": int(a.find("FrameHeight").text) if a.find("FrameHeight") is not None else None,
            "durations": [int(d.text) for d in a.find("Durations")] if a.find("Durations") is not None else [],
        })
    return anims


def pick_shape(bw, bh, max_tiles):
    fits = [(w * h, w, h) for (w, h) in OAM_SHAPES if w >= bw and h >= bh and (w // 8) * (h // 8) <= max_tiles]
    if fits:
        return min(fits)[1:]
    # too big for the VRAM budget: the largest shape that fits, frame gets cropped
    best = max(((w // 8) * (h // 8), -abs(w / h - bw / max(bh, 1)), w, h) for (w, h) in OAM_SHAPES if (w // 8) * (h // 8) <= max_tiles)
    return best[2], best[3]


def tiles_of(img, w, h):
    """4bpp tiles (as tuples of 64 indices) of an indexed image region, 1D OAM order."""
    px = img.load()
    out = []
    for ty in range(h // 8):
        for tx in range(w // 8):
            out.append(tuple(px[tx * 8 + x, ty * 8 + y] for y in range(8) for x in range(8)))
    return out


def hflip_tiles(tiles, w, h):
    tw = w // 8
    flipped = []
    for ty in range(h // 8):
        for tx in range(tw):
            t = tiles[ty * tw + (tw - 1 - tx)]
            flipped.append(tuple(t[y * 8 + (7 - x)] for y in range(8) for x in range(8)))
    return flipped


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sc_dir")
    ap.add_argument("name")
    ap.add_argument("prefix")
    ap.add_argument("--body-size", type=int, required=True)
    ap.add_argument("--palette", type=int)
    ap.add_argument("--fetch")
    args = ap.parse_args()

    sc_dir = Path(args.sc_dir)
    if args.fetch:
        fetch(args.fetch, sc_dir)
    anims = parse_anims(sc_dir)
    by_name = {a["name"]: a for a in anims}
    by_index = {a["index"]: a for a in anims if a["index"] is not None}
    missing = [i for i in range(NUM_ANIMS) if i not in by_index]
    if missing:
        # a partial sprite (common for Megas): missing animations borrow Walk (index 0), or else the
        # first one the sprite has, so the form still shows its own look
        if not by_index:
            sys.exit("SpriteCollab sprite has no usable animations")
        stand_in = by_index.get(0) or by_index[min(by_index)]
        for i in missing:
            by_index[i] = stand_in
        print(f"(animations {missing} borrowed from {stand_in['name']})", end=" ")

    def source(a):
        while a["copy"]:
            a = by_name[a["copy"]]
        return a

    # palette: best of Red's 13 shared monster palettes
    sheets = {}
    for i in range(NUM_ANIMS):
        src = source(by_index[i])
        if src["name"] not in sheets:
            sheets[src["name"]] = tuple(Image.open(sc_dir / f"{src['name']}-{k}.png").convert("RGBA") for k in ("Anim", "Offsets", "Shadow"))
    colours = set()
    for anim, _, _ in sheets.values():
        colours.update(p[:3] for p in anim.getdata() if p[3] > 0)
    pals = [read_pal(ROOT / f"graphics/ax/pal/{p}.pal") for p in range(13)]
    if args.palette is None:
        args.palette = min(range(13), key=lambda p: sum(min(sum((a - b) ** 2 for a, b in zip(pals[p][i], c)) for i in range(1, 16)) for c in colours))
    pal = pals[args.palette]
    cmap = {c: nearest(pal, c) for c in colours}
    max_tiles = args.body_size * 16

    sprites = []          # list of tile lists (unique tile data)
    sprite_lookup = {}    # tuple(tiles) -> (sprite index, flipped)
    poses = []            # (sprite, y, x, shape, size, hflip)
    pose_lookup = {}
    positions = []
    anim_tables = []      # [anim][dir] -> list of (duration, pose)
    cropped = 0

    for i in range(NUM_ANIMS):
        src = source(by_index[i])
        anim_img, off_img, sh_img = sheets[src["name"]]
        fw, fh = src["w"], src["h"]
        dirs = []
        for d in range(NUM_DIRS):
            frames = []
            for f, dur in enumerate(src["durations"]):
                box = (f * fw, d * fh, (f + 1) * fw, (d + 1) * fh)
                cell, offc, shc = anim_img.crop(box), off_img.crop(box), sh_img.crop(box)
                sbb = shc.getbbox()
                ox, oy = ((sbb[0] + sbb[2]) // 2, (sbb[1] + sbb[3]) // 2) if sbb else (fw // 2, fh // 2 + 4)
                bb = cell.getbbox() or (ox, oy, ox + 1, oy + 1)
                bw, bh = bb[2] - bb[0], bb[3] - bb[1]
                w, h = pick_shape(bw, bh, max_tiles)
                if w < bw or h < bh:
                    cropped += 1
                cx, cy = (bb[0] + bb[2]) // 2, (bb[1] + bb[3]) // 2
                left, top = cx - w // 2, cy - h // 2
                piece = Image.new("P", (w, h), 0)
                ppx = piece.load()
                for y in range(h):
                    for x in range(w):
                        sx, sy = left + x, top + y
                        if 0 <= sx < fw and 0 <= sy < fh:
                            p = cell.getpixel((sx, sy))
                            if p[3] > 0:
                                ppx[x, y] = cmap[p[:3]]
                tiles = tiles_of(piece, w, h)
                key = (w, h, tuple(tiles))
                if key in sprite_lookup:
                    spr, flip = sprite_lookup[key], False
                else:
                    fkey = (w, h, tuple(hflip_tiles(tiles, w, h)))
                    if fkey in sprite_lookup:
                        spr, flip = sprite_lookup[fkey], True
                    else:
                        spr, flip = len(sprites), False
                        sprites.append((w, h, tiles))
                        sprite_lookup[key] = spr
                shape, size = OAM_SHAPES[(w, h)]
                pose = (spr, (top - oy) & 0xFF, (left - ox) & 0xFF, shape, size, flip)
                marks = {}
                for y in range(fh):
                    for x in range(fw):
                        p = offc.getpixel((x, y))
                        if p[3] > 0:
                            marks[p[:3]] = (x - ox, y - oy)
                pos = tuple(marks.get(c, (0, 0)) for c in [(0, 0, 0), (255, 0, 0), (0, 0, 255), (0, 255, 0)])
                if (pose, pos) not in pose_lookup:
                    pose_lookup[(pose, pos)] = len(poses)
                    poses.append(pose)
                    positions.append(pos)
                frames.append((dur, pose_lookup[(pose, pos)]))
            dirs.append(frames)
        anim_tables.append(dirs)

    # --- write graphics and header ---
    gfx_dir = ROOT / f"graphics/ax/mon/{args.name}"
    gfx_dir.mkdir(parents=True, exist_ok=True)
    for old in gfx_dir.glob("sprite_*.png"):
        old.unlink()
    P = args.prefix
    out = [f"static const axmain sAxMain{P};", f"const SiroArchive gAx{P} = {{\"SIRO\", &sAxMain{P}}};", ""]
    for n, (spr, y, x, shape, size, flip) in enumerate(poses, 1):
        out.append(f"static const ax_pose s{P}Pose{n}[] = {{")
        out.append(f"\tAX_POSE({spr}, OAM1({y}, {shape}, 1), OAM2({x}, ST_OAM_SIZE_{size}, FLIP({int(flip)}, 0), 0, 0), OAM3(0, 3, {args.palette})),")
        out.append("\tAX_POSE_TERMINATOR")
        out.append("};")
    gfx_count = 0
    for n, (w, h, tiles) in enumerate(sprites, 1):
        empty = tuple([0] * 64)
        runs, i = [], 0
        while i < len(tiles):
            j = i
            blank = tiles[i] == empty
            while j < len(tiles) and (tiles[j] == empty) == blank:
                j += 1
            runs.append((blank, tiles[i:j]))
            i = j
        entries = []
        for k, (blank, run) in enumerate(runs):
            if blank:
                entries.append(f"\t{{NULL, {len(run) * 32}}},")
            else:
                gfx_count += 1
                sym = f"s{P}Gfx{n}" + (f"_{k}" if k else "")
                png = f"sprite_{n}" + (f"_{k}" if k else "") + ".png"
                strip = Image.new("P", (8 * len(run), 8), 0)
                strip.putpalette([v for c in pal for v in c])
                sp = strip.load()
                for t, tile in enumerate(run):
                    for p, v in enumerate(tile):
                        sp[t * 8 + p % 8, p // 8] = v
                strip.save(gfx_dir / png)
                out.append(f"static const u8 {sym}[] = INCBIN_U8(\"graphics/ax/mon/{args.name}/{png[:-4]}.4bpp\");")
                entries.append(f"\t{{{sym}, ARRAY_COUNT({sym})}},")
        out.append(f"static const ax_sprite s{P}Sprites{n}[] = {{")
        out.extend(entries)
        out.append("\t{NULL, 0}")
        out.append("};")
    for a, dirs in enumerate(anim_tables, 1):
        for d, frames in enumerate(dirs, 1):
            out.append(f"static const ax_anim s{P}Anims_{a}_{d}[] = {{")
            for dur, pose in frames:
                out.append(f"\t{{ .frames = {dur}, .unkFlags = 0, .poseId = {pose}, .offset = {{0, 0}}, .shadow = {{0, 0}} }},")
            out.append("\tAX_ANIM_TERMINATOR")
            out.append("};")
        out.append(f"static const ax_anim *const s{P}AnimTable{a}[] = {{")
        out.extend(f"\ts{P}Anims_{a}_{d}," for d in range(1, NUM_DIRS + 1))
        out.append("};")
    out.append(f"static const ax_pose *const sAxPoses{P}[] = {{")
    out.extend(f"\ts{P}Pose{n}," for n in range(1, len(poses) + 1))
    out.append("};")
    out.append(f"static const struct PositionSets sAxPositions{P}[] = {{")
    for n, pos in enumerate(positions):
        out.append(f"\t[{n}] = {{ .set = {{ " + ", ".join(f"{{{x}, {y}}}" for x, y in pos) + " } },")
    out.append("};")
    out.append(f"static const ax_anim *const *const sAxAnimations{P}[] = {{")
    out.extend(f"\ts{P}AnimTable{a}," for a in range(1, NUM_ANIMS + 1))
    out.append("};")
    out.append(f"static const ax_sprite *const sAxSprites{P}[] = {{")
    out.extend(f"\ts{P}Sprites{n}," for n in range(1, len(sprites) + 1))
    out.append("};")
    out.append(f"static const axmain sAxMain{P} = {{")
    out.append(f"\t.poses = sAxPoses{P},")
    out.append(f"\t.animations = sAxAnimations{P},")
    out.append(f"\t.animCount = ARRAY_COUNT(sAxAnimations{P}),")
    out.append(f"\t.spriteData = sAxSprites{P},")
    out.append(f"\t.positions = sAxPositions{P},")
    out.append("};")
    (ROOT / f"src/data/ax/{args.name}.h").write_text("\n".join(out) + "\n")
    print(f"{args.name}: palette {args.palette}, {len(poses)} poses, {len(sprites)} sprites, {gfx_count} gfx chunks, "
          f"{cropped} frames cropped to fit {max_tiles} tiles")


if __name__ == "__main__":
    main()
