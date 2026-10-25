#!/bin/bash
# Run harness tests.  tools/dxtest/run.sh <test>... | all | smoke
#
# Each tests/<name>.txt may start with directives:
#   #rom modern|debug|red   which built ROM to boot (default modern)
#   #save NAME|-            checkpoint from <project>/saves to boot with (copied; default -)
# Screenshots and results go to <project>/test_out/<name>/.
set -uo pipefail

PROJECT_DIR="${DX_PROJECT_DIR:-/mnt/c/Users/andrew-pc/Desktop/Coding Projects/Red Rescue Team}"
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
SMOKE="boot_title load_town"

[[ -x "$HERE/harness" && "$HERE/harness" -nt "$HERE/harness.c" ]] || gcc -O2 -Wall -o "$HERE/harness" "$HERE/harness.c" -lmgba -lpng || exit 2

tests=()
[[ $# -gt 0 ]] || set -- smoke
for arg in "$@"; do
    case "$arg" in
        smoke) tests+=($SMOKE) ;;
        all) for f in "$HERE"/tests/*.txt; do tests+=("$(basename "$f" .txt)"); done ;;
        *) tests+=("$arg") ;;
    esac
done
[[ ${#tests[@]} -gt 0 ]] || { echo "usage: $0 <test>... | smoke | all"; exit 2; }

pass=0; fail=0; summary=""
for t in "${tests[@]}"; do
    script="$HERE/tests/$t.txt"
    [[ -f "$script" ]] || { echo "no such test: $t"; fail=$((fail+1)); continue; }
    rom=$(sed -n 's/^#rom[[:space:]]\+//p' "$script" | head -1); rom=${rom:-modern}
    save=$(sed -n 's/^#save[[:space:]]\+//p' "$script" | head -1); save=${save:--}
    case "$rom" in red) gba="$REPO/pmd_red.gba" ;; *) gba="$REPO/pmd_red_$rom.gba" ;; esac
    elf="${gba%.gba}.elf"
    out="$PROJECT_DIR/test_out/$t"
    rm -rf "$out"; mkdir -p "$out"
    arm-none-eabi-nm "$elf" > "$out/symbols.txt"
    savearg=-
    if [[ "$save" != "-" ]]; then
        cp "$PROJECT_DIR/saves/$save.sav" "$out/boot.sav" || { fail=$((fail+1)); continue; }
        savearg="$out/boot.sav"
    fi
    echo "== $t ($(basename "$gba"), save=$save)"
    if "$HERE/harness" "$gba" "$savearg" "$out/symbols.txt" "$script" "$out" > /dev/null; then
        pass=$((pass+1)); summary+="PASS $t"$'\n'
    else
        fail=$((fail+1)); summary+="FAIL $t"$'\n'
        grep -E "^(FAIL|ERROR)" "$out/results.txt" | sed 's/^/   /'
    fi
    rm -f "$out/symbols.txt"
done
printf "%s" "$summary"
echo "== $pass passed, $fail failed"
[[ $fail -eq 0 ]]
