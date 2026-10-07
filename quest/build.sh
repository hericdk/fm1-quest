#!/bin/sh
# Build web/sloop.wasm: the SLOOP firmware of this repository (unmodified) + the web HAL (src/sloop_wasm.c, from
# sabliran/sloop-web-emu) + src/quest_wasm.c (read-only state export for the skin).
#   needs: clang + wasm-ld (or zig: `pip install ziglang`, then CC=zig), python3 + numpy + pillow
#   ./build.sh                 # SLOOP defaults to the repository root (..)
set -e
cd "$(dirname "$0")"
SLOOP=${SLOOP:-$(cd .. && pwd)}
CC=${CC:-clang}
TARGET=wasm32
if [ "$CC" = zig ]; then CC="python3 -m ziglang cc"; TARGET=wasm32-freestanding; fi
if [ ! -f "$SLOOP/build/gen/felucca_tables.h" ]; then      # the firmware's generated tables, fonts, icons, samples
    mkdir -p "$SLOOP/build/gen"
    for g in font icons tables samples drumkits logo; do
        out="$SLOOP/build/gen/felucca_$g.h"; [ "$g" = logo ] && out="$SLOOP/build/gen/sloop_logo.h"
        python3 "$SLOOP/tools/gen_$g.py" "$out"
    done
fi
python3 tools/patch_hooks.py "$SLOOP/firmware/src" build/quest_src
$CC --target=$TARGET -O2 -fno-builtin -ffreestanding -nostdlib -w \
  -I build/quest_src -I "$SLOOP/build/gen" -I "$SLOOP/firmware/src" -I src \
  -Wl,--no-entry -Wl,--export-memory -Wl,-z,stack-size=1048576 -Wl,--global-base=1048576 \
  -o web/sloop.wasm src/quest_wasm.c
ls -la web/sloop.wasm
