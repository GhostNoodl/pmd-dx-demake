#!/bin/bash
# dx test kit: build the modern ROM, copy it to the Windows builds folder,
# optionally drop in a checkpoint save, and print how full the ROM is.
#
#   tools/dxtest.sh [--debug] [checkpoint]
#
#   --debug      build with DX_DEBUG=1 (pmd_red_debug.gba, debug menu on main menu)
#   checkpoint   name of a save in <project>/saves (with or without .sav); it is
#                copied in as the ROM's .sav after backing up the current one
set -euo pipefail

# Windows project folder (builds/, saves/, test_out/); set it in tools/dxtest/local.env (gitignored)
[[ -f "$(dirname "$0")/dxtest/local.env" ]] && source "$(dirname "$0")/dxtest/local.env"
[[ -f "$(dirname "$0")/local.env" ]] && source "$(dirname "$0")/local.env"
PROJECT_DIR="${DX_PROJECT_DIR:?set DX_PROJECT_DIR in tools/dxtest/local.env}"
BUILDS_DIR="$PROJECT_DIR/builds"
SAVES_DIR="$PROJECT_DIR/saves"

variant=modern
make_args=()
if [[ "${1:-}" == "--debug" ]]; then
    variant=debug
    make_args=(DX_DEBUG=1)
    shift
fi
checkpoint="${1:-}"
rom="pmd_red_$variant.gba"
elf="pmd_red_$variant.elf"

cd "$(dirname "$0")/.."

echo "== Building $rom"
start=$(date +%s%N)
if ! make -j"$(nproc)" modern "${make_args[@]}" > build_dxtest.log 2>&1; then
    tail -40 build_dxtest.log
    echo "== BUILD FAILED (full log: ~/pmd/pmd-red/build_dxtest.log)"
    exit 1
fi
echo "   built in $(( ($(date +%s%N) - start) / 1000000 )) ms"

# DX data must not knock Red's word-aligned tables off alignment (tools/dxtest/check_align.py)
if [[ -f pmd_red.elf ]]; then
    python3 -I tools/dxtest/check_align.py pmd_red.elf "$elf" || { echo "== ALIGNMENT CHECK FAILED"; exit 1; }
else
    echo "   (no pmd_red.elf: run make compare once to enable the alignment check)"
fi

mkdir -p "$BUILDS_DIR" "$SAVES_DIR"
cp "$rom" "$BUILDS_DIR/$rom"

# ROM usage = end of the last loadable segment in cartridge space
used=$(arm-none-eabi-readelf -lW "$elf" | awk '$1 == "LOAD" { pa = strtonum($4); fs = strtonum($5); if (pa >= 0x08000000 && pa + fs > max) max = pa + fs } END { printf "%d", max - 0x08000000 }')
total=$((32 * 1024 * 1024))
free=$((total - used))
awk -v u="$used" -v t="$total" -v f="$free" 'BEGIN { printf "   ROM: %d / %d bytes used (%.2f%%), %d bytes (%.1f KB) free\n", u, t, 100 * u / t, f, f / 1024 }'

if [[ -n "$checkpoint" ]]; then
    src="$SAVES_DIR/${checkpoint%.sav}.sav"
    if [[ ! -f "$src" ]]; then
        echo "== No checkpoint '$checkpoint'. Available:"
        ls "$SAVES_DIR" | grep '\.sav$' | sed 's/\.sav$//; s/^/   /'
        exit 1
    fi
    dst="$BUILDS_DIR/${rom%.gba}.sav"
    if [[ -f "$dst" ]]; then
        mkdir -p "$SAVES_DIR/_backups"
        bak="$SAVES_DIR/_backups/${rom%.gba}-$(date +%Y%m%d-%H%M%S).sav"
        cp "$dst" "$bak"
        echo "   backed up current save to saves/_backups/$(basename "$bak")"
    fi
    cp "$src" "$dst"
    echo "   loaded checkpoint '${checkpoint%.sav}'"
fi

echo "== Ready: builds/$rom"
